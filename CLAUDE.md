# Friction

## Building

- **Debug build**: `just build-debug` (builds to `build-debug-arm64/`)
- **Release build**: `just build-mac-arm` (builds to `build-release-arm64/`)

Use `just build-debug` when iterating on code changes. Use `just build-mac-arm` for release/packaging.

## Pull Requests

- Always create PRs against the fork (`earlye/friction`), never against the upstream (`friction2d/friction`).

## Pulling Upstream Changes

- When incorporating upstream changes, use `git blame` to compare authorship and understand what changed.
- Prefer `QLoggingCategory`-based logging when it is already present in the fork; do not remove it in favor of plain `qDebug()`.
- When done incorporating upstream changes, update the list of incorporated upstream commits below.

- The last upstream commit before this fork was created: `3701b0559990eae4fa315b7215a11d46122cb97b` ("Update skia")
- The last upstream commit incorporated into this fork: `9f0ef659581e080fca88639331eb141225c45807` ("Update canvas.cpp")
