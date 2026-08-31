#include "Cafe/HW/Latte/Renderer/Metal/MetalBinaryArchive.h"
#include "Cafe/HW/Latte/Renderer/Metal/MetalRenderer.h"
#include "Cafe/HW/Latte/Renderer/Metal/MetalCommon.h"
#include "Common/version.h"
#include "Cemu/Logging/CemuLogging.h"
#include "config/ActiveSettings.h"
#include "Cafe/GameProfile/GameProfile.h"

#include <sys/sysctl.h>
#include <system_error>

namespace
{
	// Bump on ANY change to the MSL emitter or to how MetalPipelineCompiler builds its
	// descriptors. GeneratePrecompiledCacheId() below only moves on a release boundary,
	// so without this two dev builds inside one version would share an archive and the
	// second would be reading pipelines built from different source.
	constexpr uint32 kMetalTranslatorVersion = 1;

	// Above this the archive stops accepting new pipelines. Chosen to bound the
	// pathological case - a title that generates pipelines without end - rather than to
	// be tight. Going over it costs future speedups, never correctness.
	constexpr uintmax_t kMaxArchiveBytes = 256ull * 1024ull * 1024ull;

	std::atomic<bool> s_enabled{true};

	std::string SysctlString(const char* name)
	{
		size_t len = 0;
		if (sysctlbyname(name, nullptr, &len, nullptr, 0) != 0 || len == 0)
			return std::string();
		std::string out(len, '\0');
		if (sysctlbyname(name, out.data(), &len, nullptr, 0) != 0)
			return std::string();
		if (!out.empty() && out.back() == '\0')
			out.pop_back();
		return out;
	}

	uint64 FoldString(uint64 seed, const std::string& s)
	{
		for (unsigned char c : s)
			seed = (seed ^ c) * 0x100000001b3ull;
		return seed;
	}

	uint64 FoldValue(uint64 seed, uint64 v)
	{
		for (int i = 0; i < 8; i++)
			seed = (seed ^ ((v >> (i * 8)) & 0xFF)) * 0x100000001b3ull;
		return seed;
	}
} // namespace

bool MetalBinaryArchive::IsEnabled()
{
	return s_enabled.load(std::memory_order_relaxed);
}

void MetalBinaryArchive::SetEnabled(bool enabled)
{
	s_enabled.store(enabled, std::memory_order_relaxed);
}

MetalBinaryArchive* MetalBinaryArchive::OpenOrCreate(MetalRenderer* mtlr, uint64 titleId, uint16 titleVersion)
{
	if (!IsEnabled() || !mtlr || !mtlr->GetDevice())
		return nullptr;

	MTL::Device* device = mtlr->GetDevice();

	// The key goes in the FILENAME, not inside the file, so invalidation is "a different
	// file exists" rather than open-parse-compare-discard. Everything that can change what
	// a compiled pipeline should be has to be in here.
	const std::string gpuName = device->name() ? device->name()->utf8String() : "unknown";
	// Build, not product version: the Metal compiler ships with the OS, and a point
	// update or a beta can change its output while the product version stands still.
	const std::string osBuild = SysctlString("kern.osversion");

	uint64 key = 0xcbf29ce484222325ull;
	key = FoldValue(key, titleId);
	key = FoldValue(key, titleVersion);
	// The same inputs RendererShader::GeneratePrecompiledCacheId() folds, built here
	// rather than by calling it, because that function is protected and this is not a
	// RendererShader. Duplicating three version macros is a smaller price than either
	// widening its access or inheriting from a shader class to borrow one number.
	key = FoldString(key, EMULATOR_VERSION_SUFFIX);
	key = FoldValue(key, EMULATOR_VERSION_MAJOR);
	key = FoldValue(key, EMULATOR_VERSION_MINOR);
	key = FoldValue(key, EMULATOR_VERSION_PATCH);
	key = FoldValue(key, (uint64)g_current_game_profile->GetAccurateShaderMul());
	key = FoldValue(key, kMetalTranslatorVersion);
	key = FoldString(key, gpuName);
	key = FoldString(key, osBuild);
	// Capability bits, because they change which code path built the descriptor.
	key = FoldValue(key, (uint64)mtlr->IsAppleGPU() | ((uint64)mtlr->SupportsMeshShaders() << 1));
	// Compile options that change the AIR, and therefore every function identity. A
	// mismatch here would already be a guaranteed miss; including them just stops us
	// carrying an archive full of permanently-dead entries.
	key = FoldValue(key, (uint64)g_current_game_profile->GetShaderFastMath());
	key = FoldValue(key, (uint64)mtlr->GetPositionInvariance());

	const std::string keyStr = fmt::format("{:016x}", key);
	const fs::path dir = ActiveSettings::GetCachePath("shaderCache/precompiled");
	std::error_code ec;
	fs::create_directories(dir, ec);

	// Whole-file eviction. An archive has no per-entry removal API, so a stale one is
	// deleted outright rather than left to accumulate beside its replacement.
	for (auto& entry : fs::directory_iterator(dir, ec))
	{
		if (ec)
			break;
		const std::string name = entry.path().filename().string();
		if (name.rfind(fmt::format("{:016x}_", titleId), 0) != 0)
			continue;
		if (name.find(keyStr) != std::string::npos)
			continue;
		std::error_code rmEc;
		fs::remove(entry.path(), rmEc);
		cemuLog_log(LogType::Force, "Metal binary archive: discarded {} - built for a different device, OS or build", name);
	}

	const fs::path path = dir / fmt::format("{:016x}_{}.mtlarchive", titleId, keyStr);

	NS::Error* error = nullptr;
	MTL::BinaryArchiveDescriptor* desc = MTL::BinaryArchiveDescriptor::alloc()->init();
	const bool existed = fs::exists(path, ec);
	if (existed)
		desc->setUrl(ToNSURL(_pathToUtf8(path)));

	MTL::BinaryArchive* archive = device->newBinaryArchive(desc, &error);
	if (!archive && existed)
	{
		// Metal refused the file - a different Metal version, or it is damaged. This is
		// the automatic invalidation our own key cannot cover, and it must never be
		// fatal: throw the file away and start an empty one.
		cemuLog_log(LogType::Force, "Metal binary archive: {} could not be opened ({}), starting a new one",
					_pathToUtf8(path), error && error->localizedDescription() ? error->localizedDescription()->utf8String() : "no reason given");
		std::error_code rmEc;
		fs::remove(path, rmEc);
		desc->setUrl(nullptr);
		error = nullptr;
		archive = device->newBinaryArchive(desc, &error);
	}
	desc->release();

	if (!archive)
	{
		cemuLog_log(LogType::Force, "Metal binary archive: unavailable ({}), pipelines will be compiled every launch as before",
					error && error->localizedDescription() ? error->localizedDescription()->utf8String() : "no reason given");
		return nullptr;
	}

	auto* self = new MetalBinaryArchive();
	self->m_archive = archive;
	self->m_archiveArray = NS::Array::array(archive);
	self->m_archiveArray->retain();
	self->m_path = path;
	self->m_key = keyStr;
	self->m_deviceDescription = fmt::format("{} / {}", gpuName, osBuild.empty() ? "unknown OS" : osBuild);

	cemuLog_log(LogType::Force, "Metal binary archive: {} for {:016x} v{} ({})",
				existed ? "opened" : "created", titleId, titleVersion, self->m_deviceDescription);
	return self;
}

MetalBinaryArchive::~MetalBinaryArchive()
{
	if (m_archiveArray)
		m_archiveArray->release();
	if (m_archive)
		m_archive->release();
}

void MetalBinaryArchive::AttachTo(MTL::RenderPipelineDescriptor* desc)
{
	if (desc && m_archiveArray)
		desc->setBinaryArchives(m_archiveArray);
}

void MetalBinaryArchive::Add(const MTL::RenderPipelineDescriptor* desc)
{
	if (!desc || m_capped)
		return;

	NS::Error* error = nullptr;
	{
		std::scoped_lock lock(m_mutex);
		if (!m_archive->addRenderPipelineFunctions(desc, &error))
		{
			// Not worth failing a launch over. The pipeline itself compiled fine; all
			// that is lost is the chance to skip compiling it next time.
			cemuLog_logOnce(LogType::Force, "Metal binary archive: a pipeline could not be added, it will be compiled again next launch");
			return;
		}
	}
	m_adds.fetch_add(1, std::memory_order_relaxed);
	m_pendingAdds.fetch_add(1, std::memory_order_relaxed);
}

void MetalBinaryArchive::SerializeIfDirty(uint32 minPendingAdds)
{
	if (m_pendingAdds.load(std::memory_order_relaxed) == 0)
		return;
	if (minPendingAdds != 0 && m_pendingAdds.load(std::memory_order_relaxed) < minPendingAdds)
		return;

	// Write beside the real file and rename over it. Truncating the live archive and
	// then being killed mid-write would leave the user with neither the old one nor a
	// new one, and the next launch would compile everything from scratch with no warning.
	const fs::path tmp = fs::path(m_path).concat(".tmp");
	NS::Error* error = nullptr;
	bool ok;
	{
		std::scoped_lock lock(m_mutex);
		ok = m_archive->serializeToURL(ToNSURL(_pathToUtf8(tmp)), &error);
	}
	if (!ok)
	{
		cemuLog_log(LogType::Force, "Metal binary archive: could not be written ({})",
					error && error->localizedDescription() ? error->localizedDescription()->utf8String() : "no reason given");
		std::error_code rmEc;
		fs::remove(tmp, rmEc);
		return;
	}

	std::error_code ec;
	fs::rename(tmp, m_path, ec);
	if (ec)
	{
		fs::remove(tmp, ec);
		return;
	}
	m_pendingAdds.store(0, std::memory_order_relaxed);

	const uintmax_t size = fs::file_size(m_path, ec);
	if (!ec && size > kMaxArchiveBytes && !m_capped)
	{
		m_capped = true;
		cemuLog_log(LogType::Force, "Metal binary archive: reached {} MB and will stop growing - existing entries are still used",
					(unsigned long long)(size / (1024ull * 1024ull)));
	}
}

void MetalBinaryArchive::LogSummary() const
{
	std::error_code ec;
	const uintmax_t size = fs::file_size(m_path, ec);
	cemuLog_log(LogType::Force, "Metal binary archive: {} hits, {} misses, {} added, {} MB, key {} ({})",
				m_hits.load(std::memory_order_relaxed),
				m_misses.load(std::memory_order_relaxed),
				m_adds.load(std::memory_order_relaxed),
				ec ? 0ull : (unsigned long long)(size / (1024ull * 1024ull)),
				m_key, m_deviceDescription);
}
