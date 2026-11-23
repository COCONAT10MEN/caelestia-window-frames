#!/usr/bin/env bash
# Remove Caelestia Window Frames and its Caelestia part, putting Breeze back.
# Double-click "Uninstall" in this folder, or run ./uninstall.sh.
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
    rm -rf "$PLUGIN"
    rm -f "${XDG_STATE_HOME:-$HOME/.local/state}/caelestia/window-frames-edge"
    printf '  Done. Caelestia'"'"'s files are back to stock.\n'
fi

if pacman -Q caelestia-window-frames >/dev/null 2>&1; then
    step "Removing the package (needs your password)"
    sudo pacman -R --noconfirm caelestia-window-frames && printf '  Done.\n'
fi

printf '\n%s%sRemoved.%s\n' "$bold" "$green" "$reset"
printf 'Log out and back in to finish switching back to Breeze.\n'
# Offer to log out: a KDE dialog when possible, else a question here.
log_out() {
    # Runs on its own after this window has closed (a terminal still running
    # this script could ask to confirm closing and cancel the log-out). It is
    # started as a separate user service: KDE runs a double-clicked launcher
    # as a service of its own, and ends every process in it when the window
    # closes. It asks KDE to log out, checks that it really started, tries once
    # more, and as a last resort opens KDE's own log-out dialog. Steps go to a
    # log file.
    local log="${XDG_CACHE_HOME:-$HOME/.cache}/caelestia-window-frames-logout.log"
    mkdir -p "$(dirname "$log")"
    printf 'Logging out...\n'
    local helper='
        log="$1"
        started() { [ "$(qdbus6 org.kde.ksmserver /KSMServer org.kde.KSMServerInterface.isShuttingDown 2>/dev/null)" = true ]; }
        echo "$(date +%T) log-out requested" >"$log"
        sleep 2
        for attempt in 1 2; do
            qdbus6 org.kde.Shutdown /Shutdown org.kde.Shutdown.logout >>"$log" 2>&1
            echo "$(date +%T) attempt $attempt sent (exit $?)" >>"$log"
            sleep 5
            started && { echo "$(date +%T) logging out" >>"$log"; exit 0; }
        done
        echo "$(date +%T) not started: opening KDE'"'"'s log-out dialog" >>"$log"
        qdbus6 org.kde.LogoutPrompt /LogoutPrompt org.kde.LogoutPrompt.promptLogout >>"$log" 2>&1
    '
    if command -v systemd-run >/dev/null; then
        systemd-run --user --collect --quiet --unit="caelestia-window-frames-logout-$$" \
            bash -c "$helper" _ "$log" >/dev/null 2>&1 && exit 0
    fi
    setsid bash -c "$helper" _ "$log" >/dev/null 2>&1 </dev/null &
    exit 0
}
if [ -n "${WAYLAND_DISPLAY:-}${DISPLAY:-}" ] && command -v kdialog >/dev/null; then
    if kdialog --title "Caelestia Window Frames" --yes-label "Log out now" --no-label "Later" \
        --yesno "$(printf 'Removed.\n\nLog out now to finish switching back to Breeze?\n(Save your stuff first.)')"; then
        log_out
    fi
    printf 'No worries, log out whenever.\n'
elif [ -t 0 ]; then
    printf '\nLog out now? [Y/n] '
    read -r answer || answer=n
    case "$answer" in
    [nN]*) printf 'No worries, log out whenever.\n' ;;
    *) log_out ;;
    esac
fi
finish
