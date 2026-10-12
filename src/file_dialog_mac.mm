// SPDX-FileCopyrightText: 2026 DeckardDetribine and the ReSkateMusicPacker contributors
// SPDX-License-Identifier: GPL-3.0-only
#include "file_dialog.h"

#import <AppKit/AppKit.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>

namespace fs = std::filesystem;

// NSOpenPanel, modal to the app. "All files" stays reachable: the extensions only preselect.
std::vector<fs::path> pick(SDL_Window*, bool folder, bool image) {
    std::vector<fs::path> result;
    @autoreleasepool {
        NSOpenPanel* panel = [NSOpenPanel openPanel];
        panel.canChooseDirectories = folder;
        panel.canChooseFiles = !folder;
        panel.allowsMultipleSelection = !folder && !image;
        if (!folder) {
            NSArray<NSString*>* extensions = image
                ? @[@"png", @"jpg", @"jpeg", @"webp", @"bmp"]
                : @[@"mp3", @"flac", @"ogg", @"opus", @"wav", @"m4a", @"aac", @"wma", @"aiff", @"aif", @"webm", @"mka", @"mp4"];
            NSMutableArray<UTType*>* types = [NSMutableArray array];
            for (NSString* extension in extensions)
                if (UTType* type = [UTType typeWithFilenameExtension:extension]) [types addObject:type];
            panel.allowedContentTypes = types;
        }
        NSWindow* key = [NSApp keyWindow];
        if ([panel runModal] == NSModalResponseOK)
            for (NSURL* url in panel.URLs) result.emplace_back(url.fileSystemRepresentation);
        [key makeKeyAndOrderFront:nil];
    }
    return result;
}
