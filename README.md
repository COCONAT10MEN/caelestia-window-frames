<div align="center">

<img src="assets/icon.svg" width="80" alt="Caelestia window frames icon" />

# caelestia-window-frames

[![Caelestia KDE](https://img.shields.io/badge/Caelestia_KDE-2.7.5_tested-9bd0cc?style=for-the-badge&labelColor=101418)](https://github.com/caelestia-kde/caelestia)
[![KDE Plasma](https://img.shields.io/badge/Plasma_6.8-1D99F3?logo=kde&logoColor=white&style=for-the-badge&labelColor=101418)](https://kde.org/plasma-desktop)
[![Version](https://img.shields.io/badge/Version-1.0.2-9bd0cc?style=for-the-badge&labelColor=101418)](CMakeLists.txt)
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
  read as one surface. Corners that sit in a corner of the screen follow the frame's curve
  instead of poking under it.
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

`install.sh` builds a local pacman package, installs it and selects the decoration. Log out
and back in: KWin only loads a new decoration build at login.

You can also pick it by hand in *System Settings → Colors & Themes → Window Decorations →
Caelestia*.

### Updating

```bash
cd caelestia-window-frames
git pull
./rebuild.sh
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

Switch to another decoration first (above), then:

```bash
sudo pacman -R caelestia-window-frames
```

## Configuring

Nothing to configure here: it follows Caelestia's own settings (colours, transparency, blur,
rounding scale, animation speed, bar position) and KDE's button order and title font.

## Troubleshooting

| Problem | Fix |
| --- | --- |
| Windows show KDE's default frames after an update | Run `./rebuild.sh`, then log out and back in |
| A change to the decoration does not show | KWin keeps the decoration it loaded at login: log out and back in |
| Icons look wrong | Make sure `ttf-material-symbols-variable` is installed |

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
- Nothing runs while nothing changes: animations only tick while one is playing, and files
  are only read again when they change.

## Repository layout

```
src/                  the decoration (C++, KDecoration3)
packaging/PKGBUILD    local pacman package, built from this folder
install.sh            build, install and select the decoration
rebuild.sh            build and reinstall (after updates)
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
