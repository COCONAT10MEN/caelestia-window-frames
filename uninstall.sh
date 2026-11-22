#!/bin/sh
# Remove the Caelestia window decoration and its shell part, putting Breeze back.
set -e
CONFIG=${XDG_CONFIG_HOME:-$HOME/.config}
PLUGIN="$CONFIG/caelestia/plugins/caelestia-window-frames"
SHELL_DIR="$CONFIG/quickshell/caelestia"
kwriteconfig6 --file kwinrc --group org.kde.kdecoration2 --key library org.kde.breeze
qdbus6 org.kde.KWin /KWin reconfigure >/dev/null 2>&1 || true
if [ -f "$PLUGIN/shell/patch.py" ]; then
    python3 -I "$PLUGIN/shell/patch.py" revert "$SHELL_DIR"
    rm -rf "$PLUGIN"
    rm -f "${XDG_STATE_HOME:-$HOME/.local/state}/caelestia/window-frames-edge"
    sh "$(dirname "$0")/shell/restart-shell.sh" "$SHELL_DIR/shell.qml" 2>/dev/null || true
fi
if pacman -Q caelestia-window-frames >/dev/null 2>&1; then
    sudo pacman -R --noconfirm caelestia-window-frames
fi
echo "Removed. Log out and back in to finish."
