# FFmpeg public headers

## Origin

    https://ffmpeg.org/releases/ffmpeg-7.1.tar.xz
    sha256 40973d44970dbc83ef302b0609f2e74982be2d85916dd2ee7472d30678a7abe6

`include/` holds 129 headers. 128 of them are copied verbatim from that archive and are
byte-identical to it; only the public subset is kept, the archive carries 986 headers
including internal ones. No FFmpeg source code is vendored.

`libavutil/avconfig.h` is the exception and says so in its own comment: upstream
generates it from `./configure`, so it is not in the archive. `libavutil/ffversion.h` is
generated the same way and is not vendored.

## License

Every vendored upstream header carries an LGPL 2.1-**or-later** notice; none is GPL. The
notices are intact - nothing was stripped. The full license text is in
`COPYING.LGPLv2.1`, taken from the same release archive.

LGPL 2.1-or-later is compatible with this project's GPLv3: LGPL 2.1 section 3 allows a
copy to be taken under the ordinary GPL, and the "or later" wording also permits
LGPL 3, which GPLv3 accepts directly.

Source for the exact version is available from the release URL above.
