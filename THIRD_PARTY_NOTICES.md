# Third-party notices

KoggerApp is distributed under the **GNU General Public License v3** ([LICENSE](LICENSE)). It links
the components below, each under its own licence. Every licence text listed here also ships inside
the application and is readable from *Settings → Interface → About*.

| Component | Version | Linking | Licence | Text | Sources |
|---|---|---|---|---|---|
| [Qt](https://www.qt.io/) | 6.8.3 | dynamic | GNU LGPL v3 | [COPYING.LGPLv3](resources/licenses/COPYING.LGPLv3) | [download.qt.io](https://download.qt.io/archive/qt/6.8/6.8.3/single/) |
| [FFmpeg](https://ffmpeg.org/) | 7.1 | dynamic | GNU LGPL v2.1 or later | [COPYING.LGPLv2.1](third_party/ffmpeg/COPYING.LGPLv2.1) | [ffmpeg-7.1.tar.xz](https://ffmpeg.org/releases/ffmpeg-7.1.tar.xz) |
| [FreeType](https://freetype.org/) | 2.13.2 | **static** | The FreeType License (FTL) | [FTL.TXT](third_party/freetype/FTL.TXT) | [freetype-2.13.2.tar.xz](https://downloads.sourceforge.net/project/freetype/freetype2/2.13.2/freetype-2.13.2.tar.xz) |
| [SDL](https://libsdl.org/) | 3.4.2 | **static** (Windows, Linux), dynamic (Android) | zlib | [LICENSE.txt](third_party/sdl3/LICENSE.txt) | [SDL3-3.4.2.tar.gz](https://github.com/libsdl-org/SDL/releases/download/release-3.4.2/SDL3-3.4.2.tar.gz) |

**Qt** and **FFmpeg** are used unmodified from a stock Qt installation and can be replaced with any
compatible build of the same version. The FFmpeg libraries are an LGPL build: the configure line
embedded in `avutil-59` carries neither `--enable-gpl` nor `--enable-nonfree`.

**FreeType** is linked statically, so its code is part of the shipped binary. It is taken under the
FTL, the BSD-style arm of its dual FTL / GPL v2 licensing. The FTL asks that FreeType be credited in
the product documentation:

> Portions of this software are copyright © 2006–2023 The FreeType Project
> (<https://www.freetype.org>). All rights reserved.

**SDL** provides game controller input. On Windows and Linux it is linked statically; on Android it
ships as `libSDL3.so` together with its Java classes in `platform/android/src/org/libsdl/app/` (five
copied unchanged from the SDL 3.4.2 release, four replaced by stubs written for this app). The zlib
licence asks for no credit in the product; it requires that the notice stays with the sources and
that altered files are marked as such.

Provenance and what exactly is vendored: [third_party/ffmpeg/README.md](third_party/ffmpeg/README.md),
[third_party/freetype/README.md](third_party/freetype/README.md),
[third_party/sdl3/README.md](third_party/sdl3/README.md).
