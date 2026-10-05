# Changelog

All notable changes to ReSkate Music Packer are documented here.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and this project
adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [1.0.1] - 2026-10-05

### Fixed

- Thunderstore icon picker now filters for images (`.png`, `.jpg`, `.jpeg`, `.webp`, `.bmp`) by default instead of audio files.
- Thunderstore custom icons are now scaled and converted to standard 256x256 PNGs.
- Fixed File Explorer auto-reveal failing silently when exporting a Thunderstore package.
- Exclude local editor project files (`reskate-music-project.json`) from Thunderstore export packages.

### Added

- Thunderstore export dialog now allows selecting a custom output folder, previews the target package path, and displays the full export path in the status bar.
- Inline playlist cover selector button directly beside the playlist name field, with 1-click options to choose an image, generate a text cover, or reset to automatic.
- Pre-build confirmation prompt with live visual cover preview when playlist artwork is still set to Automatic, allowing 1-click generation of text covers, picking an image, or continuing with automatic.

## [1.0.0] - 2026-10-05

### Added

- Cover artwork: extract embedded album art automatically, choose a track or playlist image, or
  generate a text cover. Covers are 512x512 PNGs saved with the mod and included in Thunderstore
  exports.
- Documentation: this changelog, a `CONTRIBUTING` guide, a rewritten README, and guides under `docs/`,
  plus a `LICENSE`.
- A **Settings** dialog (from the toolbar) to change the game folder and ffmpeg after the first-run
  setup, instead of only at startup.
- **Download ffmpeg automatically**: when ffmpeg/ffprobe are missing, the setup page and the Settings
  dialog can fetch a pinned static LGPL build (BtbN/FFmpeg-Builds) into `.\ffmpeg` beside the app or
  `%LOCALAPPDATA%\ReSkateMusicPacker\ffmpeg`. Only a click starts it; the archive is verified against
  a pinned SHA-256 before extraction, and `FFMPEG_URL` / `FFMPEG_SHA256` override the source. The CLI
  gets `--get-ffmpeg [folder]` for the same, headlessly.

### Changed

- **Relicensed from MIT to GPL-3.0**, with attribution: `src/Engine/` is ported from the
  [ReSkate](https://github.com/Dingo-Shenanigans/ReSkate) project's engine code (Copyright © 2026 the
  ReSkate contributors, GPL-3.0). See `NOTICE.md`.
- Loudness normalisation now targets **-15 LUFS** with two-pass `loudnorm`, matching the level of the
  game's own tracks (they measure about -15 LUFS), instead of -16 with a single dynamic pass.
- ffmpeg/ffprobe now run without a console window, and with their output captured in-process, so the
  GUI no longer flashes command prompts.
- The Thunderstore export builds its `.zip` in-process (miniz), so there is no dependency on the
  system `tar`.

### Fixed

- Write the correct sample count in the stream's codec header, so a mod track reaches its end and the
  game advances to the next track instead of stopping.

## [0.1.0] - 2026-10-04

### Added

- First release: a hybrid Dear ImGui / DirectX 12 GUI and a scriptable CLI.
- Add-only music mods: Opus encoding with a per-source encode cache and EBU R128 loudness
  normalisation.
- Multiple playlists per mod, and a warning when a song's id clashes with an installed or built-in
  song.
- Thunderstore export: a ready `.zip` with `manifest.json`, a generated README tracklist, and a
  generated or custom icon.
- High-DPI aware interface.
