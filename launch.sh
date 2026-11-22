#!/usr/bin/env bash
# Opens install.sh or uninstall.sh in a terminal window, for the double-click
# launchers. Uses KDE's chosen terminal when it is installed, otherwise the
# first common one found (KDE's setting can point at one that is missing).
# Usage: launch.sh install|uninstall
cd "$(dirname "$(readlink -f "$0")")"
case "${1:-}" in
install) script="$PWD/install.sh" ;;
uninstall) script="$PWD/uninstall.sh" ;;
*) echo "usage: $0 install|uninstall" >&2; exit 2 ;;
esac

preferred=$(kreadconfig6 --file kdeglobals --group General --key TerminalApplication 2>/dev/null | awk '{print $1}')
for term in "$preferred" konsole kitty alacritty ghostty wezterm foot gnome-terminal kgx xfce4-terminal xterm; do
    [ -n "$term" ] && command -v "$term" >/dev/null || continue
    case "$(basename "$term")" in
    konsole) exec konsole -e bash "$script" --from-launcher ;;
    kitty | foot) exec "$term" bash "$script" --from-launcher ;;
    wezterm) exec wezterm start -- bash "$script" --from-launcher ;;
    gnome-terminal | kgx) exec "$term" -- bash "$script" --from-launcher ;;
    xfce4-terminal) exec xfce4-terminal -x bash "$script" --from-launcher ;;
    *) exec "$term" -e bash "$script" --from-launcher ;;
    esac
done

msg="No terminal app was found. Open a terminal in this folder and run: ./$(basename "$script")"
command -v kdialog >/dev/null && kdialog --error "$msg" || echo "$msg" >&2
exit 1
