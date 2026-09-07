/*
 * NOT an upstream FFmpeg file.
 *
 * FFmpeg generates libavutil/avconfig.h from ./configure, so it is absent from the
 * release tarball while several public headers (pixfmt.h, macros.h, bswap.h,
 * intreadwrite.h) include it. This is the equivalent for the targets this project
 * builds: x86_64 Windows and Linux, arm64-v8a and armeabi-v7a Android - all
 * little-endian and all tolerant of unaligned access.
 *
 * The values must agree with how the FFmpeg libraries in the Qt installation were
 * built; both are properties of the CPU, not of the build.
 */
#ifndef AVUTIL_AVCONFIG_H
#define AVUTIL_AVCONFIG_H
#define AV_HAVE_BIGENDIAN 0
#define AV_HAVE_FAST_UNALIGNED 1
#endif /* AVUTIL_AVCONFIG_H */
