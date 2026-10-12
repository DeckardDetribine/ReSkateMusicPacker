// SPDX-FileCopyrightText: 2026 DeckardDetribine and the ReSkateMusicPacker contributors
// SPDX-License-Identifier: GPL-3.0-only
// Offline tests for ffmpeg_fetch: the SHA-256 check, zip extraction from memory, and the whole
// download -> verify -> extract path using a local fixture archive instead of the network.
#include "ffmpeg_fetch.h"
#include "miniz.h"
#include "platform.h"
#include <atomic>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {
namespace fs = std::filesystem;
int failures = 0;
void check(bool ok, const char* what) {
    if (!ok) { std::printf("FAIL: %s\n", what); ++failures; }
}
using platform::narrow;
const std::string ffmpeg_exe = std::string("ffmpeg") + platform::executable_suffix;
const std::string ffprobe_exe = std::string("ffprobe") + platform::executable_suffix;
std::string read_text(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}
void write_text(const fs::path& path, const std::string& text) {
    fs::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::binary);
    out.write(text.data(), static_cast<std::streamsize>(text.size()));
}
// A tiny zip in memory: "<top>/bin/ffmpeg.exe" and "<top>/bin/ffprobe.exe" with known contents.
void make_fixture_zip(const fs::path& path, const std::string& top) {
    mz_zip_archive zip{};
    if (!mz_zip_writer_init_heap(&zip, 0, 0)) throw std::runtime_error("fixture: init");
    struct End { mz_zip_archive* zip; ~End() { mz_zip_writer_end(zip); } } end{&zip};
    const std::vector<std::pair<std::string, std::string>> entries{
        {top + "/bin/" + ffmpeg_exe, "this is the fake ffmpeg binary"},
        {top + "/bin/" + ffprobe_exe, "this is the fake ffprobe binary"},
        {top + "/LICENSE.txt", "LGPL notice"},
    };
    for (const auto& [name, content] : entries)
        if (!mz_zip_writer_add_mem_ex(&zip, name.c_str(), content.data(), content.size(), nullptr, 0,
                                      static_cast<mz_uint>(MZ_DEFAULT_COMPRESSION), 0, 0))
            throw std::runtime_error("fixture: add " + name);
    void* buffer = nullptr;
    std::size_t size = 0;
    if (!mz_zip_writer_finalize_heap_archive(&zip, &buffer, &size) || !buffer)
        throw std::runtime_error("fixture: finalize");
    std::ofstream out(path, std::ios::binary);
    out.write(static_cast<const char*>(buffer), static_cast<std::streamsize>(size));
    mz_free(buffer);
}
} // namespace

int main() try {
    const auto root = fs::temp_directory_path() /
        ("FfmpegFetchTests-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    fs::create_directories(root);
    struct Cleanup { fs::path root; ~Cleanup() { std::error_code ec; fs::remove_all(root, ec); } } cleanup{root};

    const auto abc = root / L"abc.bin";
    write_text(abc, "abc");
    check(music::sha256_of_file(abc) == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad",
          "sha256_of_file matches the known test vector");
    check(music::verify_sha256(abc, "BA7816BF8F01CFEA414140DE5DAE2223B00361A396177A9CB410FF61F20015AD"),
          "verify_sha256 accepts the same digest in either case");
    check(!music::verify_sha256(abc, std::string(64, '0')), "verify_sha256 rejects a wrong digest");

    const auto fixture = root / L"fixture.zip";
    make_fixture_zip(fixture, "ffmpeg-test-build");
    const auto extracted = root / L"out" / ffmpeg_exe;
    music::extract_zip_member(fixture, "bin/" + ffmpeg_exe, extracted);
    check(read_text(extracted) == "this is the fake ffmpeg binary", "extract_zip_member finds a member under its top folder");
    try {
        music::extract_zip_member(fixture, "bin/ffplay.exe", root / L"out" / L"ffplay.exe");
        check(false, "a missing member throws");
    } catch (const std::runtime_error&) {}

    const auto install = root / L"install";
    const auto installed = music::ensure_ffmpeg(install, narrow(fixture.wstring()), music::sha256_of_file(fixture));
    check(installed == install && fs::exists(install / ffmpeg_exe) && fs::exists(install / ffprobe_exe),
          "ensure_ffmpeg downloads, verifies and extracts both binaries");
    check(read_text(install / ffprobe_exe) == "this is the fake ffprobe binary", "the extracted ffprobe is intact");
    check(!fs::exists(install / L"ffmpeg-download.zip"), "ensure_ffmpeg removes the archive it downloaded");

    const auto rejected = root / L"rejected";
    try {
        music::ensure_ffmpeg(rejected, narrow(fixture.wstring()), std::string(64, '0'));
        check(false, "a wrong checksum aborts");
    } catch (const std::runtime_error&) {}
    check(!fs::exists(rejected / ffmpeg_exe) && !fs::exists(rejected / ffprobe_exe),
          "a checksum mismatch extracts nothing");

    std::atomic<bool> cancel = true;
    try {
        music::download_to_file(narrow(fixture.wstring()), root / L"cancelled.zip", {}, &cancel);
        check(false, "a set cancel flag aborts the download");
    } catch (const music::Cancelled&) {}

    std::printf(failures ? "%d failure(s)\n" : "all passed\n", failures);
    return failures ? 1 : 0;
} catch (const std::exception& error) {
    std::printf("error: %s\n", error.what());
    return 1;
}
