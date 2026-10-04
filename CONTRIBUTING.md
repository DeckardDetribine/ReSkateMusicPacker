# Contributing

Thanks for helping improve ReSkate Music Packer.

## Before you start

- This is a **Windows** tool (Visual Studio 2022, C++20, CMake). Keep it that way.
- For anything larger than a small fix, **open an issue first** so we can agree on the approach. Keep
  pull requests focused on one change.

## Building and testing

See [docs/building.md](docs/building.md). In short:

```powershell
cmake -B build/vs2022-x64 -G "Visual Studio 17 2022" -A x64
cmake --build build/vs2022-x64 --config Release
.\build\vs2022-x64\Release\ReSkateMusicPackerTests.exe
```

The tests and the tool need `ffmpeg`/`ffprobe` on `PATH`.

## Style

- Match the surrounding code (C++20, the existing naming and comment style). No new third-party
  dependencies without discussion - the project deliberately vendors what it needs under `External/`.
- Warnings are errors; a build must be clean.

## Commits and pull requests

- Use [Conventional Commits](https://www.conventionalcommits.org/): `feat:`, `fix:`, `docs:`, `ui:`,
  `refactor:`, `test:` ...
- In the PR, describe **what** changed and **how you tested it** - which songs/mod you packed, your
  ffmpeg version, and any caveats. Note anything you couldn't verify.

## Reporting bugs

Include:

- the packer version (or commit) and your ffmpeg/ffprobe version (`ffmpeg -version`);
- the exact steps and the output/error text;
- if it's about a built mod, the mod's `reskate-music.json` and whether the game/launcher logged an
  error.
