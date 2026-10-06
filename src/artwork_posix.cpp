// SPDX-FileCopyrightText: 2026 DeckardDetribine and the ReSkateMusicPacker contributors
// SPDX-License-Identifier: GPL-3.0-only
// The generated playlist cover off Windows: the same layout as artwork_win32.cpp (gradient, accent
// circle, "PLAYLIST" badge, the name wrapped and centred, shrinking until it fits), drawn with
// stb_truetype from a system font and written as a PNG by miniz. Characters the main font lacks
// (CJK on a Latin font) come from fallback fonts.
#include "packer.h"
#include "platform.h"
#include "miniz.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iterator>
#include <memory>
#include <mutex>
#include <optional>

// Third-party, compiled here as static functions: silence its warnings about the parts not used.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_STATIC
#include "imstb_truetype.h"
#pragma GCC diagnostic pop

namespace music {
namespace {
namespace fs = std::filesystem;

constexpr int size_px = 512;
void require(bool ok) { if (!ok) throw std::runtime_error("Could not generate the playlist cover"); }

struct Font {
    std::vector<unsigned char> data;
    stbtt_fontinfo info{};
};

#ifndef __APPLE__
// fc-match's answer for a pattern, when fontconfig is installed (most Linux desktops).
std::optional<fs::path> fontconfig(const std::string& pattern) {
    std::string file;
    if (platform::run_process({"fc-match", "-f", "%{file}", pattern}, &file) != 0 || file.empty()) return std::nullopt;
    return fs::path(file);
}
#endif

std::unique_ptr<Font> load(const fs::path& file) {
    std::error_code error;
    if (file.empty() || !fs::is_regular_file(file, error)) return nullptr;
    std::ifstream in(file, std::ios::binary);
    auto font = std::make_unique<Font>();
    font->data.assign(std::istreambuf_iterator<char>(in), {});
    const int offset = stbtt_GetFontOffsetForIndex(font->data.data(), 0); // the first face of a .ttc
    if (offset < 0 || !stbtt_InitFont(&font->info, font->data.data(), offset)) return nullptr;
    return font;
}

// A bold sans first, then wide-coverage fonts for whatever it is missing. Loaded once.
const std::vector<std::unique_ptr<Font>>& fonts() {
    static std::once_flag once;
    static std::vector<std::unique_ptr<Font>> loaded;
    std::call_once(once, [] {
        std::vector<fs::path> candidates;
#ifdef __APPLE__
        candidates = {"/System/Library/Fonts/Supplemental/Arial Bold.ttf", "/Library/Fonts/Arial Bold.ttf",
                      "/System/Library/Fonts/Helvetica.ttc", "/System/Library/Fonts/Supplemental/Arial Unicode.ttf",
                      "/Library/Fonts/Arial Unicode.ttf", "/System/Library/Fonts/Hiragino Sans GB.ttc"};
#else
        for (const char* pattern : {"sans-serif:bold", "sans-serif:bold:lang=ja", "sans-serif:lang=zh-cn", "sans-serif:lang=ko"})
            if (auto file = fontconfig(pattern)) candidates.push_back(*file);
        for (const char* file : {"/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", "/usr/share/fonts/TTF/DejaVuSans-Bold.ttf",
                                 "/usr/share/fonts/dejavu-sans-fonts/DejaVuSans-Bold.ttf",
                                 "/usr/share/fonts/truetype/liberation/LiberationSans-Bold.ttf",
                                 "/usr/share/fonts/liberation-sans/LiberationSans-Bold.ttf",
                                 "/usr/share/fonts/noto/NotoSans-Bold.ttf", "/usr/share/fonts/truetype/noto/NotoSans-Bold.ttf"})
            candidates.emplace_back(file);
#endif
        std::vector<fs::path> seen;
        for (const auto& file : candidates) {
            if (std::find(seen.begin(), seen.end(), file) != seen.end()) continue;
            seen.push_back(file);
            if (auto font = load(file)) loaded.push_back(std::move(font));
        }
    });
    return loaded;
}

struct Glyph {
    const Font* font;
    int index;
};

Glyph glyph_for(char32_t code) {
    const auto& all = fonts();
    for (const auto& font : all)
        if (const int index = stbtt_FindGlyphIndex(&font->info, static_cast<int>(code)); index)
            return {font.get(), index};
    return {all.front().get(), 0}; // the main font's missing-glyph box
}

// Text at one pixel size: the pen advance of a run and drawing it.
class Writer {
public:
    explicit Writer(float pixels) : pixels_(pixels) {
        const auto& main = fonts().front()->info;
        int ascent, descent, gap;
        stbtt_GetFontVMetrics(&main, &ascent, &descent, &gap);
        const float scale = stbtt_ScaleForMappingEmToPixels(&main, pixels);
        ascent_ = static_cast<float>(ascent) * scale;
        line_ = static_cast<float>(ascent - descent) * scale; // GDI's tmHeight
    }
    float line_height() const { return line_; }

    float width(const std::u32string& text) const {
        float x = 0;
        walk(text, [&](const Glyph&, float, float advance) { x += advance; });
        return x;
    }

    void draw(std::vector<unsigned char>& rgb, const std::u32string& text, float left, float top) const {
        float x = left;
        walk(text, [&](const Glyph& glyph, float scale, float advance) {
            int x0, y0, x1, y1;
            const float shift = x - std::floor(x);
            stbtt_GetGlyphBitmapBoxSubpixel(&glyph.font->info, glyph.index, scale, scale, shift, 0, &x0, &y0, &x1, &y1);
            const int w = x1 - x0, h = y1 - y0;
            if (w > 0 && h > 0) {
                std::vector<unsigned char> coverage(static_cast<std::size_t>(w * h));
                stbtt_MakeGlyphBitmapSubpixel(&glyph.font->info, coverage.data(), w, h, w, scale, scale, shift, 0, glyph.index);
                const int ox = static_cast<int>(std::floor(x)) + x0;
                const int oy = static_cast<int>(std::lround(top + ascent_)) + y0;
                for (int row = 0; row < h; ++row) for (int col = 0; col < w; ++col) {
                    const int px = ox + col, py = oy + row;
                    if (px < 0 || py < 0 || px >= size_px || py >= size_px) continue;
                    const unsigned alpha = coverage[static_cast<std::size_t>(row * w + col)];
                    auto* pixel = &rgb[static_cast<std::size_t>(py * size_px + px) * 3];
                    for (int c = 0; c < 3; ++c) pixel[c] = static_cast<unsigned char>((pixel[c] * (255 - alpha) + 255 * alpha) / 255);
                }
            }
            x += advance;
        });
    }

private:
    template <typename Visit> void walk(const std::u32string& text, Visit&& visit) const {
        Glyph previous{nullptr, 0};
        for (const auto code : text) {
            const auto glyph = glyph_for(code);
            const float scale = stbtt_ScaleForMappingEmToPixels(&glyph.font->info, pixels_);
            int advance, bearing;
            stbtt_GetGlyphHMetrics(&glyph.font->info, glyph.index, &advance, &bearing);
            float kern = 0;
            if (previous.font == glyph.font)
                kern = static_cast<float>(stbtt_GetGlyphKernAdvance(&glyph.font->info, previous.index, glyph.index)) * scale;
            visit(glyph, scale, static_cast<float>(advance) * scale + kern);
            previous = glyph;
        }
    }
    float pixels_, ascent_{}, line_{};
};

// Greedy wrap like artwork_win32.cpp: as many characters as fit, broken at the last space when
// there is one, else mid-word.
std::vector<std::u32string> wrapped(const Writer& writer, std::u32string text, float width) {
    std::vector<std::u32string> lines;
    while (!text.empty()) {
        std::size_t count = 0, space = 0;
        for (std::size_t i = 1; i <= text.size(); ++i) {
            if (writer.width(text.substr(0, i)) > width) break;
            count = i;
            if (text[i - 1] == U' ') space = i;
        }
        if (count < text.size() && space) count = space;
        if (!count) count = 1;
        auto line = text.substr(0, count);
        while (!line.empty() && line.back() == U' ') line.pop_back();
        lines.push_back(line);
        text.erase(0, count);
        while (!text.empty() && text.front() == U' ') text.erase(0, 1);
    }
    return lines;
}

std::u32string decode(const std::string& utf8) {
    std::u32string text;
    for (const auto c : platform::widen(utf8)) text += static_cast<char32_t>(c);
    return text;
}

} // namespace

std::vector<std::byte> playlist_artwork_png(const std::string& name) {
    if (!usable_name(name)) throw std::runtime_error("A generated cover needs a valid playlist name");
    if (fonts().empty()) throw std::runtime_error("Could not generate the playlist cover: no usable system font was found");
    std::vector<unsigned char> rgb(static_cast<std::size_t>(size_px * size_px) * 3);
    for (int y = 0; y < size_px; ++y) for (int x = 0; x < size_px; ++x) {
        auto* pixel = &rgb[static_cast<std::size_t>(y * size_px + x) * 3];
        const bool accent = (x - 450) * (x - 450) + (y - 40) * (y - 40) < 170 * 170;
        pixel[0] = static_cast<unsigned char>(45 + y * 15 / 512 + (accent ? 25 : 0)); // red
        pixel[1] = static_cast<unsigned char>(35 + x * 50 / 512 + (accent ? 15 : 0)); // green
        pixel[2] = static_cast<unsigned char>(80 + y * 50 / 512 + (accent ? 20 : 0)); // blue
    }
    Writer(22).draw(rgb, U"PLAYLIST", 48, 48);
    const auto title = decode(name);
    for (int size = 64; size >= 16; size -= 2) {
        const Writer writer(static_cast<float>(size));
        const auto lines = wrapped(writer, title, 416);
        const auto height = static_cast<float>(lines.size()) * writer.line_height();
        if (height > 280 && size > 16) continue;
        float top = 150 + std::floor((280 - height) / 2);
        for (const auto& line : lines) {
            if (top >= 430) break; // GDI clips the text to its rectangle
            writer.draw(rgb, line, 48 + std::floor((416 - writer.width(line)) / 2), top);
            top += writer.line_height();
        }
        break;
    }
    std::size_t length = 0;
    void* png = tdefl_write_image_to_png_file_in_memory_ex(rgb.data(), size_px, size_px, 3, &length, 6, MZ_FALSE);
    require(png != nullptr && length < 4 * 1024 * 1024);
    std::vector<std::byte> result(static_cast<const std::byte*>(png), static_cast<const std::byte*>(png) + length);
    mz_free(png);
    return result;
}

} // namespace music
