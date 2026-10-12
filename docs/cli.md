# CLI reference

```text
ReSkateMusicPacker.exe <game folder> <song folder> [output folder] [options]
```

- **game folder** - the ReSkate install containing `Skate.exe`.
- **song folder** - a folder of audio files. The CLI reads the files directly in that folder (it does
  not recurse), ignoring images.
- **output folder** - where the mod is written. Defaults to `<song folder>_mod` next to the song
  folder.

Run with no arguments (or `--gui`) for the graphical interface.

## Options

| Flag | Description | Default |
|---|---|---|
| `--name <mod name>` | Display name of the mod. | Folder name |
| `--playlist <playlist>` | Playlist name shown in the in-game music menu. | Folder name |
| `--bitrate <kbps>` | Opus bitrate, 64-320. | `192` |
| `--no-normalize` | Disable EBU R128 loudness normalisation (two-pass to -15 LUFS). | Normalised |
| `--thunderstore` | Also export a Thunderstore-ready `.zip` (see below). | Off |
| `--author <name>` | Author namespace in the Thunderstore manifest. | `Author` |
| `--version <x.y.z>` | Thunderstore package version. | `1.0.0` |
| `--playlist-artwork <name> <image>` | Cover image for the named playlist. | None |
| `--track-artwork <id> <image>` | Cover for the exact `Artist - Title` song id; repeat for several. | None |
| `--generate-playlist-artwork <name>` | Generate a text cover from the playlist name. | Off |
| `--get-ffmpeg [folder]` | Download the pinned static ffmpeg build and print the install path, then exit. | `.` or `%LOCALAPPDATA%` |
| `--gui` | Force the graphical interface. | |
| `--help`, `-h` | Show usage. | |

`PNG`, `JPEG`, `WebP`, `BMP` (and other WIC-readable) images can be used for artwork. Covers are scaled
proportionally to a 512x512 PNG with dark padding, saved inside the mod.

## Examples

Basic pack:

```powershell
ReSkateMusicPacker.exe "F:\Games\ReSkate-1.0.0" "C:\Music\My Mix" --playlist "My Mix"
```

Full: playlist cover, a per-track cover, and a Thunderstore zip:

```powershell
ReSkateMusicPacker.exe "F:\Games\ReSkate-1.0.0" "C:\Music\My Mix" "C:\Mods\My Mix" `
    --name "My Mix" --playlist "My Mix" --bitrate 224 `
    --playlist-artwork "My Mix" "C:\Pictures\mix.png" `
    --track-artwork "MGMT - Kids" "C:\Pictures\kids.png" `
    --thunderstore --author MyName --version 1.0.0
```

Generated playlist cover instead of an image:

```powershell
ReSkateMusicPacker.exe "F:\Games\ReSkate-1.0.0" "C:\Music\My Mix" --generate-playlist-artwork "My Mix"
```

Fetch ffmpeg (no game or songs needed):

```powershell
ReSkateMusicPacker.exe --get-ffmpeg
ReSkateMusicPacker.exe --get-ffmpeg "C:\Tools\ffmpeg"
```

`--get-ffmpeg` (Windows only) downloads the same pinned static LGPL build (BtbN/FFmpeg-Builds) the GUI button uses,
verifies its SHA-256, extracts `ffmpeg.exe` + `ffprobe.exe` and prints the folder. With no folder it
installs beside the exe when writable, else under `%LOCALAPPDATA%\ReSkateMusicPacker\ffmpeg`.
`FFMPEG_URL` and `FFMPEG_SHA256` override the pinned source.

## Caching

Encoded Opus is cached by source-file hash and settings in
`%LOCALAPPDATA%\ReSkateMusicPacker\cache\` (see [where the files live](troubleshooting.md#where-the-files-live)
for macOS and Linux), so rebuilding only re-encodes what changed. Deleting that
folder forces a full re-encode.

## Thunderstore

`--thunderstore` writes `<Author>-<Name>-<Version>.zip` (or the `--author`/`--version` you gave) beside
the mod. It contains the whole mod plus a Thunderstore `manifest.json`, a generated `README.md`
tracklist, and a 256x256 `icon.png` (custom if you provide one, otherwise generated).
