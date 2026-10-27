# SDL3

## Origin

    https://github.com/libsdl-org/SDL/releases/download/release-3.4.2/SDL3-3.4.2.tar.gz
    sha256 ef39a2e3f9a8a78296c40da701967dd1b0d0d6e267e483863ce70f8a03b4050c

Version **3.4.2** (`SDL_REVISION` `SDL-release-3.4.2-0-g683181b47`). The SHA-256 above is the
digest GitHub publishes for that release asset.

`include/SDL3/` holds the public headers, copied unchanged from the tarball. `lib/<kit>/` holds
the prebuilt libraries. No SDL source code is vendored; [Rebuilding](#rebuilding) below lists the
exact steps that produced every file here.

| Kit | File | Kind |
|---|---|---|
| `llvm-mingw-x64` | `libSDL3.a` | static, Qt llvm-mingw 17.0.6 |
| `arm64-v8a` | `libSDL3.so` | shared, NDK r27d, `android-28`, 16 KB page aligned |
| `armeabi-v7a` | `libSDL3.so` | shared, NDK r27d, `android-28` |
| `linux-x86_64` | `libSDL3.a` | static, Clang 18.1.3, glibc 2.39 (Ubuntu 24.04), udev |
| `linux-aarch64` | `libSDL3.a` | static, aarch64 Linux (native or cross) — not built yet |

## Configuration

Only the subsystems needed for game controllers are enabled: joystick, gamepad, haptic, hidapi,
virtual joystick, power, sensor. Audio, GPU, render, camera, dialog, D-Bus and IBus are off.
Video is off on desktop and on for Android.

- Windows: `SDL_XINPUT=ON`, `SDL_DIRECTX=OFF` — joystick drivers hidapi, rawinput, virtual, wgi, xinput.
- Linux: `SDL_UNIX_CONSOLE_BUILD=ON` — joystick drivers hidapi, linux, virtual; udev hotplug only when `libudev-dev` was present at build time.
- Android: shared library, `SDL_ANDROID_JAR=OFF`, `SDL_HIDAPI_LIBUSB=OFF` — joystick drivers android, hidapi, virtual.

## Linking

The root `CMakeLists.txt` (block "SDL3 game controllers") picks `lib/<kit>/` for the kit being
built. When the file is missing — today the `linux-aarch64` kit — the app still builds, just
without game controller support, and CMake prints a `STATUS` line saying so. Adding a library here
is all it takes to enable a kit; no CMake change is needed.

Static archives need system libraries next to them: on Windows `setupapi winmm imm32 version
ole32 oleaut32 uuid advapi32 shell32 user32 gdi32 kernel32`, on Linux `dl pthread m`.

Android library: `libSDL3.so` defines its own `JNI_OnLoad`, which registers native methods on the
`org.libsdl.app` Java classes (`SDLActivity`, `SDLInputConnection`, `SDLAudioManager`,
`SDLControllerManager`, `HIDDeviceManager`). It must therefore ship as a separate shared library
next to the app, together with Java classes of the same SDL version — they live in
`platform/android/src/org/libsdl/app/` and must be replaced together with this library.

## License

zlib license, `LICENSE.txt` (verbatim from the tarball), also bundled into the app and shown in
*Settings → Interface → About*. It requires no credit in the product, only that the notice stays
with the source distribution and altered versions are marked — the Java stubs that replace SDL's
own `SDL`, `SDLActivity`, `SDLAudioManager` and `SDLInputConnection` say so in their class doc.

## Rebuilding

Needed only to bump SDL, change its options or move to another toolchain — the app build itself
uses the files above as they are. Every kit uses the same source tree and the same option set.

### 1. Source

```bash
curl -fLO https://github.com/libsdl-org/SDL/releases/download/release-3.4.2/SDL3-3.4.2.tar.gz
echo "ef39a2e3f9a8a78296c40da701967dd1b0d0d6e267e483863ce70f8a03b4050c  SDL3-3.4.2.tar.gz" | sha256sum -c -
tar -xzf SDL3-3.4.2.tar.gz
```

On Windows without `sha256sum`: `(Get-FileHash SDL3-3.4.2.tar.gz).Hash` in PowerShell must print the
same digest (upper case). Delete the tarball if it does not match, never build from it.

### 2. Options common to every kit

```
-G Ninja -DCMAKE_BUILD_TYPE=Release
-DSDL_INSTALL=OFF -DSDL_UNINSTALL=OFF -DSDL_TESTS=OFF -DSDL_TEST_LIBRARY=OFF -DSDL_EXAMPLES=OFF
-DSDL_AUDIO=OFF -DSDL_GPU=OFF -DSDL_RENDER=OFF -DSDL_CAMERA=OFF -DSDL_DIALOG=OFF
-DSDL_JOYSTICK=ON -DSDL_HAPTIC=ON -DSDL_HIDAPI=ON -DSDL_POWER=ON -DSDL_SENSOR=ON
-DSDL_VIRTUAL_JOYSTICK=ON -DSDL_DBUS=OFF -DSDL_IBUS=OFF -DSDL_MMX=OFF
```

They are written as `<COMMON>` below. Read the "SDL3 was configured with the following options"
summary CMake prints: the joystick driver list must match the Configuration section above.

### 3. Windows — `lib/llvm-mingw-x64/libSDL3.a`

Qt's llvm-mingw (`C:\Qt\Tools\llvm-mingw1706_64\bin`), Qt's CMake and Ninja first in `PATH`:

```
cmake -S SDL3-3.4.2 -B build-llvm-mingw-x64 <COMMON> -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DSDL_SHARED=OFF -DSDL_STATIC=ON -DSDL_VIDEO=OFF -DSDL_DIRECTX=OFF -DSDL_XINPUT=ON
cmake --build build-llvm-mingw-x64
```

Result: `build-llvm-mingw-x64/libSDL3.a`. Build it with the same llvm-mingw the app's Qt kit uses.

### 4. Android — `lib/arm64-v8a/libSDL3.so`, `lib/armeabi-v7a/libSDL3.so`

NDK r27d, once per ABI (`<ABI>` = `arm64-v8a`, then `armeabi-v7a`). `android-28` is the
`ANDROID_PLATFORM` Qt 6.8's own Android toolchain defaults to:

```
cmake -S SDL3-3.4.2 -B build-android-<ABI> <COMMON> -DCMAKE_TOOLCHAIN_FILE=<NDK>/build/cmake/android.toolchain.cmake -DANDROID_ABI=<ABI> -DANDROID_PLATFORM=android-28 -DANDROID_SUPPORT_FLEXIBLE_PAGE_SIZES=ON -DSDL_SHARED=ON -DSDL_STATIC=OFF -DSDL_VIDEO=ON -DSDL_ANDROID_JAR=OFF -DSDL_HIDAPI_LIBUSB=OFF
cmake --build build-android-<ABI>
<NDK>/toolchains/llvm/prebuilt/<host>/bin/llvm-strip --strip-unneeded -o libSDL3.so build-android-<ABI>/libSDL3.so
```

Check the result: `llvm-readelf -d` shows `SONAME libSDL3.so`; `llvm-readelf -l` on the arm64
library shows LOAD alignment `0x4000` (16 KB pages).

### 5. Linux — `lib/linux-x86_64/libSDL3.a`, `lib/linux-aarch64/libSDL3.a`

Packages (Ubuntu/Debian): `cmake ninja-build clang pkg-config libudev-dev`; cross-compiling for
aarch64 adds `gcc-aarch64-linux-gnu g++-aarch64-linux-gnu`. Without `libudev-dev` SDL builds
without udev hotplug (with it, SDL `dlopen`s `libudev.so.1` at run time — no link dependency).
Directory names on Linux name the OS and CPU, not the compiler: SDL is plain C, so the archive
links into the app whichever of GCC or Clang built it — what ties it to a system is glibc (below).
On Windows the name does carry the toolchain, because llvm-mingw (UCRT) and other MinGW or MSVC
builds are not interchangeable. It is built with the app's own Linux
compiler anyway — Clang 18.1.3 on Ubuntu 24.04, as in the top-level README — hence
`-DCMAKE_C_COMPILER=clang` below. Qt's own CMake and Ninja (`~/Qt/Tools/CMake/bin/cmake`,
`-DCMAKE_MAKE_PROGRAM=~/Qt/Tools/Ninja/ninja`) work in place of the system packages.

The archive is tied to the build host's glibc **at link time**. Built on Ubuntu 24.04 (glibc 2.39)
it references `__isoc23_strtol`/`__isoc23_sscanf`/… and `strlcpy`/`strlcat`, which exist only in
glibc ≥ 2.38: linking the app on an older distribution (Ubuntu 22.04 has 2.35) fails with undefined
references. That matches the app's own Linux baseline (the AppImage needs glibc 2.39); to support
an older system, rebuild SDL on it.

```bash
cmake -S SDL3-3.4.2 -B build-linux <COMMON> -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DSDL_SHARED=OFF -DSDL_STATIC=ON -DSDL_VIDEO=OFF -DSDL_UNIX_CONSOLE_BUILD=ON
cmake --build build-linux
strip --strip-debug build-linux/libSDL3.a
```

Run that on an x86_64 host for `linux-x86_64` with `-DCMAKE_C_COMPILER=clang` added, or on the
Raspberry Pi itself for `linux-aarch64`. To cross-compile for `linux-aarch64` from x86_64, add
`-DCMAKE_SYSTEM_NAME=Linux -DCMAKE_SYSTEM_PROCESSOR=aarch64 -DCMAKE_C_COMPILER=aarch64-linux-gnu-gcc -DCMAKE_CXX_COMPILER=aarch64-linux-gnu-g++`
and strip with `aarch64-linux-gnu-strip`. The same glibc rule applies to the Pi image.

Check the result: `readelf -p .comment` on the archive names the compiler, and
`nm -u libSDL3.a | grep -E '__isoc23_|strlcpy'` shows whether it needs glibc ≥ 2.38.

### 6. Copy into this directory

- `SDL3-3.4.2/include/SDL3/*.h` → `include/SDL3/` (replace the whole directory).
- `SDL3-3.4.2/LICENSE.txt` → `LICENSE.txt`.
- Each library → `lib/<kit>/`, file names as in the table above.

When the version changes, also update the version, URL and SHA-256 in this file, the SDL row in
`THIRD_PARTY_NOTICES.md` and on the About page (`qml/app/AboutPage.qml`), and replace the Android
Java glue in `platform/android/src/org/libsdl/app/` with the same release's
`android-project/app/src/main/java/org/libsdl/app/` (keeping the four replacement stubs). Re-check
that every native method and callback named in that release's `src/core/android/SDL_android.c`
exists in the Java classes with the same signature — a mismatch only shows up at run time on a
device.
