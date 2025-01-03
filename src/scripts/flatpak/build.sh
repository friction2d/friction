#!/bin/bash

flatpak-builder --user --install builddir --install-deps-from=flathub graphics.friction.Friction.json --force-clean

