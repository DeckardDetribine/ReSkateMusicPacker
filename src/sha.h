// SPDX-FileCopyrightText: 2026 DeckardDetribine and the ReSkateMusicPacker contributors
// SPDX-License-Identifier: GPL-3.0-only
// SHA-1 (encode cache keys, Frostbite chunk ids) and SHA-256 (Skate.exe stamp, the ffmpeg download
// check), in plain C++ so every platform hashes the same way. FIPS 180-4.
#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>
#include <string>

namespace music {

class Sha1 {
public:
    Sha1();
    void update(std::span<const std::byte> bytes);
    std::array<std::byte, 20> finish();

private:
    void block(const std::uint8_t* data);
    std::array<std::uint32_t, 5> state_;
    std::array<std::uint8_t, 64> buffer_{};
    std::size_t buffered_{};
    std::uint64_t length_{};
};

class Sha256 {
public:
    Sha256();
    void update(std::span<const std::byte> bytes);
    std::array<std::byte, 32> finish();

private:
    void block(const std::uint8_t* data);
    std::array<std::uint32_t, 8> state_;
    std::array<std::uint8_t, 64> buffer_{};
    std::size_t buffered_{};
    std::uint64_t length_{};
};

// Streams a file through the hash, so a 160 MB archive is never held in memory. Throws when the
// file cannot be read.
std::array<std::byte, 20> sha1_file(const std::filesystem::path& file);
std::array<std::byte, 32> sha256_file(const std::filesystem::path& file);

std::string hex_digest(std::span<const std::byte> bytes);

} // namespace music
