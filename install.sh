#!/usr/bin/env bash
# Install Caelestia Window Frames: builds and installs the window decoration,
# selects it, and sets up its small shell part in Caelestia.
# Double-click "Install Caelestia Window Frames" in this folder, or run ./install.sh.
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
    printf 'Nothing else was changed. You can scroll up to see the details.\n'
    finish
    exit 1
}
trap 'fail "the step above failed."' ERR

printf '%sCaelestia Window Frames - installer%s\n' "$bold" "$reset"
printf 'Window titlebars made from the same material as Caelestia'"'"'s bar and frame.\n'

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
    printf '  %sCaelestia was not found: the decoration will still work, but the part that\n  lines it up with Caelestia'"'"'s frame is skipped.%s\n' "$dim" "$reset"
fi
printf '  KDE Plasma %s found.\n' "$(pacman -Q kwin | cut -d' ' -f2)"

step "Asking for your password (needed to install the package)"
sudo -v || fail "the password was not accepted."

step "Building and installing the decoration (this takes a minute)"
./rebuild.sh >/tmp/caelestia-window-frames-build.log 2>&1 || {
    tail -n 25 /tmp/caelestia-window-frames-build.log
    fail "the build failed (full log: /tmp/caelestia-window-frames-build.log)."
}
printf '  Installed %s.\n' "$(pacman -Q caelestia-window-frames)"

step "Selecting it as your window decoration"
kwriteconfig6 --file kwinrc --group org.kde.kdecoration2 --key library org.caelestia.windowframes
kwriteconfig6 --file kwinrc --group org.kde.kdecoration2 --key theme ""
qdbus6 org.kde.KWin /KWin reconfigure >/dev/null 2>&1 || true
printf '  Done.\n'

if [ "$HAS_CAELESTIA" = 1 ]; then
    step "Setting up the Caelestia part"
    # Skipped when this folder already is the plugin (installed from Settings -> Plugins).
    PLUGIN="$CONFIG/caelestia/plugins/caelestia-window-frames"
    if [ "$(readlink -f "$PLUGIN" 2>/dev/null)" != "$REPO" ]; then
        mkdir -p "$PLUGIN"
        cp -r metadata.json LICENSE shell "$PLUGIN"/
    fi
    # The shell restarts to load it (and once more by itself after patching).
    sh "$PLUGIN/shell/restart-shell.sh" "$SHELL_DIR/shell.qml" || true
    printf '  Done.\n'
fi

trap - ERR
printf '\n%s%sCaelestia Window Frames is installed.%s\n' "$bold" "$green" "$reset"
printf 'KDE only loads a new window decoration when you log in, so log out and back in\nto see it. Save your work first.\n'
command -v notify-send >/dev/null && notify-send -a "Caelestia Window Frames" "Installed" "Log out and back in to see your new window frames." || true

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
    printf 'OK: log out whenever you are ready.\n'
fi
finish
