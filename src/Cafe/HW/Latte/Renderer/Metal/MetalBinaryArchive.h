#pragma once

#include <Metal/Metal.hpp>
#include <atomic>
#include <mutex>
#include <string>
#include "Common/precompiled.h"

// A persistent store of COMPILED pipelines, so the same title on the same device does
// not recompile every one of them on every launch.
//
// WHAT THIS FIXES
//
// There are already two caches on disk, and both store a recipe rather than a result.
// The shader cache holds Wii U R600 bytecode plus GPU register state; the pipeline cache
// holds shader hashes plus attachment formats plus register state. On load, the pipeline
// cache rehydrates that state and RE-RUNS the real compiler. So every launch of a title
// pays for the whole R600 -> MSL -> AIR -> GPU-ISA chain again, identically, for a result
// that was byte-for-byte the same the last time. That is the loading screen.
//
// WHAT IT DOES NOT FIX, AND THIS MATTERS FOR THE CLAIMS WE MAKE
//
// A binary archive is keyed on the descriptor plus the identity of the MTLFunctions
// attached to it, so a lookup still requires real MTLFunctions - newLibrary(source) still
// runs for every shader. This removes the back end (AIR -> GPU ISA, and pipeline
// specialisation), not the front end. That is still the majority of the work here because
// pipelines heavily outnumber shaders, but "most of it" is the honest claim, not "all".
//
// WHY THIS IS SAFE IN A WAY A HAND-ROLLED BINARY CACHE WOULD NOT BE
//
// Function identity is content-derived. A shader whose code changed is therefore a
// structural MISS, not a stale hit - it is not possible for this to hand back last
// version's compiled code. That is the difference between it and the abandoned AIR cache
// in RendererShaderMtl.cpp, which keyed on baseHash/auxHash and would have. Metal also
// refuses to open an archive written by a different device, OS or Metal version, which is
// a second net underneath our own key.
class MetalBinaryArchive
{
  public:
	// Returns nullptr when the feature is off or unavailable. Every caller must cope with
	// that - a device with no archive has to keep working exactly as before.
	static MetalBinaryArchive* OpenOrCreate(class MetalRenderer* mtlr, uint64 titleId, uint16 titleVersion);
	~MetalBinaryArchive();

	// Attach for lookup. Cheap, and safe to call on every compile.
	void AttachTo(MTL::RenderPipelineDescriptor* desc);

	// Record a pipeline that had to be compiled the slow way.
	void Add(const MTL::RenderPipelineDescriptor* desc);

	void NoteHit() { m_hits.fetch_add(1, std::memory_order_relaxed); }
	void NoteMiss() { m_misses.fetch_add(1, std::memory_order_relaxed); }

	// Writes only when there is something new. minPendingAdds == 0 forces.
	void SerializeIfDirty(uint32 minPendingAdds);

	// One line, at title exit. This is what tells you from a device log whether the
	// feature did anything at all, which is the only way to know without a profiler.
	void LogSummary() const;

	static bool IsEnabled();
	static void SetEnabled(bool enabled);

  private:
	MetalBinaryArchive() = default;

	MTL::BinaryArchive* m_archive = nullptr;
	// One-element array holding m_archive, built once and reused for every descriptor.
	// Rebuilding it per pipeline would allocate on the compile path for no reason.
	NS::Array* m_archiveArray = nullptr;
	fs::path m_path;

	// MTLBinaryArchive is not safe for concurrent add/serialize, and there are up to
	// eight compiler threads adding. The critical section is one API call on each side.
	std::mutex m_mutex;

	std::atomic_uint32_t m_pendingAdds{0};
	std::atomic_uint32_t m_hits{0};
	std::atomic_uint32_t m_misses{0};
	std::atomic_uint32_t m_adds{0};
	// Set when the archive has grown past what is reasonable. Lookups keep working; only
	// adding stops. The failure mode becomes "stops improving", never "becomes wrong".
	bool m_capped = false;
	std::string m_key;
	std::string m_deviceDescription;
};
