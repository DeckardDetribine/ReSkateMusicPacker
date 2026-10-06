// SPDX-FileCopyrightText: 2026 DeckardDetribine and the ReSkateMusicPacker contributors
// SPDX-License-Identifier: GPL-3.0-only
// The system's open dialog: Explorer's on Windows, NSOpenPanel on macOS, zenity or kdialog on Linux.
#pragma once
#include "gui_renderer.h"
#include <filesystem>
#include <vector>

// Audio files (several), one image (`image`), or one folder (`folder`). Empty when cancelled.
std::vector<std::filesystem::path> pick(dingosdk::launcher_gui::detail::NativeWindow owner, bool folder, bool image = false);
