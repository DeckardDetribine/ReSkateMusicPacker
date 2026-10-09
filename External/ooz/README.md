# ooz (decoder only)

Open-source Kraken / Mermaid / Selkie / Leviathan / LZNA / BitKnit decompressor by Powzix,
from the portable fork https://github.com/zao/ooz at commit `ff5aeb9e45e362e8d6bb1199aa82406285dd2a18`.
Only the decoder sources are vendored: `kraken.cpp`, `bitknit.cpp`, `lzna.cpp`, `stdafx.h`,
built with `OOZ_BUILD_DLL=1` so the command-line tool and DLL loader are left out.

One local change: `stdafx.h` always defines `_rotl` as a portable rotate instead of taking it from
`<x86gprintrin.h>`, which Clang on x86-64 Linux does not provide (marked "ReSkateMusicPacker patch").

License: GNU GPL version 3 or later (see the header of `kraken.cpp`), compatible with this
project's GPL-3.0.

`simde/` is the subset of SIMD Everywhere (https://github.com/simd-everywhere/simde, commit
`dd0b662fd8cf4b1617dbbb4d08aa053e512b08e4`, the fork's pinned submodule) that the decoder includes;
it maps the SSE2 intrinsics onto NEON or plain C so the decoder also builds for ARM. MIT license,
see `simde/COPYING`.

ReSkateMusicPacker uses it only off Windows, to read the game's Oodle-compressed CAS blocks. On
Windows the game's own oo2core DLL is used. The decoder is not hardened against malicious input
("not fuzz safe" upstream); it only ever reads the user's own game files.
