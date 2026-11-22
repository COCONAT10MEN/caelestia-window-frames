#!/usr/bin/env bash
# Remove Caelestia Window Frames and its Caelestia part, putting Breeze back.
# Double-click "Uninstall Caelestia Window Frames" in this folder, or run ./uninstall.sh.
set -uo pipefail
cd "$(dirname "$(readlink -f "$0")")"
FROM_LAUNCHER=0
[ "${1:-}" = "--from-launcher" ] && FROM_LAUNCHER=1

bold=$'\e[1m'; dim=$'\e[2m'; green=$'\e[32m'; reset=$'\e[0m'
step() { printf '\n%s==> %s%s\n' "$bold" "$1" "$reset"; }
finish() {
    if [ "$FROM_LAUNCHER" = 1 ]; then
        printf '\n%sPress Enter to close this window.%s' "$dim" "$reset"
        read -r _ || true
    fi
}

printf '%sCaelestia Window Frames - uninstaller%s\n' "$bold" "$reset"
CONFIG=${XDG_CONFIG_HOME:-$HOME/.config}
PLUGIN="$CONFIG/caelestia/plugins/caelestia-window-frames"
SHELL_DIR="$CONFIG/quickshell/caelestia"

step "Switching back to Breeze"
kwriteconfig6 --file kwinrc --group org.kde.kdecoration2 --key library org.kde.breeze
qdbus6 org.kde.KWin /KWin reconfigure >/dev/null 2>&1 || true
printf '  Done.\n'

if [ -f "$PLUGIN/shell/patch.py" ]; then
    step "Removing the Caelestia part"
    python3 -I "$PLUGIN/shell/patch.py" revert "$SHELL_DIR" >/dev/null || true
    RESTART="$PLUGIN/shell/restart-shell.sh"
    cp "$RESTART" /tmp/caelestia-window-frames-restart.sh 2>/dev/null || true
    rm -rf "$PLUGIN"
    rm -f "${XDG_STATE_HOME:-$HOME/.local/state}/caelestia/window-frames-edge"
    sh /tmp/caelestia-window-frames-restart.sh "$SHELL_DIR/shell.qml" 2>/dev/null || true
    printf '  Done: Caelestia is back to its original files.\n'
fi

if pacman -Q caelestia-window-frames >/dev/null 2>&1; then
    step "Removing the package (asks for your password)"
    sudo pacman -R --noconfirm caelestia-window-frames && printf '  Done.\n'
fi

printf '\n%s%sCaelestia Window Frames is removed.%s\n' "$bold" "$green" "$reset"
printf 'Log out and back in to finish switching back to Breeze.\n'
if [ -t 0 ]; then
    printf '\nLog out now? [y/N] '
    read -r answer || answer=n
    case "$answer" in
    [yY]*)
        qdbus6 org.kde.Shutdown /Shutdown org.kde.Shutdown.logout >/dev/null 2>&1 \
            || printf 'Could not log out automatically: please log out from the menu.\n'
        exit 0
        ;;
    esac
fi
finish
