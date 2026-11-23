#!/usr/bin/env bash
# Install Caelestia Window Frames: builds and installs the window decoration,
# selects it, and sets up its small shell part in Caelestia.
# Double-click "Install" in this folder, or run ./install.sh.
set -euo pipefail
cd "$(dirname "$(readlink -f "$0")")"
REPO=$(pwd)
FROM_LAUNCHER=0
[ "${1:-}" = "--from-launcher" ] && FROM_LAUNCHER=1

bold=$'\e[1m'; dim=$'\e[2m'; green=$'\e[32m'; red=$'\e[31m'; reset=$'\e[0m'
step() { printf '\n%s==> %s%s\n' "$bold" "$1" "$reset"; }
finish() {
    # Keep the window open when started by double-click.
    if [ "$FROM_LAUNCHER" = 1 ]; then
        printf '\n%sPress Enter to close this window.%s' "$dim" "$reset"
        read -r _ || true
    fi
}
fail() {
    printf '\n%sSomething went wrong: %s%s\n' "$red" "$1" "$reset"
    printf 'Nothing else changed. Scroll up for the details.\n'
    finish
    exit 1
}
trap 'fail "the step above failed."' ERR

printf '%sCaelestia Window Frames - installer%s\n' "$bold" "$reset"
printf 'Titlebars that match Caelestia'"'"'s bar and frame.\n'

step "Checking your system"
command -v pacman >/dev/null || fail "this installer needs an Arch-based system (Arch, CachyOS, EndeavourOS...)."
command -v kwin_wayland >/dev/null || fail "KDE Plasma (KWin) is not installed."
CONFIG=${XDG_CONFIG_HOME:-$HOME/.config}
SHELL_DIR="$CONFIG/quickshell/caelestia"
HAS_CAELESTIA=0
if [ -f "$SHELL_DIR/shell.qml" ]; then
    HAS_CAELESTIA=1
    printf '  Caelestia found.\n'
else
    printf '  %sCaelestia isn'"'"'t installed. The titlebars still work, they just won'"'"'t line up\n  with Caelestia'"'"'s frame.%s\n' "$dim" "$reset"
fi
printf '  KDE Plasma %s found.\n' "$(pacman -Q kwin | cut -d' ' -f2)"

step "Need your password to install the package"
sudo -v || fail "the password was not accepted."

step "Building it (takes a minute)"
./rebuild.sh >/tmp/caelestia-window-frames-build.log 2>&1 || {
    tail -n 25 /tmp/caelestia-window-frames-build.log
    fail "the build failed (full log: /tmp/caelestia-window-frames-build.log)."
}
printf '  Installed %s.\n' "$(pacman -Q caelestia-window-frames)"

step "Switching your window decoration to it"
kwriteconfig6 --file kwinrc --group org.kde.kdecoration2 --key library org.caelestia.windowframes
kwriteconfig6 --file kwinrc --group org.kde.kdecoration2 --key theme ""
qdbus6 org.kde.KWin /KWin reconfigure >/dev/null 2>&1 || true
printf '  Done.\n'

if [ "$HAS_CAELESTIA" = 1 ]; then
    step "Setting up the Caelestia side"
    # Skipped when this folder already is the plugin (installed from Settings -> Plugins).
    PLUGIN="$CONFIG/caelestia/plugins/caelestia-window-frames"
    if [ "$(readlink -f "$PLUGIN" 2>/dev/null)" != "$REPO" ]; then
        mkdir -p "$PLUGIN"
        cp -r metadata.json LICENSE shell "$PLUGIN"/
    fi
    # No shell restart here: it loads at the next login, with the decoration.
    printf '  Done (kicks in after you log back in).\n'
fi

trap - ERR
printf '\n%s%sAll done!%s\n' "$bold" "$green" "$reset"
printf 'KDE only picks up new window decorations at login, so log out and back in to\nsee it. Save your stuff first.\n'
command -v notify-send >/dev/null && notify-send -a "Caelestia Window Frames" "Installed" "Log out and back in to see it." || true

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
        --yesno "$(printf 'All done!\n\nKDE only picks up new window decorations at login. Log out now?\n(Save your stuff first.)')"; then
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
