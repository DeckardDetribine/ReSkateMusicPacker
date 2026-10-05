// Downloads the pinned ffmpeg build and unpacks just the two binaries the packer needs. See
// ffmpeg_fetch.h for the contract. WinHTTP follows the redirect from github.com to its asset CDN.
#include "ffmpeg_fetch.h"
#include "miniz.h"
#include <Windows.h>
#include <bcrypt.h>
#include <shlobj.h>
#include <winhttp.h>
#include <array>
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cwchar>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <vector>

namespace music {
namespace fs = std::filesystem;

namespace {

// The pinned asset: BtbN/FFmpeg-Builds autobuild-2026-09-30-13-08, win64 LGPL (static; ffmpeg,
// ffprobe and libopus). The dated month-end build is retained far longer than the daily ones.
constexpr char default_url[] =
    "https://github.com/BtbN/FFmpeg-Builds/releases/download/autobuild-2026-09-30-13-08/"
    "ffmpeg-n8.1.3-9-g29e619e767-win64-lgpl-8.1.zip";
constexpr char default_sha256[] = "4a7642b2264c03e8a0ce8a3825b933ee5580656f45695a086fe7e294045ffc0a";
constexpr std::uint64_t max_download = 400ull * 1024 * 1024;

std::wstring widen(const std::string& text) {
    std::wstring wide(MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), wide.data(), static_cast<int>(wide.size()));
    return wide;
}
std::string narrow(const std::wstring& text) {
    std::string utf8(WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr), '\0');
    WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), utf8.data(), static_cast<int>(utf8.size()), nullptr, nullptr);
    return utf8;
}

bool writable_directory(const fs::path& dir) {
    std::error_code error;
    fs::create_directories(dir, error);
    if (error) return false;
    const auto probe = dir / L".reskate-write-test";
    {
        std::ofstream out(probe, std::ios::binary);
        if (!out) return false;
        out.put('\0');
    }
    fs::remove(probe, error);
    return !error;
}
fs::path executable_directory() {
    std::wstring buffer(MAX_PATH, L'\0');
    const DWORD length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (length == 0 || length >= buffer.size()) return {};
    buffer.resize(length);
    return fs::path(buffer).parent_path();
}

// Copies a local path or file:// URL, so the tests stay offline and FFMPEG_URL can point at a mirror.
void download_local(const std::string& source, const fs::path& dest,
                    const std::function<void(const DownloadProgress&)>& progress, const std::atomic<bool>* cancel) {
    std::string text = source.rfind("file://", 0) == 0 ? source.substr(7) : source;
    const auto path = fs::path(widen(text));
    std::error_code error;
    const auto total = fs::file_size(path, error);
    if (error) throw std::runtime_error("Cannot read " + text);
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("Cannot read " + text);
    std::ofstream out(dest, std::ios::binary);
    if (!out) throw std::runtime_error("Cannot write " + narrow(dest.wstring()));
    std::array<char, 65536> buffer{};
    std::uint64_t received = 0;
    while (in.read(buffer.data(), static_cast<std::streamsize>(buffer.size())) || in.gcount() > 0) {
        if (cancel && *cancel) throw Cancelled();
        const auto count = in.gcount();
        out.write(buffer.data(), count);
        if (!out) throw std::runtime_error("Cannot write " + narrow(dest.wstring()));
        received += static_cast<std::uint64_t>(count);
        if (progress) progress(DownloadProgress{received, static_cast<std::uint64_t>(total)});
    }
}

struct InternetHandle {
    HINTERNET handle{};
    ~InternetHandle() { if (handle) WinHttpCloseHandle(handle); }
    InternetHandle() = default;
    InternetHandle(const InternetHandle&) = delete;
    InternetHandle& operator=(const InternetHandle&) = delete;
};

void download_http(const std::string& url, const fs::path& dest,
                   const std::function<void(const DownloadProgress&)>& progress, const std::atomic<bool>* cancel) {
    std::wstring wide = widen(url);
    URL_COMPONENTS parts{};
    parts.dwStructSize = sizeof(parts);
    parts.dwSchemeLength = static_cast<DWORD>(-1);
    parts.dwHostNameLength = static_cast<DWORD>(-1);
    parts.dwUrlPathLength = static_cast<DWORD>(-1);
    parts.dwExtraInfoLength = static_cast<DWORD>(-1);
    if (!WinHttpCrackUrl(wide.c_str(), static_cast<DWORD>(wide.size()), 0, &parts))
        throw std::runtime_error("The ffmpeg download address is not a valid URL.");
    const std::wstring scheme(parts.lpszScheme, parts.dwSchemeLength);
    const std::wstring host(parts.lpszHostName, parts.dwHostNameLength);
    std::wstring target(parts.lpszUrlPath, parts.dwUrlPathLength);
    if (parts.dwExtraInfoLength) target.append(parts.lpszExtraInfo, parts.dwExtraInfoLength);
    const bool secure = _wcsicmp(scheme.c_str(), L"https") == 0;

    InternetHandle session;
    session.handle = WinHttpOpen(L"ReSkateMusicPacker/0.1", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                 WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!session.handle) throw std::runtime_error("Cannot start an HTTP session for the ffmpeg download.");
    WinHttpSetTimeouts(session.handle, 20000, 20000, 30000, 60000);

    InternetHandle connection;
    connection.handle = WinHttpConnect(session.handle, host.c_str(), parts.nPort, 0);
    if (!connection.handle) throw std::runtime_error("Cannot connect to " + narrow(host) + ".");
    InternetHandle request;
    request.handle = WinHttpOpenRequest(connection.handle, L"GET", target.c_str(), nullptr, WINHTTP_NO_REFERER,
                                        WINHTTP_DEFAULT_ACCEPT_TYPES, secure ? WINHTTP_FLAG_SECURE : 0);
    if (!request.handle) throw std::runtime_error("Cannot open the ffmpeg download request.");
    DWORD redirect = WINHTTP_OPTION_REDIRECT_POLICY_ALWAYS;
    WinHttpSetOption(request.handle, WINHTTP_OPTION_REDIRECT_POLICY, &redirect, sizeof(redirect));
    DWORD maxRedirects = 10;
    WinHttpSetOption(request.handle, WINHTTP_OPTION_MAX_HTTP_AUTOMATIC_REDIRECTS, &maxRedirects, sizeof(maxRedirects));
    DWORD protocols = WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2;
    WinHttpSetOption(request.handle, WINHTTP_OPTION_SECURE_PROTOCOLS, &protocols, sizeof(protocols));

    if (!WinHttpSendRequest(request.handle, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) ||
        !WinHttpReceiveResponse(request.handle, nullptr))
        throw std::runtime_error("The ffmpeg download could not start (offline, blocked or the link moved).");
    DWORD status = 0, size = sizeof(status);
    if (!WinHttpQueryHeaders(request.handle, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                             WINHTTP_HEADER_NAME_BY_INDEX, &status, &size, WINHTTP_NO_HEADER_INDEX))
        throw std::runtime_error("The ffmpeg download gave no response.");
    if (status != 200) throw std::runtime_error("The ffmpeg download failed (HTTP " + std::to_string(status) + ").");
    std::uint64_t total = 0;
    wchar_t length[32]{};
    DWORD lengthBytes = sizeof(length);
    if (WinHttpQueryHeaders(request.handle, WINHTTP_QUERY_CONTENT_LENGTH, WINHTTP_HEADER_NAME_BY_INDEX,
                            length, &lengthBytes, WINHTTP_NO_HEADER_INDEX))
        total = std::wcstoull(length, nullptr, 10);
    if (total > max_download) throw std::runtime_error("The ffmpeg download is larger than expected.");

    std::ofstream out(dest, std::ios::binary);
    if (!out) throw std::runtime_error("Cannot write " + narrow(dest.wstring()));
    std::uint64_t received = 0;
    std::vector<char> buffer;
    for (;;) {
        if (cancel && *cancel) throw Cancelled();
        DWORD available = 0;
        if (!WinHttpQueryDataAvailable(request.handle, &available))
            throw std::runtime_error("The ffmpeg download was interrupted.");
        if (available == 0) break;
        buffer.resize(available);
        DWORD read = 0;
        if (!WinHttpReadData(request.handle, buffer.data(), available, &read))
            throw std::runtime_error("The ffmpeg download was interrupted.");
        if (read == 0) break;
        out.write(buffer.data(), static_cast<std::streamsize>(read));
        if (!out) throw std::runtime_error("Cannot write " + narrow(dest.wstring()));
        received += read;
        if (received > max_download) throw std::runtime_error("The ffmpeg download is larger than expected.");
        if (progress) progress(DownloadProgress{received, total});
    }
    if (total && received != total) throw std::runtime_error("The ffmpeg download stopped before it was complete.");
}

// A member by exact name, or by path suffix so "bin/ffmpeg.exe" finds "<top>/bin/ffmpeg.exe".
// Backslashes are treated as separators too: a few zip writers still store Windows-style paths.
int find_member(mz_zip_archive* zip, const std::string& member) {
    const std::string suffix = "/" + member;
    const mz_uint count = mz_zip_reader_get_num_files(zip);
    for (mz_uint i = 0; i < count; ++i) {
        mz_zip_archive_file_stat stat{};
        if (!mz_zip_reader_file_stat(zip, i, &stat)) continue;
        std::string name = stat.m_filename;
        std::replace(name.begin(), name.end(), '\\', '/');
        if (name == member || (name.size() >= suffix.size() && name.compare(name.size() - suffix.size(), suffix.size(), suffix) == 0))
            return static_cast<int>(i);
    }
    return -1;
}

} // namespace

const char* ffmpeg_url() {
    const DWORD needed = GetEnvironmentVariableW(L"FFMPEG_URL", nullptr, 0);
    static const std::string value = [needed] {
        if (needed == 0) return std::string(default_url);
        std::wstring buffer(needed, L'\0');
        buffer.resize(GetEnvironmentVariableW(L"FFMPEG_URL", buffer.data(), needed));
        return narrow(buffer);
    }();
    return value.c_str();
}
const char* ffmpeg_sha256() {
    const DWORD needed = GetEnvironmentVariableW(L"FFMPEG_SHA256", nullptr, 0);
    static const std::string value = [needed] {
        if (needed == 0) return std::string(default_sha256);
        std::wstring buffer(needed, L'\0');
        buffer.resize(GetEnvironmentVariableW(L"FFMPEG_SHA256", buffer.data(), needed));
        return narrow(buffer);
    }();
    return value.c_str();
}

std::filesystem::path default_install_dir() {
    if (const auto exe = executable_directory(); !exe.empty()) {
        const auto portable = exe / L"ffmpeg";
        if (writable_directory(portable)) return portable;
    }
    PWSTR local{};
    fs::path root;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &local))) root = local;
    CoTaskMemFree(local);
    const auto fallback = (root.empty() ? fs::temp_directory_path() : root) / L"ReSkateMusicPacker" / L"ffmpeg";
    std::error_code error;
    fs::create_directories(fallback, error);
    if (error) throw std::runtime_error("Cannot create " + narrow(fallback.wstring()));
    return fallback;
}

std::string sha256_of_file(const std::filesystem::path& file) {
    BCRYPT_ALG_HANDLE algorithm{};
    if (BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0) < 0)
        throw std::runtime_error("Cannot open the SHA-256 provider.");
    BCRYPT_HASH_HANDLE handle{};
    if (BCryptCreateHash(algorithm, &handle, nullptr, 0, nullptr, 0, 0) < 0) {
        BCryptCloseAlgorithmProvider(algorithm, 0);
        throw std::runtime_error("Cannot start a SHA-256 hash.");
    }
    std::ifstream in(file, std::ios::binary);
    if (!in) {
        BCryptDestroyHash(handle);
        BCryptCloseAlgorithmProvider(algorithm, 0);
        throw std::runtime_error("Cannot read " + narrow(file.wstring()));
    }
    std::array<char, 65536> buffer{};
    while (in.read(buffer.data(), static_cast<std::streamsize>(buffer.size())) || in.gcount() > 0)
        BCryptHashData(handle, reinterpret_cast<PUCHAR>(buffer.data()), static_cast<ULONG>(in.gcount()), 0);
    std::array<unsigned char, 32> digest{};
    BCryptFinishHash(handle, digest.data(), static_cast<ULONG>(digest.size()), 0);
    BCryptDestroyHash(handle);
    BCryptCloseAlgorithmProvider(algorithm, 0);
    char hex[65];
    for (std::size_t i = 0; i < digest.size(); ++i) std::snprintf(hex + i * 2, 3, "%02x", digest[i]);
    return std::string(hex, 64);
}

bool verify_sha256(const std::filesystem::path& file, const std::string& expected) {
    const auto actual = sha256_of_file(file);
    if (actual.size() != expected.size()) return false;
    for (std::size_t i = 0; i < actual.size(); ++i)
        if (std::tolower(static_cast<unsigned char>(actual[i])) != std::tolower(static_cast<unsigned char>(expected[i])))
            return false;
    return true;
}

void download_to_file(const std::string& url, const std::filesystem::path& dest,
                      const std::function<void(const DownloadProgress&)>& progress, const std::atomic<bool>* cancel) {
    if (cancel && *cancel) throw Cancelled();
    if (url.rfind("http://", 0) == 0 || url.rfind("https://", 0) == 0) download_http(url, dest, progress, cancel);
    else download_local(url, dest, progress, cancel);
}

void extract_zip_member(const std::filesystem::path& archive, const std::string& member,
                        const std::filesystem::path& dest) {
    std::ifstream in(archive, std::ios::binary);
    if (!in) throw std::runtime_error("Cannot read " + narrow(archive.wstring()));
    std::vector<char> blob((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    if (blob.empty()) throw std::runtime_error("The downloaded ffmpeg archive is empty.");
    mz_zip_archive zip{};
    if (!mz_zip_reader_init_mem(&zip, blob.data(), blob.size(), 0))
        throw std::runtime_error("The downloaded ffmpeg archive is not a zip.");
    struct Guard { mz_zip_archive* zip; ~Guard() { mz_zip_reader_end(zip); } } guard{&zip};
    const int index = find_member(&zip, member);
    if (index < 0) throw std::runtime_error("The ffmpeg archive has no " + member + ".");
    mz_zip_archive_file_stat stat{};
    if (!mz_zip_reader_file_stat(&zip, static_cast<mz_uint>(index), &stat))
        throw std::runtime_error("Cannot read " + member + " from the ffmpeg archive.");
    if (stat.m_uncomp_size == 0 || stat.m_uncomp_size > max_download)
        throw std::runtime_error("The ffmpeg archive's " + member + " has an unexpected size.");
    std::vector<std::byte> data(static_cast<std::size_t>(stat.m_uncomp_size));
    if (!mz_zip_reader_extract_to_mem(&zip, static_cast<mz_uint>(index), data.data(), data.size(), 0))
        throw std::runtime_error("Cannot extract " + member + " from the ffmpeg archive.");
    if (!dest.parent_path().empty()) fs::create_directories(dest.parent_path());
    std::ofstream out(dest, std::ios::binary);
    if (!out) throw std::runtime_error("Cannot write " + narrow(dest.wstring()));
    out.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
    if (!out) throw std::runtime_error("Cannot write " + narrow(dest.wstring()));
}

std::filesystem::path ensure_ffmpeg(const std::filesystem::path& install_dir, const std::string& url,
                                    const std::string& sha256, const std::function<void(const DownloadProgress&)>& progress,
                                    const std::atomic<bool>* cancel) {
    std::error_code error;
    fs::create_directories(install_dir, error);
    if (error) throw std::runtime_error("Cannot create " + narrow(install_dir.wstring()));
    const auto archive = install_dir / L"ffmpeg-download.zip";
    struct Cleanup { fs::path path; ~Cleanup() { std::error_code ignored; fs::remove(path, ignored); } } cleanup{archive};
    download_to_file(url, archive, progress, cancel);
    if (!verify_sha256(archive, sha256))
        throw std::runtime_error("The ffmpeg download did not match its expected checksum and was rejected.");
    extract_zip_member(archive, "bin/ffmpeg.exe", install_dir / L"ffmpeg.exe");
    extract_zip_member(archive, "bin/ffprobe.exe", install_dir / L"ffprobe.exe");
    return install_dir;
}

} // namespace music
