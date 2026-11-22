// SPDX-FileCopyrightText: 2026 COCONAT10MEN
// SPDX-License-Identifier: MIT

#pragma once

#include "motion.h"

#include <QColor>
#include <QObject>
#include <QPainterPath>
#include <QPointF>
#include <QRectF>

class QPainter;

namespace Caelestia
{

class Theme;

/// The Material 3 Expressive state of one icon button, modelled on
/// Caelestia's own StateLayer and ButtonBase: round at rest, a hover layer
/// that fades in, and on press a ripple from the pointer while the shape
/// morphs from a circle to a rounded square.
class StateLayer : public QObject
{
    Q_OBJECT

public:
    explicit StateLayer(Theme *theme, QObject *parent = nullptr);

    void setHovered(bool hovered);
    void press(const QPointF &pos);
    void release();

    /// 0 at rest, 1 fully hovered (animated).
    qreal hover() const { return m_hover.value(); }

    /// The button's current shape inside its square.
    QPainterPath shape(const QRectF &rect) const;
    /// Hover layer and ripple, in `colour`, clipped to the shape.
    /// `hoverAlpha` is the hover layer's opacity when fully hovered.
    void paint(QPainter *painter, const QRectF &rect, const QColor &colour, qreal hoverAlpha) const;

Q_SIGNALS:
    void changed();

private:
    int duration(Motion::Token token) const;
    void animate(Tween &tween, qreal target, Motion::Token token);
    void fadeRipple();

    Theme *m_theme;
    bool m_hovered = false;
    bool m_pressed = false;
    QPointF m_pressPos;

    Tween m_hover;
    Tween m_morph; // 0 round, 1 the pressed rounded square
    Tween m_ripple; // ripple radius as a fraction of the end radius
    Tween m_rippleOpacity;
};

}
