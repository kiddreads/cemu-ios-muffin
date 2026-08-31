#include "Cafe/TitleList/WuaWriter.h"
#include "Cafe/Filesystem/fsc.h"
#include "Cemu/Logging/CemuLogging.h"
#include "util/helpers/ZArchiveHelpers.h"

#include <zarchive/zarchivewriter.h>

#include <fstream>
#include <memory>
#include <vector>

namespace WuaWriter
{
	namespace
	{
		struct Context
		{
			ZArchiveWriter* writer{nullptr};
			WriteFn write;
			Progress* progress{nullptr};
			std::vector<uint8> transferBuffer;
			bool writeFailed{false};

			static void NewOutputFileCb(const int32_t partIndex, void* ctx)
			{
				// Multi-part output. Not used here: everything goes to one stream.
				(void)partIndex;
				(void)ctx;
			}

			static void WriteOutputDataCb(const void* data, size_t length, void* rawCtx)
			{
				auto* ctx = static_cast<Context*>(rawCtx);
				if (ctx->writeFailed)
					return;
				if (!ctx->write(data, length))
				{
					// A short write here used to be discarded outright, which meant a
					// full disk produced a truncated archive that still looked like an
					// archive. Recorded and checked once the writer is finished, because
					// ZArchiveWriter's callback has no way to report failure back into it.
					ctx->writeFailed = true;
					cemuLog_log(LogType::Force, "WuaWriter: a write failed - the output is incomplete and will be discarded");
				}
			}

			bool cancelled() const { return progress->cancelRequested.load(); }

			bool CountFiles(const std::string& fscPath)
			{
				sint32 status;
				std::unique_ptr<FSCVirtualFile> dir(fsc_openDirIterator(fscPath.c_str(), &status));
				if (!dir)
					return false;
				FSCDirEntry entry;
				while (fsc_nextDir(dir.get(), &entry))
				{
					if (cancelled())
						return false;
					if (entry.isFile)
					{
						progress->totalInputBytes += (uint64)entry.fileSize;
						progress->totalFileCount++;
					}
					else if (entry.isDirectory && !CountFiles(fmt::format("{}{}/", fscPath, entry.path)))
						return false;
				}
				return true;
			}

			bool AddFiles(const std::string& archivePath, const std::string& fscPath)
			{
				sint32 status;
				std::unique_ptr<FSCVirtualFile> dir(fsc_openDirIterator(fscPath.c_str(), &status));
				if (!dir)
					return false;
				writer->MakeDir(archivePath.c_str(), false);
				FSCDirEntry entry;
				while (fsc_nextDir(dir.get(), &entry))
				{
					if (cancelled() || writeFailed)
						return false;
					if (entry.isFile)
					{
						writer->StartNewFile((archivePath + entry.path).c_str());
						std::unique_ptr<FSCVirtualFile> file(
							fsc_open((fscPath + entry.path).c_str(),
									 FSC_ACCESS_FLAG::OPEN_FILE | FSC_ACCESS_FLAG::READ_PERMISSION, &status));
						if (!file)
							return false;
						transferBuffer.resize(32 * 1024);
						while (true)
						{
							const uint32 read = file->fscReadData(transferBuffer.data(), transferBuffer.size());
							if (read == 0)
								break;
							writer->AppendData(transferBuffer.data(), read);
							if (cancelled() || writeFailed)
								return false;
							progress->transferredInputBytes += read;
						}
						progress->currentFileIndex++;
					}
					else if (entry.isDirectory &&
							 !AddFiles(fmt::format("{}{}/", archivePath, entry.path),
									   fmt::format("{}{}/", fscPath, entry.path)))
						return false;
				}
				return true;
			}
		};

		/// Mount, do something, unmount - so a failure part-way cannot leave a title
		/// mounted and the next attempt fighting it for the same path.
		template <typename Fn>
		bool WithMounted(TitleInfo* title, Fn&& body)
		{
			const std::string mountPath = TitleInfo::GetUniqueTempMountingPath();
			if (!title->Mount(mountPath, "", FSC_PRIORITY_BASE))
				return false;
			const bool ok = body(mountPath);
			title->Unmount(mountPath);
			return ok;
		}
	} // namespace

	bool ConvertTitles(std::span<TitleInfo* const> titles, WriteFn onWrite,
					   Progress& progress, std::string& errorOut)
	{
		if (titles.empty())
		{
			errorOut = "nothing to convert";
			progress.stage = Stage::Failed;
			return false;
		}

		Context ctx;
		ctx.write = std::move(onWrite);
		ctx.progress = &progress;
		ctx.writer = new ZArchiveWriter(&Context::NewOutputFileCb, &Context::WriteOutputDataCb, &ctx);
		std::unique_ptr<ZArchiveWriter> writerOwner(ctx.writer);

		// Two passes. The first exists only so a progress bar has a denominator; without
		// it the user gets a spinner for what can be twenty minutes.
		progress.stage = Stage::CollectingFiles;
		progress.totalFileCount = 0;
		progress.currentFileIndex = 0;
		for (TitleInfo* title : titles)
		{
			if (!WithMounted(title, [&](const std::string& mount) { return ctx.CountFiles(mount); }))
			{
				errorOut = progress.cancelRequested ? "cancelled" : "could not read the source";
				progress.stage = progress.cancelRequested ? Stage::Cancelled : Stage::Failed;
				return false;
			}
		}

		progress.stage = Stage::Compressing;
		for (TitleInfo* title : titles)
		{
			const std::string root = fmt::format("{:016x}_v{}/", (uint64)title->GetAppTitleId(), title->GetAppTitleVersion());
			if (!WithMounted(title, [&](const std::string& mount) { return ctx.AddFiles(root, mount); }))
			{
				errorOut = progress.cancelRequested ? "cancelled" : "could not read the source";
				progress.stage = progress.cancelRequested ? Stage::Cancelled : Stage::Failed;
				return false;
			}
		}

		progress.stage = Stage::Finalizing;
		ctx.writer->Finalize();

		if (ctx.writeFailed)
		{
			errorOut = "the output could not be written in full - check free space";
			progress.stage = Stage::Failed;
			return false;
		}

		progress.stage = Stage::Done;
		return true;
	}

	bool ConvertTitlesToFile(std::span<TitleInfo* const> titles, const fs::path& outputPath,
							 Progress& progress, std::string& errorOut)
	{
		std::ofstream out(outputPath, std::ios::binary | std::ios::trunc);
		if (!out.is_open())
		{
			errorOut = "could not create the output file";
			progress.stage = Stage::Failed;
			return false;
		}

		const bool ok = ConvertTitles(titles, [&out](const void* data, size_t length) {
			out.write(static_cast<const char*>(data), (std::streamsize)length);
			return out.good();
		}, progress, errorOut);

		out.flush();
		const bool streamOk = out.good();
		out.close();

		if (!ok || !streamOk)
		{
			// Never leave a half-written archive where a library scan will find it and
			// offer it as a game.
			std::error_code ec;
			fs::remove(outputPath, ec);
			if (ok && !streamOk)
			{
				errorOut = "the output could not be flushed - check free space";
				progress.stage = Stage::Failed;
			}
			return false;
		}
		return true;
	}

	bool VerifyFile(const fs::path& path, TitleId expectedTitleId, std::string& errorOut)
	{
		TitleInfo info{path};
		if (!info.IsValid())
		{
			errorOut = "the converted file did not open as a title";
			return false;
		}
		if (expectedTitleId != 0 && info.GetAppTitleId() != expectedTitleId)
		{
			errorOut = "the converted file contains a different title than the source";
			return false;
		}
		// Mounting is what proves the archive index and the file data agree - opening
		// only proves the header does.
		const std::string mountPath = TitleInfo::GetUniqueTempMountingPath();
		if (!info.Mount(mountPath, "", FSC_PRIORITY_BASE))
		{
			errorOut = "the converted file could not be mounted";
			return false;
		}
		sint32 status;
		std::unique_ptr<FSCVirtualFile> meta(
			fsc_open(fmt::format("{}meta/meta.xml", mountPath).c_str(),
					 FSC_ACCESS_FLAG::OPEN_FILE | FSC_ACCESS_FLAG::READ_PERMISSION, &status));
		const bool haveMeta = (bool)meta;
		meta.reset();
		info.Unmount(mountPath);
		if (!haveMeta)
		{
			errorOut = "the converted file is missing meta/meta.xml";
			return false;
		}
		return true;
	}

	std::string SuggestFileName(TitleInfo* base, TitleInfo* update, TitleInfo* aoc)
	{
		TitleInfo* naming = base ? base : (update ? update : aoc);
		if (!naming || !naming->IsValid())
			return "converted.wua";
		std::string name = naming->GetMetaTitleName();
		if (name.empty())
			name = fmt::format("{:016x}", (uint64)naming->GetAppTitleId());
		for (char& c : name)
		{
			if (c == '/' || c == '\\' || c == ':')
				c = '_';
		}
		const uint16 version = update && update->IsValid() ? update->GetAppTitleVersion() : naming->GetAppTitleVersion();
		return fmt::format("{} (v{}).wua", name, version);
	}
} // namespace WuaWriter
