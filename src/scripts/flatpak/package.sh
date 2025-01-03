#!/bin/bash

rm -f Friction.flatpak || true
flatpak-builder builddir graphics.friction.Friction.json --force-clean
flatpak build-export export builddir
flatpak build-bundle export Friction.flatpak graphics.friction.Friction
