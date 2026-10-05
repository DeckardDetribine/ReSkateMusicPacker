// SPDX-FileCopyrightText: 2026 DeckardDetribine and the ReSkateMusicPacker contributors
// SPDX-License-Identifier: GPL-3.0-only
// Fetches and installs ffmpeg.exe + ffprobe.exe when they are missing. Windows-only: the download
// goes through WinHTTP (the system proxy), and the pinned archive is verified with SHA-256 before
// anything is extracted. Nothing here runs on its own; the GUI and CLI call it after an explicit click.
#pragma once
#include "packer.h"
#include <atomic>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <string>

namespace music {

// How far a download has come. `total` is 0 when the server did not report a length.
struct DownloadProgress {
    std::uint64_t received{}, total{};
};

// The pinned build: a dated, static, LGPL win64 asset from BtbN/FFmpeg-Builds that ships ffmpeg,
// ffprobe and libopus. FFMPEG_URL and FFMPEG_SHA256 (environment variables) override both, so link
// rot or a newer build can be worked around without a rebuild.
const char* ffmpeg_url();
const char* ffmpeg_sha256();

// Where a downloaded ffmpeg lands: `.\ffmpeg` beside the running exe when that folder is writable
// (portable install), otherwise `%LOCALAPPDATA%\ReSkateMusicPacker\ffmpeg`. Creates the folder.
std::filesystem::path default_install_dir();

// Lower-case hex SHA-256 of a file, streaming so a 160 MB archive does not sit in memory twice.
std::string sha256_of_file(const std::filesystem::path& file);

// True when `file`'s SHA-256 equals `expected` (case-insensitive). Throws only if the file cannot be
// read, so a mismatch is a plain `false`, not an exception.
bool verify_sha256(const std::filesystem::path& file, const std::string& expected);

// Copies `url` to `dest` (an existing file is replaced). `http(s)://` goes through WinHTTP, following
// redirects, with timeouts and a size cap; anything else is treated as a local path (or `file://...`),
// which keeps the tests offline and lets FFMPEG_URL point at a mirror. `progress` and `cancel` may be
// null. Throws std::runtime_error on any failure, or Cancelled when `cancel` was set.
void download_to_file(const std::string& url, const std::filesystem::path& dest,
                      const std::function<void(const DownloadProgress&)>& progress = {},
                      const std::atomic<bool>* cancel = nullptr);

// Extracts one zip member into `dest`. `member` is a forward-slash path, matched exactly or as the
// suffix of an entry (so "bin/ffmpeg.exe" finds "ffmpeg-.../bin/ffmpeg.exe"). The archive is read
// into memory: miniz is built without its stdio APIs. Throws when the archive or member is missing.
void extract_zip_member(const std::filesystem::path& archive, const std::string& member,
                        const std::filesystem::path& dest);

// Downloads `url`, verifies its SHA-256 and extracts ffmpeg.exe + ffprobe.exe into `install_dir`,
// returning that folder. The URL and hash are parameters so the core has no hidden network use and
// tests can pass a local fixture.
std::filesystem::path ensure_ffmpeg(const std::filesystem::path& install_dir,
                                    const std::string& url = ffmpeg_url(),
                                    const std::string& sha256 = ffmpeg_sha256(),
                                    const std::function<void(const DownloadProgress&)>& progress = {},
                                    const std::atomic<bool>* cancel = nullptr);

} // namespace music
