// SPDX-FileCopyrightText: 2026 COCONAT10MEN
// SPDX-License-Identifier: MIT

#pragma once

#include <QColor>
#include <QRectF>

class QPainter;

namespace Caelestia
{

/// The window controls, drawn as thin strokes rather than taken from a font,
/// so they line up on the pixel grid and read as one even set.
namespace Icons
{
enum class Kind {
    Minimize,
    Maximize,
    Restore,
    Close,
};

/// Paints `kind` centred in `button` (the whole button square).
void paint(QPainter *painter, Kind kind, const QRectF &button, const QColor &colour);
}

}
