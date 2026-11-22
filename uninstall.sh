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
# Offer to log out: a KDE dialog when possible, else a question here.
log_out() {
    # Asked a moment after this window has closed: a terminal still running
    # this script would otherwise ask to confirm closing and cancel the log-out.
    if ! qdbus6 org.kde.Shutdown /Shutdown >/dev/null 2>&1; then
        printf 'Could not log out automatically: please log out from the menu.\n'
        return 1
    fi
    printf 'Logging out...\n'
    setsid sh -c "sleep 1; qdbus6 org.kde.Shutdown /Shutdown org.kde.Shutdown.logout" >/dev/null 2>&1 < /dev/null &
    exit 0
}
if [ -n "${WAYLAND_DISPLAY:-}${DISPLAY:-}" ] && command -v kdialog >/dev/null; then
    if kdialog --title "Caelestia Window Frames" --yes-label "Log out now" --no-label "Later" \
        --yesno "$(printf 'Caelestia Window Frames is removed.\n\nLog out now to finish switching back to Breeze?\n(Save your work first.)')"; then
        log_out
    fi
    printf 'OK: log out whenever you are ready.\n'
elif [ -t 0 ]; then
    printf '\nLog out now? [Y/n] '
    read -r answer || answer=n
    case "$answer" in
    [nN]*) printf 'OK: log out whenever you are ready.\n' ;;
    *) log_out ;;
    esac
fi
finish
