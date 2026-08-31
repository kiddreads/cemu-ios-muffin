// Converting a disc image into a .wua, on the device.
//
// Lives on the CMake side rather than in CemuBridge.mm for the same reason
// IOSTitleLaunch.cpp does: TitleInfo.h and WuaWriter.h drag in pugixml and ZArchive,
// which this build already resolves and Xcode's build of that one file would have to be
// taught about. The bridge sees a flat C surface.
//
// What this actually gives the user: a .wud or .wux is an encrypted disc image that needs
// a matching key out of their keys.txt every single time it is opened. The .wua this
// produces is decrypted, roughly a third to a half the size, opens without any keys at
// all, and is one file instead of a folder tree. The decryption itself is not implemented
// here - TitleInfo::Mount has already done it before a byte reaches WuaWriter.

#include "Cafe/TitleList/TitleInfo.h"
#include "Cafe/TitleList/WuaWriter.h"
#include "Cemu/Logging/CemuLogging.h"

#include <atomic>
#include <memory>
#include <pthread.h>
#include <string>
#include <system_error>
#include <thread>

extern "C" void cemu_bridge_install_thread_crash_stack(void);

// Mirrored by IOSConvertProgress in CemuBridge.h. Plain types across the boundary.
struct IOSConvertProgress
{
	int stage;
	unsigned int totalFileCount;
	unsigned int currentFileIndex;
	unsigned long long totalInputBytes;
	unsigned long long transferredInputBytes;
};

namespace
{
	// One conversion at a time. It is disk-bound and the device has one disk, so a queue
	// would add complexity with no throughput to gain.
	std::atomic<bool> s_running{false};
	WuaWriter::Progress s_progress;
	std::string s_error;
	std::atomic<int> s_result{-1}; // -1 running, 0 ok, 1 failed
	std::string s_sourcePath;
	std::string s_outputPath;
	bool s_asFolder = false;

	void ConvertWorker()
	{
		// Same reason the boot thread takes one: this recurses over a title's whole
		// directory tree, and a 512 KB stack kills the process with no crash block at all.
		cemu_bridge_install_thread_crash_stack();

		std::error_code ec;
		const fs::path source{s_sourcePath};
		const fs::path output{s_outputPath};

		TitleInfo title{source};
		if (!title.IsValid())
		{
			switch (title.GetInvalidReason())
			{
			case TitleInfo::InvalidReason::NO_DISC_KEY:
				s_error = "This is an encrypted disc image and no key in your keys.txt opens it.";
				break;
			case TitleInfo::InvalidReason::NO_TITLE_TIK:
				s_error = "This title has no usable title.tik.";
				break;
			default:
				s_error = "This file is not a title Muffin can open.";
				break;
			}
			s_progress.stage = WuaWriter::Stage::Failed;
			s_result = 1;
			s_running = false;
			return;
		}

		const TitleId titleId = title.GetAppTitleId();
		cemuLog_log(LogType::Force, "iOS: converting {:016x} to {}", (uint64)titleId, _pathToUtf8(output));

		TitleInfo* titles[] = {&title};
		const bool converted = s_asFolder
			? WuaWriter::ConvertTitlesToFolder(titles, output, s_progress, s_error)
			: WuaWriter::ConvertTitlesToFile(titles, output, s_progress, s_error);
		if (!converted)
		{
			if (s_error.empty())
				s_error = "The conversion did not finish.";
			s_result = 1;
			s_running = false;
			return;
		}

		// Verify BEFORE anyone is told it succeeded. This is what earns the right to
		// offer deleting the original afterwards; without it "converted" would mean only
		// "a file of about the right size exists".
		s_progress.stage = WuaWriter::Stage::Verifying;
		if (!WuaWriter::VerifyFile(output, titleId, s_error))
		{
			// Whole tree for a folder, single file for an archive. Either way the
			// half-made thing must not survive to be offered as a game.
			if (s_asFolder)
				fs::remove_all(output, ec);
			else
				fs::remove(output, ec);
			s_progress.stage = WuaWriter::Stage::Failed;
			s_result = 1;
			s_running = false;
			cemuLog_log(LogType::Force, "iOS: conversion produced a file that did not verify - discarded");
			return;
		}

		s_progress.stage = WuaWriter::Stage::Done;
		s_result = 0;
		s_running = false;
		cemuLog_log(LogType::Force, "iOS: conversion finished and verified");
	}
} // namespace

extern "C" int IOSTitleConvert_Start(const char* sourcePath, const char* outputPath, int asFolder)
{
	if (!sourcePath || !outputPath)
		return 0;
	bool expected = false;
	if (!s_running.compare_exchange_strong(expected, true))
		return 0; // one at a time

	s_sourcePath = sourcePath;
	s_outputPath = outputPath;
	s_asFolder = (asFolder != 0);
	s_error.clear();
	s_result = -1;
	s_progress.stage = WuaWriter::Stage::Starting;
	s_progress.totalFileCount = 0;
	s_progress.currentFileIndex = 0;
	s_progress.totalInputBytes = 0;
	s_progress.transferredInputBytes = 0;
	s_progress.cancelRequested = false;

	// A pthread with an explicit 16 MB stack rather than std::thread, which offers no way
	// to ask for one.
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr, 16u * 1024 * 1024);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_t tid;
	const int rc = pthread_create(&tid, &attr, [](void*) -> void* {
		ConvertWorker();
		return nullptr;
	}, nullptr);
	pthread_attr_destroy(&attr);

	if (rc != 0)
	{
		s_error = "Could not start the conversion.";
		s_result = 1;
		s_running = false;
		return 0;
	}
	return 1;
}

extern "C" void IOSTitleConvert_Poll(IOSConvertProgress* out)
{
	if (!out)
		return;
	out->stage = (int)s_progress.stage.load();
	out->totalFileCount = s_progress.totalFileCount.load();
	out->currentFileIndex = s_progress.currentFileIndex.load();
	out->totalInputBytes = s_progress.totalInputBytes.load();
	out->transferredInputBytes = s_progress.transferredInputBytes.load();
}

extern "C" void IOSTitleConvert_Cancel(void)
{
	s_progress.cancelRequested = true;
}

extern "C" int IOSTitleConvert_Result(char* errBuf, int errBufLen)
{
	const int r = s_result.load();
	if (errBuf && errBufLen > 0)
	{
		std::snprintf(errBuf, (size_t)errBufLen, "%s", s_error.c_str());
	}
	return r;
}
