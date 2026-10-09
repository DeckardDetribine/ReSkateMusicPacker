# Notices and attribution

ReSkateMusicPacker is distributed under the [GNU General Public License, version 3](LICENSE)
(GPL-3.0). Copyright © 2026 DeckardDetribine and the ReSkateMusicPacker contributors.

## ReSkate

The engine code under `src/Engine/` (`Core/`, `Resource/` and `Vfs/`: Frostbite bundle, TOC, CAS
and EBX handling) is ported from the engine of the **ReSkate** project
(<https://github.com/Dingo-Shenanigans/ReSkate>), Copyright © 2026 the ReSkate contributors, which
is licensed under GPL-3.0. The source comments in those files also credit the upstream
*ReSkateStudio* `sdk/` they were originally ported from.

Because that code is GPL-3.0, ReSkateMusicPacker as a whole is distributed under GPL-3.0. If you
share a modified version, share its source code under the same license.

## Third-party libraries

The libraries under `External/` are **not** covered by this project's GPL-3.0; each keeps its own
license, and their license files are kept beside them in the tree and should ship with any binary:

| Library | Path | License |
|---|---|---|
| Dear ImGui | `External/imgui` | MIT |
| RapidJSON | `External/rapidjson` | MIT |
| LZ4 | `External/lz4` | BSD 2-Clause |
| Zstandard (zstd) | `External/zstd` | BSD 3-Clause (or GPL-2.0) |
| miniz | `External/miniz` | MIT / public domain |
| ooz (decoder only; macOS/Linux builds) | `External/ooz` | GPL-3.0-or-later |
| SIMD Everywhere (subset, used by ooz) | `External/ooz/simde` | MIT |
| stb_image (macOS/Linux builds) | `External/stb` | MIT / public domain |

The Dear ImGui SDL2 backends (`External/imgui/backends/imgui_impl_sdl2*`, `imgui_impl_sdlrenderer2*`)
are part of Dear ImGui (MIT). macOS and Linux builds link SDL2 (zlib license) from the system.

## Game content

*skate.* and its content belong to Electronic Arts. Nothing in this repository grants any rights to
them.
