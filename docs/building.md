# Building from source

## Prerequisites

- **Visual Studio 2022** with the **Desktop development with C++** workload (v143 toolset).
- **CMake 3.20+**.
- No other downloads: Dear ImGui, zstd, LZ4, miniz and RapidJSON are vendored under `External/`.

## Build

From the repository root:

```powershell
cmake -B build/vs2022-x64 -G "Visual Studio 17 2022" -A x64
cmake --build build/vs2022-x64 --config Release
```

Outputs land in `build/vs2022-x64/Release/`:

- `ReSkateMusicPacker.exe` - the unified GUI & CLI application.
- `ReSkateMusicPackerTests.exe` - the regression tests.

## Tests

```powershell
.\build\vs2022-x64\Release\ReSkateMusicPackerTests.exe
```

It prints `all passed` on success. The tests cover packing and the artwork conversion, and need
ffmpeg/ffprobe on `PATH`.

## Layout of the source

```
src/
  main.cpp            # GUI (Dear ImGui/DX12) and the CLI entry point
  packer.cpp/.h       # the library: scan, encode, build the mod, Thunderstore export
  artwork.cpp         # cover image extraction/scaling and the generated text cover
  Engine/              # Frostbite RES/EBX/TOC/cas readers, writer, and bundle handling
  gui_renderer.*      # the DirectX 12 renderer for the GUI
External/             # vendored third-party libraries
test/                 # tests
```
