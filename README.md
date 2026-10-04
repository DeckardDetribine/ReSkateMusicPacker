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
| `--gui` | Force launch graphical user interface | |
| `--help`, `-h` | Display command-line usage | |

## Building

Requires Visual Studio 2022 (v143 toolset) and CMake 3.20+.

```powershell
cmake -B build/vs2022-x64 -G "Visual Studio 17 2022" -A x64
cmake --build build/vs2022-x64 --config Release
```

The resulting binaries will be placed in `build/vs2022-x64/Release/`:
- `ReSkateMusicPacker.exe` (Unified GUI & CLI application)
- `ReSkateMusicPackerTests.exe` (Automated regression test suite)
