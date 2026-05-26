# earlye/friction Fork Changes

This document tracks changes landed in [earlye/friction](https://github.com/earlye/friction) relative to upstream [friction2d/friction](https://github.com/friction2d/friction).

## New Features

### macOS Build Support

**Add Justfile for macOS Homebrew-based builds** — `just build-debug`
  and `just build-mac-arm` recipes for macOS development without
  manual CMake invocations. Includes `build-debug` and `run-debug`
  targets.

- [#2](https://github.com/earlye/friction/pull/2), [#7](https://github.com/earlye/friction/pull/7)

### SVG-Driven Animation System (SvgElementTrack)

A new animation targeting system that reads YAML annotations from SVG
`<desc>` elements and automatically creates animation tracks bound to
SVG elements by ID.

- **Per-element animation targeting for SvgLinkBox** — each linked SVG
    element can have independent animation
    properties. [#4](https://github.com/earlye/friction/pull/4)

- **Animation track support** — adds `SvgElementTrack`, a new track
    type that binds keyframe animation to SVG
    elements. [#8](https://github.com/earlye/friction/pull/8),
    [#9](https://github.com/earlye/friction/pull/9)

- **`<desc>` YAML auto-creates SvgElementTracks** — place
    `animation-node` markers in SVG `<desc>` tags to auto-generate
    tracks on
    import. [#13](https://github.com/earlye/friction/pull/13)

- **Flipbook track support for SvgLinkBox** — `<desc>` YAML can
    declare a flipbook track that steps through SVG
    pages. [#14](https://github.com/earlye/friction/pull/14)

- **`kind:pivot` SVG desc tag** — declares a pivot point element;
    SvgElementTracks are trimmed to transform-only for pivot
    elements. [#23](https://github.com/earlye/friction/pull/23)

### Camera as a first class entity

Added an "add camera" button, and implemented support for having a
camera as a first class entity that can be translated within the
scene.  If there is a camera, it is wrapped by a Cameras flipbook, so
that specific cameras can be selected, and switched by an animation
track. (No transition effects for now - just hard cuts). Pressing 'C'
toggles between "look through camera" and "look at world."

Introduced in [#29](https://github.com/earlye/friction/pull/29), which
added `CameraBox`, the `cameraCreate` canvas mode, and
`getActiveCameraRect` with multi-camera flipbook selection.

### Lock Entity UX

- **Flash lock icon on blocked modification** — when the user attempts
    to modify a locked entity, the lock icon flashes to indicate why
    the operation
    failed. [#35](https://github.com/earlye/friction/pull/35)

### Keyboard Shortcuts

- **Remap Add Keyframe → `K`; Split Clip → `Shift+K`** across all
    platforms. [#39](https://github.com/earlye/friction/pull/39)

### Categorized Debug Logging

- **Replace `qDebug()` with `qCDebug` categories** throughout the
    codebase, enabling per-subsystem log
    filtering. [#16](https://github.com/earlye/friction/pull/16)

### SemVer 2.0 Build Versioning

- **Structured build metadata** using SemVer 2.0 (`X.Y.Z+build.N`),
    replacing the prior ad hoc version
    strings. [#21](https://github.com/earlye/friction/pull/21)

### CI / Release Automation

- **macOS CI: SDK caching, concurrency, named artifacts.**
    [#1](https://github.com/earlye/friction/pull/1)

- **Release workflow** — on each merge to `main`, builds all platform
    artifacts and creates a GitHub release
    automatically. [#22](https://github.com/earlye/friction/pull/22)

- **Parallel macOS CI** — arm64 and x86_64 builds run as separate
    parallel jobs. [#33](https://github.com/earlye/friction/pull/33)


### Developer Tooling

- **`just run-debug-with-logs`** — config-driven debug sessions with
    log category filtering via
    `.claude/logs.local.json`. [#40](https://github.com/earlye/friction/pull/40)

- **`just index`** — ctags-based symbol index for IDE/AI
    navigation. [#44](https://github.com/earlye/friction/pull/44)

- **`just index` includes CodeGraph** — runs `codegraph init` as part
    of the index
    build. [#49](https://github.com/earlye/friction/pull/49)

- **`just start-worktree`** — launches a named tmux window for a
    Claude Code worktree
    session. [#50](https://github.com/earlye/friction/pull/50)


### Audio Waveform in Animation Timeline

- **Audio waveform visualization** — the timeline now renders the
    decoded audio waveform behind clip regions, giving visual tempo
    cues for keyframe
    placement. [#56](https://github.com/earlye/friction/pull/56)

## Bug Fixes

Bug entries are annotated with their likely origin:
**[fork-introduced]** means the bug was brought in by this fork's own
changes; **[pre-existing]** means the bug was present in upstream code
before the fork.

### CI / Build

| # | Fix | Origin |
|---|-----|--------|
| [#3](https://github.com/earlye/friction/pull/3) | Fix macOS CI not running on `main` branch pushes | fork-introduced — CI workflow was added by this fork |
| [#6](https://github.com/earlye/friction/pull/6) | Fix Linux CI for push events and branch names containing `+` | fork-introduced — Linux CI workflow was added by this fork |
| [#12](https://github.com/earlye/friction/pull/12) | Restrict CI push triggers to `main`; eliminate duplicate PR builds | fork-introduced — trigger logic was set up by this fork |
| [#26](https://github.com/earlye/friction/pull/26) | Fix reusable workflow concurrency groups cancelling release builds | fork-introduced — release workflow was added by this fork |
| [#27](https://github.com/earlye/friction/pull/27) | Fix Linux/macOS sharing same concurrency group within a release run | fork-introduced — same |
| [#28](https://github.com/earlye/friction/pull/28) | Fix release artifact glob: Linux artifact has a version subdirectory | fork-introduced — same |
| [#42](https://github.com/earlye/friction/pull/42) | Fix shallow checkout causing wrong commit count in build version | fork-introduced — SemVer CI setup introduced the shallow-clone assumption |
| [#43](https://github.com/earlye/friction/pull/43) | Optimize CI: SDK/Docker caching, `MKJOBS=4`, DMG arch naming, 7z `-mx5` | fork-introduced — CI infrastructure owned by this fork |

### Rendering / Playback

| # | Fix | Origin |
|---|-----|--------|
| [#15](https://github.com/earlye/friction/pull/15) | Fix render output hang: suppress `mStateId++` during output rendering | likely fork-introduced — render pipeline was modified by SvgElementTrack work |
| [#19](https://github.com/earlye/friction/pull/19) | Fix preview black screen: suppress `mStateId++` during preview rendering | likely fork-introduced — same render pipeline changes |
| [#18](https://github.com/earlye/friction/pull/18) | Fix crash in VideoEncoder: `sws` context dimension mismatch and image use-after-free | pre-existing — bug was in the existing upstream VideoEncoder code |
| [#53](https://github.com/earlye/friction/pull/53) | Fix re-render doing nothing after first render completes | likely fork-introduced — render state management changed by animation work |

### Camera / Viewport

| # | Fix | Origin |
|---|-----|--------|
| [#31](https://github.com/earlye/friction/pull/31) | Fix camera box drawn at wrong position when camera is active viewport | fork-introduced — same |
| [#38](https://github.com/earlye/friction/pull/38) | Fix C-toggle clip state ignored when camera is active | fork-introduced — camera mode was added by this fork |
| [#41](https://github.com/earlye/friction/pull/41) | Fix active camera box hover/selection in canvas viewport | fork-introduced — same |
| [#46](https://github.com/earlye/friction/pull/46) | Fix double camera transform on SvgElementTrack elements after timeline scrub | fork-introduced — SvgElementTrack + camera interaction is entirely fork code |

### SvgElementTrack / Flipbook

| # | Fix | Origin |
|---|-----|--------|
| [#20](https://github.com/earlye/friction/pull/20) | Fix FlipBook `<desc>` index: use step-function instead of interpolation | fork-introduced — flipbook track is a fork feature |
| [#24](https://github.com/earlye/friction/pull/24) | Fix `kind:pivot` not taking effect due to deferred center-pivot overwrite | fork-introduced — `kind:pivot` is a fork feature |
| [#30](https://github.com/earlye/friction/pull/30) | Fix SvgElementTrack `syncToTarget` accumulating stale keyframes | fork-introduced — SvgElementTrack is a fork feature |
| [#32](https://github.com/earlye/friction/pull/32) | Fix SvgElementTrack including transform effects in animation track tree | fork-introduced — same |
| [#48](https://github.com/earlye/friction/pull/48) | Fix flipbook index field not triggering canvas redraw on edit | fork-introduced — same |
| [#55](https://github.com/earlye/friction/pull/55) | Fix flipbook page lookup ignoring `inkscape:label` when `id` is absent | fork-introduced — same |

### Lock System

| # | Fix | Origin |
|---|-----|--------|
| [#36](https://github.com/earlye/friction/pull/36), [#37](https://github.com/earlye/friction/pull/37) | Fix locked entity children allowing slider drag and manual typing | fork-introduced — enhanced locking UX is a fork feature |
| [#47](https://github.com/earlye/friction/pull/47) | Fix keyframe deletion and movement ignoring object lock state | fork-introduced — same |

### Compiler Warnings

| # | Fix | Origin |
|---|-----|--------|
| [#11](https://github.com/earlye/friction/pull/11) | Fix `-Winconsistent-missing-override` warning in SvgLinkBox | pre-existing — warning was in existing upstream SvgLinkBox |
| [#17](https://github.com/earlye/friction/pull/17) | Fix `-Wunused-but-set-variable` warnings across codebase | pre-existing — warnings were in existing upstream code |

### Developer Tooling

| # | Fix | Origin |
|---|-----|--------|
| [#45](https://github.com/earlye/friction/pull/45) | Fix `just index` to use Homebrew `universal-ctags` on macOS | fork-introduced — `just index` recipe is a fork addition |
| [#51](https://github.com/earlye/friction/pull/51) | Set tmux window name in `start-worktree` recipe | fork-introduced — recipe is a fork addition |
| [#54](https://github.com/earlye/friction/pull/54) | Fix `just index` recipe handling stale CodeGraph state | fork-introduced — same |
