# ReSkate Music Packer

A standalone hybrid GUI and CLI tool that packages audio files into native **skate.** music mods for **ReSkate**.

Mods built with this tool add new songs and custom playlists into the game's audio engine without replacing any existing files.

## Features

- **Hybrid GUI & CLI**:
  - Run with no arguments or double-click in Explorer to launch the Dear ImGui DirectX 12 graphical interface.
  - Run from PowerShell / Command Prompt with arguments for headless scripted batch packing.
- **Transcode & Encode Cache**: Hashed Opus caching (`%LOCALAPPDATA%\ReSkateMusicPacker\cache\`) avoids re-encoding unchanged tracks across rebuilds.
- **EBU R128 Loudness Normalization**: Normalizes tracks to -16 LUFS via FFmpeg `loudnorm` filter (enabled by default).
- **Multiple Playlists per Mod**: Custom playlists per track, visible in the in-game music menu.
- **Track & Playlist Artwork**: Embedded album art is extracted automatically. Through **Artwork...**, choose an override image or enable **Generate text cover** for a playlist, with a preview. Covers are 512x512 PNGs saved with the mod and included in Thunderstore exports. Requires a ReSkate runtime with mod artwork support.
- **Thunderstore Export**: Direct generation of Thunderstore-compatible `.zip` packages complete with `manifest.json`, generated `README.md` tracklists, and auto-generated or custom 256x256 icons.
- **Song Clash Warning**: Checks artist and title against already-installed mods and game soundtrack to prevent asset ID collisions.
- **High-DPI Scaling**: Per-monitor v2 DPI awareness for crisp visuals on 4K and multi-monitor setups.

## CLI Usage

```text
ReSkateMusicPacker.exe <game folder> <song folder> [output folder] [options]
```

### Options

| Flag | Description | Default |
|------|-------------|---------|
| `--name <mod name>` | Name of the mod | Folder name |
| `--playlist <playlist>` | Default in-game playlist name | Folder name |
| `--bitrate <kbps>` | Opus bitrate (64-320 kbps) | `192` |
| `--no-normalize` | Disable EBU R128 loudness normalization | Normalized |
| `--thunderstore` | Export Thunderstore-ready zip package | Off |
| `--author <name>` | Author namespace in Thunderstore manifest | `Author` |
| `--version <x.y.z>` | Semantic version string | `1.0.0` |
| `--playlist-artwork <name> <image>` | Cover image for the named playlist | None |
| `--track-artwork <id> <image>` | Cover image for the exact `Artist - Title` song ID; repeat for several tracks | None |
| `--generate-playlist-artwork <name>` | Generate a text cover using the playlist name | Off |
| `--gui` | Force launch graphical user interface | |
| `--help`, `-h` | Display command-line usage | |

```powershell
ReSkateMusicPacker.exe "F:\Games\ReSkate-1.0.0" "C:\Music\My Mix" --playlist "My Mix" --playlist-artwork "My Mix" "C:\Pictures\mix.jpg" --track-artwork "Artist - Title" "C:\Pictures\track.png"
```

PNG, JPEG, WebP and BMP images can be selected in the GUI. The project file preserves the original image paths, so keep the source images for future edits. The packed mod contains its own PNG copies and needs no source images or external image host to display covers. Older ReSkate builds ignore the optional artwork fields and continue to load the music.

Rectangular images are resized proportionally to fit the square cover, with dark padding around the edges. No artwork is cropped or stretched, and previews use the same conversion as packing.

Preview extraction and image decoding run in the background, with a loading message while the window stays responsive. Previews share the scan/build worker so the packer's temporary files are never used by concurrent jobs.

For tracks, an explicitly selected image overrides embedded album art. Only attached pictures are extracted; ordinary video streams are ignored. Missing or unreadable embedded art does not block packing. Extraction is cached by the track's content hash.

For playlists, a selected image takes priority over a generated text cover; otherwise the first track with available artwork supplies the cover. The text generator fits and wraps the playlist name on a colored background, including long words and Unicode names. Generated covers need no image file and their settings are saved with the project.

## Building

Requires Visual Studio 2022 (v143 toolset) and CMake 3.20+.

```powershell
cmake -B build/vs2022-x64 -G "Visual Studio 17 2022" -A x64
cmake --build build/vs2022-x64 --config Release
```

The resulting binaries will be placed in `build/vs2022-x64/Release/`:
- `ReSkateMusicPacker.exe` (Unified GUI & CLI application)
- `ReSkateMusicPackerTests.exe` (Automated regression test suite)
