// SPDX-FileCopyrightText: 2026 COCONAT10MEN
// SPDX-License-Identifier: MIT

#include "icons.h"

#include <QPainter>
#include <QPainterPath>
#include <QPen>

#include <cmath>

namespace Caelestia
{
namespace Icons
{

void paint(QPainter *painter, Kind kind, const QRectF &button, const QColor &colour)
{
    // A 10px glyph with a 1px stroke on a 32px button, scaled with the
    // button. Coordinates sit on half pixels so 1px lines stay sharp.
    const qreal unit = button.height() / 32.0;
    const qreal stroke = std::max<qreal>(1.0, std::round(unit * 1.0 * 4) / 4);
    const qreal half = std::round(5 * unit);
    const QPointF c(std::floor(button.center().x()) + 0.5, std::floor(button.center().y()) + 0.5);

    QPen pen(colour, stroke, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setPen(pen);
    painter->setBrush(Qt::NoBrush);

    switch (kind) {
    case Kind::Minimize:
        painter->drawLine(QPointF(c.x() - half, c.y()), QPointF(c.x() + half, c.y()));
        break;
    case Kind::Maximize: {
        const QRectF box(c.x() - half, c.y() - half, half * 2, half * 2);
        const qreal r = 1.5 * unit;
        painter->drawRoundedRect(box, r, r);
        break;
    }
    case Kind::Restore: {
        // A front square with the corner of a second one peeking out behind.
        const qreal small = std::round(half * 1.4);
        const qreal shift = std::round(half * 2) - small;
        const qreal r = 1.5 * unit;
        const QRectF front(c.x() - half, c.y() - half + shift, small, small);
        painter->drawRoundedRect(front, r, r);
        QPainterPath back;
        back.moveTo(front.left() + shift, front.top());
        back.lineTo(front.left() + shift, front.top() - shift + r);
        back.quadTo(front.left() + shift, front.top() - shift, front.left() + shift + r, front.top() - shift);
        back.lineTo(front.right() + shift - r, front.top() - shift);
        back.quadTo(front.right() + shift, front.top() - shift, front.right() + shift, front.top() - shift + r);
        back.lineTo(front.right() + shift, front.bottom() - shift);
        back.lineTo(front.right(), front.bottom() - shift);
        painter->drawPath(back);
        break;
    }
    case Kind::Close: {
        // Diagonals look lighter than straight lines at the same width.
        pen.setWidthF(stroke * 1.3);
        painter->setPen(pen);
        const qreal d = std::round(half * 0.9 * 2) / 2;
        painter->drawLine(QPointF(c.x() - d, c.y() - d), QPointF(c.x() + d, c.y() + d));
        painter->drawLine(QPointF(c.x() + d, c.y() - d), QPointF(c.x() - d, c.y() + d));
        break;
    }
    }

    painter->restore();
}

}
}
