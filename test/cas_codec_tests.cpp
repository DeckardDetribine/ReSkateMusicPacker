// SPDX-FileCopyrightText: 2026 DeckardDetribine and the ReSkateMusicPacker contributors
// SPDX-License-Identifier: GPL-3.0-only
// The Oodle path off Windows: CAS blocks compressed with Kraken, Selkie and Leviathan go through
// decode_cas() and the open-source ooz decoder, and encode_cas() writes raw blocks that read back.
// The fixtures are this repository's LICENSE (35149 bytes), compressed by ooz's own encoder, so the
// test needs neither the game nor its DLL. Usage: ReSkateCasCodecTests <test/fixtures folder>
#include "Engine/Resource/cas_codec.h"
#include "sha.h"
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

namespace {
namespace fs = std::filesystem;
namespace fb = dingosdk::frostbite;
int failures = 0;
void check(bool ok, const std::string& what) {
    if (!ok) { std::printf("FAIL: %s\n", what.c_str()); ++failures; }
}
constexpr std::size_t license_size = 35149;
constexpr char license_sha256[] = "3972dc9744f6499f0f9b2dbf76696f2ae7ad8af9b23dde66d6af86c9dfb36986";

std::vector<std::byte> slurp(const fs::path& file) {
    std::ifstream in(file, std::ios::binary);
    const std::vector<char> raw((std::istreambuf_iterator<char>(in)), {});
    std::vector<std::byte> bytes(raw.size());
    for (std::size_t i = 0; i < raw.size(); ++i) bytes[i] = static_cast<std::byte>(raw[i]);
    return bytes;
}
// One CAS block as the game stores it: decoded size (BE), codec, 0x70 | encoded size bits 16-19,
// encoded size bits 0-15 (BE), then the payload.
std::vector<std::byte> cas_block(std::uint8_t codec, std::size_t decoded, const std::vector<std::byte>& payload) {
    const auto size = payload.size();
    std::vector<std::byte> block{
        std::byte(decoded >> 24), std::byte(decoded >> 16), std::byte(decoded >> 8), std::byte(decoded),
        std::byte(codec), std::byte(0x70 | ((size >> 16) & 0x0F)), std::byte(size >> 8), std::byte(size)};
    block.insert(block.end(), payload.begin(), payload.end());
    return block;
}
std::string sha256(const std::vector<std::byte>& bytes) {
    music::Sha256 hash;
    hash.update(bytes);
    return music::hex_digest(hash.finish());
}
} // namespace

int main(int argc, char** argv) try {
    const fs::path fixtures = argc > 1 ? argv[1] : "test/fixtures";
    const struct { const char* file; std::uint8_t codec; } cases[]{
        {"license.kraken", 0x11}, {"license.selkie", 0x15}, {"license.leviathan", 0x19}};
    for (const auto& item : cases) {
        const auto payload = slurp(fixtures / item.file);
        check(!payload.empty(), std::string(item.file) + " is missing");
        if (payload.empty()) continue;
        try {
            const auto decoded = fb::decode_cas(cas_block(item.codec, license_size, payload));
            check(decoded.size() == license_size && sha256(decoded) == license_sha256,
                  std::string(item.file) + " decodes to the original bytes");
            // Two blocks back to back, as a multi-block chunk is stored.
            auto twice = cas_block(item.codec, license_size, payload);
            const auto again = cas_block(item.codec, license_size, payload);
            twice.insert(twice.end(), again.begin(), again.end());
            check(fb::decode_cas(twice).size() == 2 * license_size, std::string(item.file) + " decodes twice in a row");
        } catch (const std::exception& error) {
            check(false, std::string(item.file) + ": " + error.what());
        }
    }
    try {
        auto truncated = slurp(fixtures / "license.kraken");
        truncated.resize(truncated.size() / 2);
        (void)fb::decode_cas(cas_block(0x11, license_size, truncated));
        check(false, "a truncated Kraken block is rejected");
    } catch (const std::runtime_error&) {}

    // Without an Oodle encoder, asking for Kraken stores every block raw; those read back exactly.
    std::vector<std::byte> data(200000);
    for (std::size_t i = 0; i < data.size(); ++i) data[i] = static_cast<std::byte>((i * 7) ^ (i >> 5));
    const auto encoded = fb::encode_cas(data, {.compression = fb::CasCompression::kraken});
    check(encoded.size() == data.size() + 8 * 4, "encode_cas writes four raw 64 KiB blocks");
    check(fb::decode_cas(encoded) == data, "raw blocks decode to the input");

    std::printf(failures ? "%d failure(s)\n" : "all passed\n", failures);
    return failures ? 1 : 0;
} catch (const std::exception& error) {
    std::printf("error: %s\n", error.what());
    return 1;
}
