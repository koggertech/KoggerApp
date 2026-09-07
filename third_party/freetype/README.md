# FreeType

## Origin

    https://downloads.sourceforge.net/project/freetype/freetype2/2.13.2/freetype-2.13.2.tar.xz

Version **2.13.2**, confirmed two ways in this tree: `include/freetype/freetype.h` declares
`FREETYPE_MAJOR 2` / `FREETYPE_MINOR 13` / `FREETYPE_PATCH 2`, and the Android archives
(`lib/arm64-v8a`, `lib/armeabi-v7a`) carry the string `2.13.2`. The other four archives are
stripped and hold no version string.

`include/` holds the public headers. `lib/<kit>/libfreetype.a` holds **prebuilt static**
archives, one per kit: `aarch64`, `arm64-v8a`, `armeabi-v7a`, `gcc`, `llvm-mingw-x64`,
`mingw-x64`. No FreeType source code is vendored.

No archive checksum is recorded: what is vendored here are prebuilt libraries, not a copy of
the release tarball, so there is nothing in the tree to hash against upstream.

⚠️ FreeType is linked **statically**, so it ends up inside the shipped binary — unlike Qt and
FFmpeg, which are dynamic. It is the one dependency whose code is distributed as part of
`KoggerApp` itself.

## License

FreeType is dual-licensed: the **FreeType License** (`FTL.TXT`, a BSD-style licence with an
advertising/attribution clause) or **GPL v2**. This project takes it under the **FTL**.

`FTL.TXT` in this directory is a verbatim copy of the same licence text Qt ships at
`Src/qtbase/LICENSES/FTL.txt` (Qt bundles FreeType too); the two files are byte-identical.

The FTL asks that FreeType be credited in the documentation of any product that uses it. That
credit lives in [THIRD_PARTY_NOTICES.md](../../THIRD_PARTY_NOTICES.md) and in the app's
**About** page, which renders `FTL.TXT` from the resource bundle.
