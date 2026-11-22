#!/bin/sh
# Rebuild and reinstall caelestia-window-frames. Run it after a Plasma/KWin
# update: decorations are built against one KWin version, and a stale build
# makes KWin fall back to its default frames.
set -e
cd "$(dirname "$0")/packaging"
rm -rf src pkg build ./*.pkg.tar.*
makepkg -sfi --noconfirm
echo "caelestia-window-frames rebuilt for KWin $(pacman -Q kwin | cut -d' ' -f2)."
# KWin keeps the decoration it loaded at login in memory, so the new build
# is used from the next login.
echo "Log out and back in to use the new build."
