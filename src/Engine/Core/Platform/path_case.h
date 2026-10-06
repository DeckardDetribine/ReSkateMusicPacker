// SPDX-FileCopyrightText: 2026 DeckardDetribine and the ReSkateMusicPacker contributors
// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include <filesystem>
#include <string>

namespace dingosdk {

// `path` itself when it exists. Otherwise, off Windows, the existing path whose components match
// it case-insensitively (ASCII), or `path` unchanged when there is none.
//
// The game's own layout names its folders as Windows spells them ("Win32", "Data", ...), and
// Windows does not care whether the disk agrees. A copy of the game on a Linux file system (Steam
// under Proton, a copied folder) keeps whatever case the installer wrote, so a lookup that works on
// Windows can miss there. Only the slow path lists directories, and only when the exact path is
// missing.
inline std::filesystem::path resolve_case(const std::filesystem::path& path) {
#ifdef _WIN32
    return path;
#else
    namespace fs = std::filesystem;
    std::error_code error;
    if (fs::exists(path, error)) return path;
    const auto fold = [](std::string text) {
        for (auto& c : text) if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
        return text;
    };
    fs::path current = path.root_path();
    for (const auto& part : path.relative_path()) {
        if (part.empty() || part == ".") continue;
        if (part == "..") { current /= part; continue; }
        if (fs::exists(current / part, error)) { current /= part; continue; }
        const auto wanted = fold(part.string());
        bool found = false;
        for (const auto& entry : fs::directory_iterator(current.empty() ? fs::path(".") : current, error)) {
            if (fold(entry.path().filename().string()) == wanted) {
                current /= entry.path().filename();
                found = true;
                break;
            }
        }
        if (!found) return path;
    }
    return current;
#endif
}

} // namespace dingosdk
