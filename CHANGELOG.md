# Changelog

All notable changes to ReSkate Music Packer are documented here.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and this project
adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- Cover artwork: extract embedded album art automatically, choose a track or playlist image, or
  generate a text cover. Covers are 512x512 PNGs saved with the mod and included in Thunderstore
  exports.
- Documentation: this changelog, a `CONTRIBUTING` guide, a rewritten README, and guides under `docs/`,
  plus an MIT `LICENSE`.

### Changed

- Loudness normalisation now targets **-15 LUFS** with two-pass `loudnorm`, matching the level of the
  game's own tracks (they measure about -15 LUFS), instead of -16 with a single dynamic pass.

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
