# Troubleshooting

## ffmpeg / ffprobe not found

The packer needs both `ffmpeg.exe` and `ffprobe.exe`. Put them on your `PATH`, or point the GUI's
ffmpeg field at the folder that contains them. A full ffmpeg build includes both.

## Song clash warning

Every song id is the string `Artist - Title`. If a song's id already exists in another installed mod
or in the game's soundtrack, the mod can't add it (and a duplicate id can break the game's music
catalog). Rename the artist/title (or fix the file's tags) so the id is unique, then rebuild.

## Cover art doesn't show

- The runtime must support mod artwork. On an older ReSkate runtime the covers are ignored (music still
  plays).
- Check `reskate-music.json` references the file by a path relative to the mod folder, `.png`, with no
  `..` or `\`.
- The runtime only serves artwork for songs it can resolve by their `Artist - Title` id, so make sure
  the id in `song_artwork` matches the song exactly.

## Mod tracks sound quiet (or loud) next to the game's music

The packer normalises to **-15 LUFS** (true peak -1.5 dBTP) with two-pass `loudnorm`, the level the
game's own tracks sit at. If you disabled normalisation, or if you're comparing against a differently
mastered source, this will differ. Use `--no-normalize` to pack the source level unchanged.

## The build fails with "cannot open ReSkateMusicPacker.exe"

A previous copy of the packer (or the GUI) is still running and holding the file. Close it and rebuild.

## Thunderstore export

The `.zip` is written beside the mod (or to the output you gave). Give it an author and version with
`--author` / `--version` (or the export dialog). It contains the full mod plus `manifest.json`, a
generated `README.md` tracklist and a 256x256 `icon.png`.

## The game doesn't see the mod

- Make sure the mod folder is in your ReSkate `Mods` folder and enabled in the launcher.
- ReSkate merges mods at launch; restart the game (or re-run the launcher) after adding a mod.
- The `.reskate-studio-patch` records the game build the mod targets. A mod built for a different game
  build may be left out by the merger; rebuild against the game folder you're playing.

## Where the files live

- Settings: `%APPDATA%\ReSkateMusicPacker\settings.json`
- Encode cache: `%LOCALAPPDATA%\ReSkateMusicPacker\cache\` (delete to force a full re-encode)
