// SPDX-FileCopyrightText: 2026 COCONAT10MEN
// SPDX-License-Identifier: MIT

#pragma once

#include <QEasingCurve>
#include <QObject>
#include <QVariantAnimation>

namespace Caelestia
{

/// Caelestia's motion tokens (shell/plugin/src/Caelestia/Config/tokens.hpp):
/// the same curves and base durations the shell animates with.
namespace Motion
{
enum class Token {
    FastSpatial,
    DefaultSpatial,
    SlowSpatial,
    FastEffects,
    DefaultEffects,
    SlowEffects,
    Standard,
};

/// Base duration in ms, before Caelestia's duration scale.
int baseDuration(Token token);
QEasingCurve curve(Token token);
}

/// One animated number. Nothing runs while it is at rest, so a window that
/// is not being hovered costs nothing.
class Tween : public QObject
{
    Q_OBJECT

public:
    explicit Tween(qreal initial = 0, QObject *parent = nullptr);

    qreal value() const { return m_value; }
    qreal target() const { return m_target; }
    bool isRunning() const;

    /// Animate from where it is now; a duration of 0 jumps straight there.
    void animateTo(qreal target, int duration, const QEasingCurve &curve);
    void set(qreal value);

Q_SIGNALS:
    void valueChanged();
    void finished();

private:
    QVariantAnimation m_anim;
    qreal m_value;
    qreal m_target;
};

}
