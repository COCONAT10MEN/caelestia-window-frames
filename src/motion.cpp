// SPDX-FileCopyrightText: 2026 COCONAT10MEN
// SPDX-License-Identifier: MIT

#include "motion.h"

#include <QPointF>

namespace Caelestia
{

namespace Motion
{
namespace
{
QEasingCurve bezier(qreal x1, qreal y1, qreal x2, qreal y2)
{
    QEasingCurve c(QEasingCurve::BezierSpline);
    c.addCubicBezierSegment(QPointF(x1, y1), QPointF(x2, y2), QPointF(1, 1));
    return c;
}
}

int baseDuration(Token token)
{
    switch (token) {
    case Token::FastSpatial:
        return 350;
    case Token::DefaultSpatial:
        return 500;
    case Token::SlowSpatial:
        return 650;
    case Token::FastEffects:
        return 150;
    case Token::DefaultEffects:
        return 200;
    case Token::SlowEffects:
        return 300;
    case Token::Standard:
        return 400;
    }
    return 200;
}

QEasingCurve curve(Token token)
{
    switch (token) {
    case Token::FastSpatial:
        return bezier(0.42, 1.67, 0.21, 0.9);
    case Token::DefaultSpatial:
        return bezier(0.38, 1.21, 0.22, 1);
    case Token::SlowSpatial:
        return bezier(0.39, 1.29, 0.35, 0.98);
    case Token::FastEffects:
        return bezier(0.31, 0.94, 0.34, 1);
    case Token::DefaultEffects:
        return bezier(0.34, 0.8, 0.34, 1);
    case Token::SlowEffects:
        return bezier(0.34, 0.88, 0.34, 1);
    case Token::Standard:
        return bezier(0.2, 0, 0, 1);
    }
    return {};
}
}

Tween::Tween(qreal initial, QObject *parent)
    : QObject(parent)
    , m_value(initial)
    , m_target(initial)
{
    connect(&m_anim, &QVariantAnimation::valueChanged, this, [this](const QVariant &v) {
        m_value = v.toReal();
        Q_EMIT valueChanged();
    });
    connect(&m_anim, &QVariantAnimation::finished, this, &Tween::finished);
}

bool Tween::isRunning() const
{
    return m_anim.state() == QAbstractAnimation::Running;
}

void Tween::animateTo(qreal target, int duration, const QEasingCurve &curve)
{
    if (qFuzzyCompare(target + 1, m_target + 1) && (isRunning() || qFuzzyCompare(m_value + 1, target + 1)))
        return;
    m_target = target;
    m_anim.stop();
    if (duration <= 0) {
        set(target);
        Q_EMIT finished();
        return;
    }
    m_anim.setStartValue(m_value);
    m_anim.setEndValue(target);
    m_anim.setDuration(duration);
    m_anim.setEasingCurve(curve);
    m_anim.start();
}

void Tween::set(qreal value)
{
    m_anim.stop();
    m_target = value;
    if (qFuzzyCompare(value + 1, m_value + 1))
        return;
    m_value = value;
    Q_EMIT valueChanged();
}

}
