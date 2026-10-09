// SPDX-FileCopyrightText: 2026 DeckardDetribine and the ReSkateMusicPacker contributors
// SPDX-License-Identifier: GPL-3.0-only
#include "file_dialog.h"

#include <ShObjIdl.h>
#include <wrl/client.h>

namespace fs = std::filesystem;
using Microsoft::WRL::ComPtr;

// The shell's file dialog: audio files (several) or one folder.
std::vector<fs::path> pick(HWND owner, bool folder, bool image) {
    std::vector<fs::path> result;
    ComPtr<IFileOpenDialog> dialog;
    if (FAILED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dialog)))) return result;
    FILEOPENDIALOGOPTIONS options{};
    dialog->GetOptions(&options);
    dialog->SetOptions(options | (folder ? FOS_PICKFOLDERS : image ? 0 : FOS_ALLOWMULTISELECT) | FOS_FORCEFILESYSTEM);
    if (!folder) {
        const COMDLG_FILTERSPEC filters[]{{image ? L"Images" : L"Audio", image ? L"*.png;*.jpg;*.jpeg;*.webp;*.bmp" : L"*.mp3;*.flac;*.ogg;*.opus;*.wav;*.m4a;*.aac;*.wma;*.aiff;*.aif;*.webm;*.mka;*.mp4"}, {L"All files", L"*.*"}};
        dialog->SetFileTypes(2, filters);
    }
    if (FAILED(dialog->Show(owner))) return result;
    ComPtr<IShellItemArray> items;
    if (FAILED(dialog->GetResults(&items))) return result;
    DWORD count{};
    items->GetCount(&count);
    for (DWORD i = 0; i < count; ++i) {
        ComPtr<IShellItem> item;
        PWSTR path{};
        if (SUCCEEDED(items->GetItemAt(i, &item)) && SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &path))) result.emplace_back(path);
        CoTaskMemFree(path);
    }
    return result;
}
