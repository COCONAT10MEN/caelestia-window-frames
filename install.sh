#!/bin/sh
# Build, install and select the Caelestia window decoration, and install its
# small shell part into Caelestia's plugins.
set -e
cd "$(dirname "$0")"
REPO=$(pwd)
./rebuild.sh
kwriteconfig6 --file kwinrc --group org.kde.kdecoration2 --key library org.caelestia.windowframes
kwriteconfig6 --file kwinrc --group org.kde.kdecoration2 --key theme ""
qdbus6 org.kde.KWin /KWin reconfigure >/dev/null 2>&1 || true

# The shell part (see shell/patch.py). Skipped when this folder already is the
# plugin, i.e. it was installed from Caelestia's Settings -> Plugins.
CONFIG=${XDG_CONFIG_HOME:-$HOME/.config}
PLUGIN="$CONFIG/caelestia/plugins/caelestia-window-frames"
if [ "$(readlink -f "$PLUGIN" 2>/dev/null)" != "$REPO" ]; then
    mkdir -p "$PLUGIN"
    cp -r metadata.json LICENSE shell "$PLUGIN"/
fi
# Restart the shell so it loads the plugin; the plugin patches the frame and
# restarts it once more by itself.
SHELL_QML="$CONFIG/quickshell/caelestia/shell.qml"
[ -f "$SHELL_QML" ] && sh "$PLUGIN/shell/restart-shell.sh" "$SHELL_QML" || true

echo "Caelestia window frames installed. Log out and back in to see them."
