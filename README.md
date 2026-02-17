# Friction

[![Windows](https://github.com/friction2d/friction/actions/workflows/windows.yml/badge.svg)](https://github.com/friction2d/friction/actions/workflows/windows.yml?query=branch%3Amain) [![Linux](https://github.com/friction2d/friction/actions/workflows/linux.yml/badge.svg)](https://github.com/friction2d/friction/actions/workflows/linux.yml?query=branch%3Amain) [![macOS](https://github.com/friction2d/friction/actions/workflows/macos.yml/badge.svg)](https://github.com/friction2d/friction/actions/workflows/macos.yml?query=branch%3Amain)

[Friction](https://friction.graphics) is a powerful and versatile motion graphics application that allows you to create vector and raster animations for web and video.

![Screenshot of Friction 1.0.0](https://friction.graphics/assets/screenshots/100/friction-100rc2-screenshot.png?v=1)

## Contribute

We accept any contributions, big or small. Before submitting a pull request it's recommended that you communicate with the developers first (on [GitHub](https://github.com/friction2d/friction/issues) or [Codeberg](https://codeberg.org/friction/friction/issues)).

It's always preferred to submit pull requests against the `main` branch.

## Branches and versions

Friction uses `X.Y.Z` version numbers and `vX.Y` branches.

* `X` = Major
* `Y` = Minor
* `Z` = Patch

Branch `main` is always the current branch for the next `X` or `Y` release.

A new stable branch is cut from `main` on each `X` or `Y` release and is maintained until a new stable branch is created. Patch (`Z`) releases comes from the parent stable branch (`vX.Y`).

Important fixes added to `main` will be backported to active stable branches when possible (and within reason).

## Documentation

See https://friction.graphics/documentation for generic documentation.

### Build instructions

* [Linux](https://friction.graphics/documentation/source-linux.html)
* [Windows](https://friction.graphics/documentation/source-windows.html)
* [macOS](https://friction.graphics/documentation/source-macos.html)

## License (GPL-3.0-only)

Friction is copyright &copy; Ole-André Rodlie and contributors.

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 3.

**This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the [GNU General Public License](LICENSE.md) for more details.**

Friction is based on [enve](https://github.com/MaurycyLiebner/enve) - Copyright &copy; Maurycy Liebner and contributors.

Third-party software may contain other OSS licenses, see 'Help' > 'About' > 'Licenses' in Friction.

Source code for third-party software can be downloaded [here](https://download.friction.graphics/distfiles/).
