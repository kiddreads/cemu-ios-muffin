#pragma once

#include "Cafe/TitleList/TitleInfo.h"

#include <atomic>
#include <functional>
#include <span>
#include <string>

// Converting a title into a .wua archive, in one place.
//
// WHY THIS FILE EXISTS
//
// This logic existed twice - once in src/android/.../WuaConverter.cpp behind a JNI
// callback interface, and once inlined into src/gui/wxgui/.../wxTitleManagerList.cpp
// behind a wx progress dialog - as roughly 150 copy-pasted lines each. Neither was
// reachable from iOS, so the port had no way to convert anything despite the engine
// already being able to read every format involved.
//
// WHAT IT ACTUALLY DOES, AND WHY THAT MATTERS FOR ENCRYPTED DISCS
//
// It reads through the FSC virtual filesystem, which means TitleInfo::Mount has already
// opened the source - finding the AES key in the user's keys.txt for a .wud/.wux, or
// reading title.tik for a NUS install - before a single byte reaches this code. So what
// gets written into the archive is PLAINTEXT, and the conversion is format-agnostic: it
// does not know or care whether the source was an encrypted disc image, an extracted
// folder or an install.
//
// That is what makes "decrypt this disc image on the device" a wiring job rather than a
// cryptography one. The decryption already worked; nothing on iOS could ask for it.
//
// The resulting .wua also needs no keys.txt to open ever again, which is the practical
// reason a user would want one.
namespace WuaWriter
{
	enum class Stage
	{
		Starting,
		CollectingFiles,   // walking the source to count what is in it
		Compressing,       // the long part
		Finalizing,
		Verifying,
		Done,
		Cancelled,
		Failed
	};

	/// Written by the worker, read by whatever is drawing a progress bar. Atomics because
	/// those are different threads and a torn count is not worth a lock on this path.
	struct Progress
	{
		std::atomic<Stage> stage{Stage::Starting};
		std::atomic_uint32_t totalFileCount{0};
		std::atomic_uint32_t currentFileIndex{0};
		std::atomic_uint64_t totalInputBytes{0};
		std::atomic_uint64_t transferredInputBytes{0};
		std::atomic_bool cancelRequested{false};
	};

	/// Returns false on a short write. The Android version discarded write()'s result,
	/// so a full disk or an interrupted write silently truncated the archive and the
	/// only check afterwards was whether the file could be opened at all.
	using WriteFn = std::function<bool(const void* data, size_t length)>;

	/// Blocking. The caller owns the thread, and on iOS that thread needs a large stack -
	/// this recurses over the whole directory tree of a title.
	bool ConvertTitles(std::span<TitleInfo* const> titles, WriteFn onWrite,
					   Progress& progress, std::string& errorOut);

	/// Opens outputPath, streams into it, closes it. Produces a .wua.
	bool ConvertTitlesToFile(std::span<TitleInfo* const> titles, const fs::path& outputPath,
							 Progress& progress, std::string& errorOut);

	/// Writes the decrypted title out as a plain folder - code/, content/, meta/ - instead
	/// of an archive.
	///
	/// WHY BOTH EXIST
	///
	/// ZArchive compresses with zstd, so every read from a .wua decompresses. That is a
	/// real cost and it is fair to want to avoid it. A folder is decrypted, uncompressed
	/// and read straight off the filesystem.
	///
	/// The trade is not obvious in either direction, which is why this is a choice rather
	/// than a replacement:
	///
	///   .wua    - roughly a third to a half the size. Decompression is native ARM64 code
	///             at hundreds of MB/s, against an emulator whose bottleneck is an
	///             interpreted PowerPC core, and a smaller file is also less flash I/O.
	///   folder  - no decompression at all, but two to three times the disk. On a device
	///             where storage is fixed and a Wii U title can be 25 GB, that is the
	///             cost that actually bites most people.
	///
	/// Both are formats the engine already mounts, so neither is a special case at load.
	bool ConvertTitlesToFolder(std::span<TitleInfo* const> titles, const fs::path& outputDir,
							   Progress& progress, std::string& errorOut);

	/// Reopens a written archive and confirms it actually contains the title it should.
	/// The Android version's verification was a `// todo` that only checked the file
	/// opened; this is what earns the right to offer deleting the original.
	bool VerifyFile(const fs::path& path, TitleId expectedTitleId, std::string& errorOut);

	/// "Name (US) (v32).wua"
	std::string SuggestFileName(TitleInfo* base, TitleInfo* update, TitleInfo* aoc);
} // namespace WuaWriter
