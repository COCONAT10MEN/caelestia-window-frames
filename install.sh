#!/bin/sh
# Build, install and select the Caelestia window decoration.
set -e
cd "$(dirname "$0")"
./rebuild.sh
kwriteconfig6 --file kwinrc --group org.kde.kdecoration2 --key library org.caelestia.windowframes
kwriteconfig6 --file kwinrc --group org.kde.kdecoration2 --key theme ""
qdbus6 org.kde.KWin /KWin reconfigure >/dev/null 2>&1 || true
echo "Caelestia window frames selected. Log out and back in to see them."
