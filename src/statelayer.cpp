// SPDX-FileCopyrightText: 2026 COCONAT10MEN
// SPDX-License-Identifier: MIT

#include "statelayer.h"
#include "theme.h"

#include <QPainter>
#include <QRadialGradient>

#include <algorithm>
#include <cmath>

namespace Caelestia
{

namespace
{
// Caelestia's rounding.small, the radius a pressed button morphs to.
constexpr qreal RoundingSmall = 8;
// The ripple's starting opacity (StateLayer.qml).
constexpr qreal RippleOpacity = 0.1;
}

StateLayer::StateLayer(Theme *theme, QObject *parent)
    : QObject(parent)
    , m_theme(theme)
{
    for (Tween *t : {&m_hover, &m_morph, &m_ripple, &m_rippleOpacity})
        connect(t, &Tween::valueChanged, this, &StateLayer::changed);
    // A ripple that outlives the press fades once it has spread.
    connect(&m_ripple, &Tween::finished, this, [this] {
        if (!m_pressed)
            fadeRipple();
    });
}

int StateLayer::duration(Motion::Token token) const
{
    return m_theme->duration(Motion::baseDuration(token));
}

void StateLayer::animate(Tween &tween, qreal target, Motion::Token token)
{
    tween.animateTo(target, duration(token), Motion::curve(token));
}

void StateLayer::setHovered(bool hovered)
{
    if (hovered == m_hovered)
        return;
    m_hovered = hovered;
    animate(m_hover, hovered ? 1 : 0, Motion::Token::FastEffects);
}

void StateLayer::press(const QPointF &pos)
{
    m_pressed = true;
    m_pressPos = pos;
    animate(m_morph, 1, Motion::Token::DefaultEffects);

    // Same as Caelestia: the ripple grows on the standard curve over twice
    // the slow-effects duration.
    m_rippleOpacity.set(RippleOpacity);
    m_ripple.set(0);
    m_ripple.animateTo(1, duration(Motion::Token::SlowEffects) * 2, Motion::curve(Motion::Token::Standard));
}

void StateLayer::release()
{
    if (!m_pressed)
        return;
    m_pressed = false;
    animate(m_morph, 0, Motion::Token::DefaultEffects);
    if (!m_ripple.isRunning())
        fadeRipple();
}

void StateLayer::fadeRipple()
{
    animate(m_rippleOpacity, 0, Motion::Token::SlowEffects);
}

QPainterPath StateLayer::shape(const QRectF &rect) const
{
    // Round buttons follow the rounding scale like Caelestia's isRound ones.
    const qreal scale = m_theme->roundingScale();
    const qreal round = std::min(rect.width(), rect.height()) / 2 * std::min<qreal>(1, scale);
    const qreal pressed = std::min(round, RoundingSmall * scale);
    const qreal radius = std::max<qreal>(0, round + (pressed - round) * m_morph.value());

    QPainterPath path;
    path.addRoundedRect(rect, radius, radius);
    return path;
}

void StateLayer::paint(QPainter *painter, const QRectF &rect, const QColor &colour, qreal hoverAlpha) const
{
    const qreal hover = std::clamp<qreal>(m_hover.value(), 0, 1);
    const qreal ripple = m_rippleOpacity.value();
    if (hover <= 0 && ripple <= 0)
        return;

    const QPainterPath path = shape(rect);
    painter->save();
    painter->setPen(Qt::NoPen);

    if (hover > 0) {
        QColor c = colour;
        c.setAlphaF(colour.alphaF() * hoverAlpha * hover);
        painter->setBrush(c);
        painter->drawPath(path);
    }

    if (ripple > 0 && m_ripple.value() > 0) {
        // The ripple's soft-edged circle, as in StateLayer.qml.
        const QPointF p = m_pressPos;
        qreal far = 0;
        for (const QPointF &corner : {rect.topLeft(), rect.topRight(), rect.bottomLeft(), rect.bottomRight()})
            far = std::max(far, std::hypot(corner.x() - p.x(), corner.y() - p.y()));
        const qreal end = far * 1.3;
        const qreal r = std::max<qreal>(0.5, end * m_ripple.value());

        QColor solid = colour;
        solid.setAlphaF(ripple);
        QColor edge = colour;
        edge.setAlphaF(ripple * std::clamp((r / end - 0.9) / 0.1, 0.0, 1.0));

        QRadialGradient g(p, r);
        g.setColorAt(0, solid);
        g.setColorAt(std::clamp(1 - 0.2 * end / r, 0.01, 0.99), solid);
        g.setColorAt(1, edge);

        painter->setClipPath(path, Qt::IntersectClip);
        painter->setBrush(g);
        painter->drawEllipse(p, r, r);
    }

    painter->restore();
}

}
