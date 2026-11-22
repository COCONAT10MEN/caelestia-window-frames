// SPDX-FileCopyrightText: 2026 COCONAT10MEN
// SPDX-License-Identifier: MIT

#pragma once

#include <QColor>
#include <QFont>
#include <QFileSystemWatcher>
#include <QHash>
#include <QObject>
#include <QString>
#include <QTimer>

#include <memory>

namespace Caelestia
{

/// Caelestia's current look, read from the files the shell itself uses:
///   ~/.local/state/caelestia/scheme.json  (Material You scheme, light/dark)
///   ~/.config/caelestia/shell.json        (transparency, blur, rounding)
/// One instance is shared by every window; it watches both files and emits
/// changed() once per edit, so windows repaint live without a KWin restart.
class Theme : public QObject
{
    Q_OBJECT

public:
    static std::shared_ptr<Theme> instance();
    ~Theme() override;

    /// A scheme colour by its Material name ("surface", "onSurface", "primary"…).
    QColor colour(const QString &name) const;
    bool isLight() const { return m_light; }

    bool transparencyEnabled() const { return m_transparencyEnabled; }
    /// Alpha of the shell's base layer (the bar and the screen frame).
    qreal transparencyBase() const { return m_transparencyBase; }
    qreal transparencyLayers() const { return m_transparencyLayers; }
    bool blurEnabled() const { return m_blur; }
    qreal roundingScale() const { return m_roundingScale; }

    /// A motion token's duration after Caelestia's animation settings
    /// (appearance.anim.enabled and appearance.anim.durations.scale).
    int duration(int baseMs) const { return m_animEnabled ? int(baseMs * m_animScale) : 0; }

    /// The colour of Caelestia's bar and screen frame, built the way the shell
    /// builds it: surface at the base transparency (0.1 lower in light mode),
    /// solid when transparency is off, black when "pitch black" is on.
    QColor frameColour() const;

    /// Caelestia's frame is drawn above windows, so where it meets a window
    /// KWin's blur under the frame takes in that window's own content. A
    /// titlebar touching the frame mirrors that: this is the frame colour
    /// with `appShare` of the blurred backdrop made of `app` (the app's
    /// background) instead of what lies behind the window.
    QColor frameColourWithApp(const QColor &app, qreal appShare) const;
    /// How much of the frame's backdrop is the app, at `distance` px along the
    /// frame from where it turns into the bar (`barSide`) or the thin border.
    /// Measured on Caelestia 2.7.5 with KWin's default blur strength.
    static qreal frameAppShare(qreal distance, bool barSide);
    /// The radius of the frame's inner corners (border.rounding), or 0 when
    /// Caelestia draws no frame ("islands" mode).
    qreal frameCornerRadius() const { return m_islands ? 0 : m_frameRounding; }
    /// What a titlebar pixel must hold so that Caelestia's frame, drawn over
    /// it with `coverage` (0..1), comes out the way `target` looks on its own.
    /// Used under the frame's soft inner edge and its unblurred corner band.
    QColor underFrame(const QColor &target, qreal coverage) const;
    /// Width of the band along the frame's inner corner curve where Caelestia
    /// turns its blur off (its BlurCorners: rounding + 2, as stepped regions).
    static constexpr qreal FrameCornerBand = 2.5;
    /// How much of a pixel the frame's inner corner covers, `distance` px
    /// from the curve's centre (fitted to Caelestia 2.7.5 on screen).
    qreal frameCornerCoverage(qreal distance) const;
    /// Which screen edge Caelestia's bar is on.
    Qt::Edge barEdge() const { return m_barEdge; }

    /// Material Symbols Rounded, light and thin like the reference titlebar.
    static QFont iconFont(qreal pixelSize);

Q_SIGNALS:
    void changed();

private:
    Theme();
    void watch();
    void reload();
    void loadScheme();
    void loadShell();

    QString m_schemePath;
    QString m_shellPath;
    QFileSystemWatcher m_watcher;
    QTimer m_debounce;
    qint64 m_schemeStamp = -1;
    qint64 m_shellStamp = -1;

    QHash<QString, QColor> m_colours;
    bool m_light = false;
    bool m_transparencyEnabled = true;
    qreal m_transparencyBase = 0.85;
    qreal m_transparencyLayers = 0.4;
    bool m_blur = true;
    qreal m_roundingScale = 1.0;
    bool m_pitchBlack = false;
    Qt::Edge m_barEdge = Qt::LeftEdge;
    qreal m_frameRounding = 25;
    bool m_islands = false;
    bool m_animEnabled = true;
    qreal m_animScale = 1.0;
};

}
