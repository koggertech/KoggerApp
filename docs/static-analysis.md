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

## Reduce Clazy noise (important)

For `Analyze > Clang-Tidy or Clazy` in Qt Creator, the most important place is
`Projects > Project Settings > Clang Tools` (project-local diagnostic configuration).

Use this setup first:

1. Open `Projects > Project Settings > Clang Tools`.
2. Clear `Use global settings`.
3. Open your `Diagnostic configuration`.
4. In `Clang Warnings`, disable `Use diagnostic flags from the build system`.
5. In `Clang Warnings`, add `-Wno-character-conversion`.
6. In `Clazy Checks > Edit Checks as String`, use this low-noise baseline:

```text
level0,old-style-connect,qproperty-without-notify,incorrect-emit,post-event,range-loop-detach,readlock-detaching,container-anti-pattern,no-const-signal-or-slot
```

Expected result for a typical file analysis: Qt header spam from `qchar.h` is gone,
and only project warnings remain (for example `Q_PROPERTY ... without NOTIFY`).

Important: this profile intentionally trades coverage for lower noise. It can hide
real project warnings (for example `const-signal-or-slot`) if they are disabled.

## Two-profile workflow (recommended)

Use two analyzer profiles in Qt Creator:

1. `Low-noise (daily)`:
- fast checks while coding;
- good for current file / touched module;
- allows temporary suppression of noisy checks.

2. `Audit (strict)`:
- run regularly (for example once per sprint, before release, or before big merges);
- run on the whole touched subsystem;
- do not disable checks that can find real project issues.

Suggested strict `Clazy Checks` string:

```text
level0,level1,old-style-connect,qproperty-without-notify,incorrect-emit,post-event,range-loop-detach,readlock-detaching,container-anti-pattern,const-signal-or-slot
```

For the strict run, avoid adding `no-const-signal-or-slot`.

## Triage rules for strict runs

- Warning points to project file under `src/`: fix it or create a tracked task.
- Warning points to Qt/system headers: treat as toolchain noise unless it affects behavior.
- If a warning is noisy but valid, prefer narrowing scope/header filters over globally disabling the check.

### Build Environment variables (optional)

These are useful for `CLI` / `qmake QMAKE_CXX=clazy` / `CI`.
For Qt Creator analyze runs they are optional and may not fully control diagnostics.

If you still want them, set them as **three separate variables**:

```bash
CLAZY_HEADER_FILTER=(^|.*/)(KoggerApp/(src|platform)/).*
CLAZY_IGNORE_DIRS=.*/Qt/[^/]+/.*|.*/usr/include/.*
CLAZY_CHECKS=level0,old-style-connect,qproperty-without-notify,incorrect-emit,post-event,range-loop-detach,readlock-detaching,container-anti-pattern
```

## Clazy with qmake from CLI

`Clazy` works well with `qmake` by compiling through the `clazy` wrapper.

Example:

```bash
export CLAZY_CHECKS="level0,old-style-connect,qproperty-without-notify,incorrect-emit,post-event,range-loop-detach,readlock-detaching,container-anti-pattern"
export CLAZY_HEADER_FILTER='(^|.*/)(KoggerApp/(src|platform)/).*'
export CLAZY_IGNORE_DIRS='.*/Qt/[^/]+/.*|.*/usr/include/.*'
mkdir -p build-clazy
cd build-clazy
qmake -spec linux-clang ../KoggerApp.pro QMAKE_CXX=clazy "QMAKE_CXXFLAGS+=-Wno-character-conversion"
make -j"$(nproc)"
```

Notes:

- Re-run `qmake` after changing `CLAZY_CHECKS` or `QMAKE_CXX`.
- Re-run `qmake` after changing `CLAZY_HEADER_FILTER` / `CLAZY_IGNORE_DIRS` too.
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
- Keep two Qt Creator profiles: `Low-noise (daily)` and `Audit (strict)`.
- Require a periodic strict pass to catch warnings hidden by low-noise settings.
- Add stricter checks only after the current baseline is mostly clean.

## Checks we intentionally did not enable yet

The starter `.clang-tidy` profile intentionally avoids style-heavy or noisy groups such as:

- broad `readability-*`
- broad `cppcoreguidelines-*`
- broad `modernize-*`

These can be added later once the project is comfortable with the baseline signal level.
