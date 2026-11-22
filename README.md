<div align="center">

<img src="assets/icon.svg" width="80" alt="Caelestia window frames icon" />

# caelestia-window-frames

[![Caelestia KDE](https://img.shields.io/badge/Caelestia_KDE-2.7.5_tested-9bd0cc?style=for-the-badge&labelColor=101418)](https://github.com/caelestia-kde/caelestia)
[![KDE Plasma](https://img.shields.io/badge/Plasma_6.8-1D99F3?logo=kde&logoColor=white&style=for-the-badge&labelColor=101418)](https://kde.org/plasma-desktop)
[![Version](https://img.shields.io/badge/Version-1.2.1-9bd0cc?style=for-the-badge&labelColor=101418)](CMakeLists.txt)
[![License: MIT](https://img.shields.io/badge/License-MIT-9bd0cc?style=for-the-badge&labelColor=101418)](LICENSE)

</div>

> [!NOTE]
> **✅ Tested and working on Caelestia KDE 2.7.5** (Plasma 6.8.3, KWin 6.8.3, Wayland).
> Other versions may work too. After a Plasma update, run `./rebuild.sh` once (see below).

A KWin window decoration for [Caelestia KDE](https://github.com/caelestia-kde/caelestia):
window titlebars made from the same material as Caelestia's own bar and screen frame,
instead of a separate theme with its own opacity settings.

## What it does

- **Caelestia's colours, live.** The titlebar is Caelestia's Material You `surface` colour,
  read from the scheme Caelestia writes. Change the wallpaper or switch light/dark and every
  window follows within a moment, no KWin restart.
- **Caelestia's transparency and blur.** The titlebar is built the way Caelestia builds its
  frame: `surface` at the shell's base transparency (0.1 lower in light mode), with KWin's
  blur behind it, the same blur the bar uses. Transparency off in Caelestia makes the frames
  solid; "pitch black" makes them black.
- **Meets the screen frame.** Where a titlebar touches Caelestia's frame (maximized, or tiled
  against the top), it takes in the app's own colour the way the frame's blur does, so the two
  read as one surface. At the frame's rounded corners the titlebar runs under the curve and
  blends with it, and a small shell part lines the frame's edge up with the window to the
  pixel, so no thin line of wallpaper shows between them. The frame's shadow still falls on
  the wallpaper, but no longer darkens the top of a window that touches the frame.
- **Material 3 Expressive buttons.** Thin, pixel-sharp minimize, maximize/restore and close.
  Round at rest; on hover a state layer fades in, on press a ripple spreads and the shape
  morphs to a rounded square. Close turns the theme's error colour. KDE's button order
  (System Settings) is respected.
- **Caelestia's motion.** Every animation uses Caelestia's curves and durations, and follows
  its animation speed and on/off settings.
- **Layout.** App icon, title and a small arrow that opens the window menu. A thin outline
  around windowed windows only. Square corners where a tiled window meets its tile edge, so
  tiles fit together. No titlebar at all in fullscreen.

## Installation

**Requirements:** Arch-based system (CachyOS, Arch, EndeavourOS…), Plasma 6 on Wayland,
Caelestia KDE (tested on 2.7.5). The build pulls in `cmake`, `extra-cmake-modules`,
`kdecoration`, `kwin`, `qt6-base`, `kcoreaddons` and `ttf-material-symbols-variable`.

```bash
git clone https://github.com/COCONAT10MEN/caelestia-window-frames
cd caelestia-window-frames
./install.sh
```

`install.sh` builds a local pacman package, installs and selects the decoration, and puts its
small shell part into Caelestia's plugins (`~/.config/caelestia/plugins/caelestia-window-frames`).
Log out and back in: KWin only loads a new decoration build at login.

The repository is also a Caelestia plugin: installing it from *Settings → Plugins* with the URL
above adds the shell part only. Run `./install.sh` from that folder for the decoration itself.

You can also pick it by hand in *System Settings → Colors & Themes → Window Decorations →
Caelestia*.

### Updating

```bash
cd caelestia-window-frames
git pull
./install.sh
```

Then log out and back in.

### After a Plasma update

A window decoration is built against one KWin version. When Plasma updates, KWin may fall
back to its default frames until the decoration is rebuilt:

```bash
cd caelestia-window-frames
./rebuild.sh
```

Then log out and back in.

### Switching back to Breeze

Pick *Breeze* in *System Settings → Colors & Themes → Window Decorations*, or:

```bash
kwriteconfig6 --file kwinrc --group org.kde.kdecoration2 --key library org.kde.breeze
qdbus6 org.kde.KWin /KWin reconfigure
```

### Uninstalling

```bash
cd caelestia-window-frames
./uninstall.sh
```

It switches back to Breeze, undoes the shell part's one-line change to Caelestia, removes the
plugin folder and the package. Then log out and back in.

## Configuring

Nothing to configure here: it follows Caelestia's own settings (colours, transparency, blur,
rounding scale, animation speed, bar position) and KDE's button order and title font.

## Troubleshooting

| Problem | Fix |
| --- | --- |
| Windows show KDE's default frames after an update | Run `./rebuild.sh`, then log out and back in |
| A change to the decoration does not show | KWin keeps the decoration it loaded at login: log out and back in |
| Icons look wrong | Make sure `ttf-material-symbols-variable` is installed |
| "Could not patch the screen frame's edge" notification | Caelestia changed the line the shell part edits; nothing was touched. Check for an update of this repository |
| Check the shell part | `python3 ~/.config/caelestia/plugins/caelestia-window-frames/shell/patch.py status ~/.config/quickshell/caelestia` |

## How it works

- `src/theme.cpp` reads `~/.local/state/caelestia/scheme.json` (colours, light/dark) and
  `~/.config/caelestia/shell.json` (transparency, blur, rounding, animations, bar position)
  and watches both, so every window repaints when Caelestia changes them. One reader is
  shared by all windows.
- `src/decoration.cpp` draws the titlebar, asks KWin to blur behind it, and sets the window's
  corner shape and outline. Where a titlebar touches Caelestia's frame, it mixes in the app's
  own background colour (from the window's palette) by the share Caelestia's blur takes in at
  that point. Measured on Caelestia 2.7.5: about 69% along the top, easing to about a third
  next to the bar.
- `src/button.cpp`, `src/statelayer.cpp` and `src/icons.cpp` draw the buttons, their state
  layers and the stroke icons; `src/motion.cpp` holds Caelestia's motion tokens.
- `shell/` is the shell part, a Caelestia plugin. Caelestia's plugin API has no hook for the
  screen frame, so on every shell start it makes one clearly marked edit to
  `modules/drawers/ContentWindow.qml`: the frame's inner edge moves half a pixel over the
  windows, so the frame's soft edge fully covers its own last pixel row instead of letting a
  line of wallpaper through next to a window (on the empty desktop half a pixel is not
  visible). And it draws the frame's shadow on its own layer, with the frame's own shadow
  effect (same blur, same strength) masked by the frame and by every window, so it falls on
  the wallpaper exactly as before but never on a window. Rounded corners and blur are
  untouched. Every changed line keeps its original in a marker comment
  (`/*caelestia-window-frames was: …*/`) and inserted code sits between
  `// caelestia-window-frames:begin` and `:end`, so any version can undo it. After a Caelestia update it patches again and
  restarts the shell once; if Caelestia changed that line, it touches nothing and shows a
  notification. While it runs it leaves `~/.local/state/caelestia/window-frames-edge`, so the
  decoration matches its edge pixels to the new overlap.
- Nothing runs while nothing changes: animations only tick while one is playing, and files
  are only read again when they change.

## Repository layout

```
src/                  the decoration (C++, KDecoration3)
shell/                the shell part (Caelestia plugin: patch.py, main.qml)
metadata.json         Caelestia plugin manifest for the shell part
packaging/PKGBUILD    local pacman package, built from this folder
install.sh            build, install and select the decoration, install the shell part
rebuild.sh            build and reinstall the decoration (after Plasma updates)
uninstall.sh          remove both and switch back to Breeze
assets/               README artwork
```

## Credits

- [caelestia-kde](https://github.com/caelestia-kde/caelestia) by [ladybug-me](https://github.com/ladybug-me) and [0xSolanaceae](https://github.com/0xSolanaceae) - the shell whose look, colours, transparency and motion tokens this follows
- [Breeze](https://invent.kde.org/plasma/breeze) by KDE - read as a reference for the KDecoration3 API; no Breeze code is included
- [KDecoration](https://invent.kde.org/plasma/kdecoration) by KDE (LGPL) - the decoration library this plugs into
- [Material Symbols](https://github.com/google/material-design-icons) by Google (Apache 2.0) - icon font used for the rarer buttons
- [COCONAT10MEN](https://github.com/COCONAT10MEN) - this decoration

## License

MIT - see [LICENSE](LICENSE). Free for anyone to use, modify and redistribute, including
in other projects or commercial ones. If you use this as a base, please credit
[COCONAT10MEN](https://github.com/COCONAT10MEN) (keeping the LICENSE file's copyright
line does that).
