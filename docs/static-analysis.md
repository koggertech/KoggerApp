# Static Analysis

This project uses `qmake`, Qt 6, QML, and custom OpenGL rendering. The most practical static-analysis starting point is:

- `Clazy` for Qt-specific issues.
- `Clang-Tidy` for general C++ correctness and performance issues.

## Recommended rollout

1. Start with `Clazy` in Qt Creator on the current file or module.
2. Fix high-confidence Qt issues first.
3. Run `Clang-Tidy` on touched files.
4. Expand to whole-module analysis only after the warning volume is under control.

This order keeps the signal high on a large existing codebase.

## Why Clazy matters here

`Clazy` is especially valuable in this repository because the codebase has:

- many `QObject::connect(...)` calls;
- many `Q_PROPERTY` declarations exposed to QML;
- heavy use of Qt containers and implicit sharing;
- multi-threaded Qt code;
- Qt Quick / scene graph integration.

High-value checks for this project:

- `level0`
- `level1`
- `old-style-connect`

In practice, the checks we expect to pay off earliest are:

- `connect-3arg-lambda`
- `qproperty-without-notify`
- `incorrect-emit`
- `container-anti-pattern`
- `range-loop-detach`
- `readlock-detaching`
- `post-event`

Examples in the current codebase that make these checks relevant:

- lambda-based connects: `src/scene3d/domain/surface.cpp`
- many QML-facing properties: `src/core.h`
- lots of Qt threading/connect code: `src/core.cpp`, `src/device/device_manager.cpp`

## Qt Creator workflow

Qt Creator is the easiest way to use both tools with a `qmake` project.

1. Open `KoggerApp.pro`.
2. Configure and build the project once with your normal Qt kit.
3. Open `Analyze > Clang-Tidy or Clazy`.
4. Start with the current file or one submodule, not the whole project.

Recommended first pass:

- Run `Clazy` first.
- Then run `Clang-Tidy` using the repo `.clang-tidy`.

## Clazy with qmake from CLI

`Clazy` works well with `qmake` by compiling through the `clazy` wrapper.

Example:

```bash
export CLAZY_CHECKS="level0,level1,old-style-connect"
mkdir -p build-clazy
cd build-clazy
qmake -spec linux-clang ../KoggerApp.pro QMAKE_CXX=clazy
make -j"$(nproc)"
```

Notes:

- Re-run `qmake` after changing `CLAZY_CHECKS` or `QMAKE_CXX`.
- Use a separate build directory for analyzer runs.
- Start with one module if the warning volume is high.

## Clang-Tidy with qmake from CLI

`Clang-Tidy` needs a compilation database (`compile_commands.json`) for a comfortable CLI workflow.

For a `qmake` project, the usual approach is:

1. Create an out-of-source build directory.
2. Run `qmake`.
3. Build once while generating a compilation database with a tool such as `Bear`.
4. Run `clang-tidy -p <build-dir> <file>`.

Example:

```bash
mkdir -p build-clang-tidy
cd build-clang-tidy
qmake ../KoggerApp.pro
bear -- make -j"$(nproc)"
clang-tidy -p . ../src/core.cpp
```

If you do not want to maintain a compilation database manually, prefer running `Clang-Tidy` from Qt Creator.

## Suggested team policy

- Keep `.clang-tidy` in the repository.
- Treat `Clazy` as a developer tool first, not as a hard CI gate.
- Run both tools on changed files before larger refactors or rendering/threading changes.
- Add stricter checks only after the current baseline is mostly clean.

## Checks we intentionally did not enable yet

The starter `.clang-tidy` profile intentionally avoids style-heavy or noisy groups such as:

- broad `readability-*`
- broad `cppcoreguidelines-*`
- broad `modernize-*`

These can be added later once the project is comfortable with the baseline signal level.
