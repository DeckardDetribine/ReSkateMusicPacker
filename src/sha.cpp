// SPDX-FileCopyrightText: 2026 DeckardDetribine and the ReSkateMusicPacker contributors
// SPDX-License-Identifier: GPL-3.0-only
#include "sha.h"
#include "Engine/Core/Platform/path_text.h"
#include <algorithm>
#include <cstring>
#include <fstream>
#include <stdexcept>
#include <vector>

namespace music {
namespace {

constexpr std::uint32_t rotl(std::uint32_t x, int n) { return (x << n) | (x >> (32 - n)); }
constexpr std::uint32_t rotr(std::uint32_t x, int n) { return (x >> n) | (x << (32 - n)); }

std::uint32_t load_be(const std::uint8_t* p) {
    return (std::uint32_t{p[0]} << 24) | (std::uint32_t{p[1]} << 16) | (std::uint32_t{p[2]} << 8) | p[3];
}

// The Merkle-Damgard framing both hashes share: buffer to 64-byte blocks, then pad with 0x80,
// zeros and the bit length (big endian).
template <typename Block>
void absorb(std::span<const std::byte> bytes, std::array<std::uint8_t, 64>& buffer, std::size_t& buffered,
            std::uint64_t& length, Block&& block) {
    const auto* data = reinterpret_cast<const std::uint8_t*>(bytes.data());
    auto size = bytes.size();
    length += size;
    if (buffered) {
        const auto take = std::min(size, buffer.size() - buffered);
        std::memcpy(buffer.data() + buffered, data, take);
        buffered += take;
        data += take;
        size -= take;
        if (buffered < buffer.size()) return;
        block(buffer.data());
        buffered = 0;
    }
    for (; size >= 64; data += 64, size -= 64) block(data);
    std::memcpy(buffer.data(), data, size);
    buffered = size;
}

template <typename Block>
void pad(std::array<std::uint8_t, 64>& buffer, std::size_t buffered, std::uint64_t length, Block&& block) {
    buffer[buffered++] = 0x80;
    if (buffered > 56) {
        std::memset(buffer.data() + buffered, 0, 64 - buffered);
        block(buffer.data());
        buffered = 0;
    }
    std::memset(buffer.data() + buffered, 0, 56 - buffered);
    const auto bits = length * 8;
    for (int i = 0; i < 8; ++i) buffer[56 + i] = static_cast<std::uint8_t>(bits >> (56 - 8 * i));
    block(buffer.data());
}

template <std::size_t N, std::size_t Words>
std::array<std::byte, N> digest(const std::array<std::uint32_t, Words>& state) {
    std::array<std::byte, N> out{};
    for (std::size_t i = 0; i < N; ++i)
        out[i] = static_cast<std::byte>(state[i / 4] >> (24 - 8 * (i % 4)));
    return out;
}

template <typename Hash>
auto hash_file(const std::filesystem::path& file) {
    std::ifstream in(file, std::ios::binary);
    if (!in) throw std::runtime_error("Cannot read " + dingosdk::path_utf8(file));
    Hash hash;
    std::vector<char> buffer(65536);
    while (in.read(buffer.data(), static_cast<std::streamsize>(buffer.size())) || in.gcount() > 0)
        hash.update(std::as_bytes(std::span(buffer.data(), static_cast<std::size_t>(in.gcount()))));
    return hash.finish();
}

constexpr std::array<std::uint32_t, 64> k256{
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};

} // namespace

Sha1::Sha1() : state_{0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0} {}

void Sha1::block(const std::uint8_t* data) {
    std::uint32_t w[80];
    for (int i = 0; i < 16; ++i) w[i] = load_be(data + 4 * i);
    for (int i = 16; i < 80; ++i) w[i] = rotl(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
    auto [a, b, c, d, e] = state_;
    for (int i = 0; i < 80; ++i) {
        std::uint32_t f, k;
        if (i < 20) { f = (b & c) | (~b & d); k = 0x5A827999; }
        else if (i < 40) { f = b ^ c ^ d; k = 0x6ED9EBA1; }
        else if (i < 60) { f = (b & c) | (b & d) | (c & d); k = 0x8F1BBCDC; }
        else { f = b ^ c ^ d; k = 0xCA62C1D6; }
        const auto t = rotl(a, 5) + f + e + k + w[i];
        e = d; d = c; c = rotl(b, 30); b = a; a = t;
    }
    state_[0] += a; state_[1] += b; state_[2] += c; state_[3] += d; state_[4] += e;
}

void Sha1::update(std::span<const std::byte> bytes) {
    absorb(bytes, buffer_, buffered_, length_, [this](const std::uint8_t* p) { block(p); });
}

std::array<std::byte, 20> Sha1::finish() {
    pad(buffer_, buffered_, length_, [this](const std::uint8_t* p) { block(p); });
    return digest<20>(state_);
}

Sha256::Sha256()
    : state_{0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19} {}

void Sha256::block(const std::uint8_t* data) {
    std::uint32_t w[64];
    for (int i = 0; i < 16; ++i) w[i] = load_be(data + 4 * i);
    for (int i = 16; i < 64; ++i) {
        const auto s0 = rotr(w[i - 15], 7) ^ rotr(w[i - 15], 18) ^ (w[i - 15] >> 3);
        const auto s1 = rotr(w[i - 2], 17) ^ rotr(w[i - 2], 19) ^ (w[i - 2] >> 10);
        w[i] = w[i - 16] + s0 + w[i - 7] + s1;
    }
    auto [a, b, c, d, e, f, g, h] = state_;
    for (int i = 0; i < 64; ++i) {
        const auto t1 = h + (rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25)) + ((e & f) ^ (~e & g)) + k256[i] + w[i];
        const auto t2 = (rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22)) + ((a & b) ^ (a & c) ^ (b & c));
        h = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2;
    }
    state_[0] += a; state_[1] += b; state_[2] += c; state_[3] += d;
    state_[4] += e; state_[5] += f; state_[6] += g; state_[7] += h;
}

void Sha256::update(std::span<const std::byte> bytes) {
    absorb(bytes, buffer_, buffered_, length_, [this](const std::uint8_t* p) { block(p); });
}

std::array<std::byte, 32> Sha256::finish() {
    pad(buffer_, buffered_, length_, [this](const std::uint8_t* p) { block(p); });
    return digest<32>(state_);
}

std::array<std::byte, 20> sha1_file(const std::filesystem::path& file) { return hash_file<Sha1>(file); }
std::array<std::byte, 32> sha256_file(const std::filesystem::path& file) { return hash_file<Sha256>(file); }

std::string hex_digest(std::span<const std::byte> bytes) {
    constexpr char digits[] = "0123456789abcdef";
    std::string hex;
    hex.reserve(bytes.size() * 2);
    for (const auto b : bytes) {
        hex += digits[std::to_integer<unsigned>(b) >> 4];
        hex += digits[std::to_integer<unsigned>(b) & 15];
    }
    return hex;
}

} // namespace music
