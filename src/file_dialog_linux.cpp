// SPDX-FileCopyrightText: 2026 DeckardDetribine and the ReSkateMusicPacker contributors
// SPDX-License-Identifier: GPL-3.0-only
#include "file_dialog.h"
#include "platform.h"

#include <SDL.h>

#include <chrono>
#include <future>
#include <sstream>
#include <string>

namespace fs = std::filesystem;

// zenity (GNOME and most desktops) or kdialog (KDE); both print the chosen paths one per line.
std::vector<fs::path> pick(SDL_Window* owner, bool folder, bool image) {
    const char* audio = "*.mp3 *.flac *.ogg *.opus *.wav *.m4a *.aac *.wma *.aiff *.aif *.webm *.mka *.mp4";
    const char* images = "*.png *.jpg *.jpeg *.webp *.bmp";
    std::vector<std::string> args;
    if (platform::on_path("zenity")) {
        args = {"zenity", "--file-selection", "--separator=\n"};
        if (folder) args.emplace_back("--directory");
        else {
            if (!image) args.emplace_back("--multiple");
            args.push_back(std::string("--file-filter=") + (image ? "Images | " : "Audio | ") + (image ? images : audio));
            args.emplace_back("--file-filter=All files | *");
        }
    } else if (platform::on_path("kdialog")) {
        if (folder) args = {"kdialog", "--getexistingdirectory", "."};
        else {
            args = {"kdialog", "--getopenfilename", ".", std::string(image ? images : audio) + (image ? "|Images" : "|Audio")};
            if (!image) args.insert(args.begin() + 1, {"--multiple", "--separate-output"});
        }
    } else {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "ReSkate Music Packer",
                                 "No file dialog was found. Install zenity or kdialog, or drag files and folders onto the "
                                 "window instead.", owner);
        return {};
    }
    // The dialog is another process, so waiting on it would stop this window answering the
    // compositor, which then shows it as hung. Keep pumping events meanwhile, and drop keys and
    // clicks aimed at the window behind the dialog, as a modal dialog would.
    auto dialog = std::async(std::launch::async, [&args] {
        std::string output;
        const int code = platform::run_process(args, &output);
        return std::pair{code, output};
    });
    while (dialog.wait_for(std::chrono::milliseconds(20)) == std::future_status::timeout) {
        SDL_PumpEvents();
        SDL_FlushEvents(SDL_KEYDOWN, SDL_MOUSEWHEEL);
    }
    const auto [code, output] = dialog.get();
    if (code != 0) return {}; // cancelled
    std::vector<fs::path> result;
    std::istringstream lines(output);
    for (std::string line; std::getline(lines, line);)
        if (!line.empty() && line.front() == '/') result.emplace_back(line); // skips GTK warnings on stderr
    return result;
}
