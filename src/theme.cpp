// SPDX-FileCopyrightText: 2026 COCONAT10MEN
// SPDX-License-Identifier: MIT

#include "theme.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QStandardPaths>

#include <algorithm>
#include <cmath>

namespace Caelestia
{

namespace
{
QString stateHome()
{
    const QByteArray env = qgetenv("XDG_STATE_HOME");
    return env.isEmpty() ? QDir::homePath() + QStringLiteral("/.local/state") : QString::fromLocal8Bit(env);
}

QString configHome()
{
    const QByteArray env = qgetenv("XDG_CONFIG_HOME");
    return env.isEmpty() ? QDir::homePath() + QStringLiteral("/.config") : QString::fromLocal8Bit(env);
}

qint64 stampOf(const QString &path)
{
    const QFileInfo info(path);
    return info.exists() ? info.lastModified().toMSecsSinceEpoch() ^ info.size() : -1;
}

QJsonObject readJson(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return {};
    return QJsonDocument::fromJson(file.readAll()).object();
}
}

std::shared_ptr<Theme> Theme::instance()
{
    // Shared by every decorated window, released with the last one.
    static std::weak_ptr<Theme> shared;
    auto theme = shared.lock();
    if (!theme) {
        theme = std::shared_ptr<Theme>(new Theme());
        shared = theme;
    }
    return theme;
}

Theme::Theme()
    : m_schemePath(stateHome() + QStringLiteral("/caelestia/scheme.json"))
    , m_shellPath(configHome() + QStringLiteral("/caelestia/shell.json"))
{
    // Editors and the shell often write a new file and rename it over the
    // old one, which only shows up as a change of the folder: watch both
    // folders and the files, and settle bursts of writes into one reload.
    m_debounce.setSingleShot(true);
    m_debounce.setInterval(120);
    connect(&m_debounce, &QTimer::timeout, this, &Theme::reload);
    connect(&m_watcher, &QFileSystemWatcher::fileChanged, &m_debounce, qOverload<>(&QTimer::start));
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, &m_debounce, qOverload<>(&QTimer::start));

    loadScheme();
    loadShell();
    watch();
}

Theme::~Theme() = default;

void Theme::watch()
{
    for (const QString &path : {m_schemePath, m_shellPath}) {
        const QString dir = QFileInfo(path).absolutePath();
        if (!m_watcher.directories().contains(dir) && QFileInfo::exists(dir))
            m_watcher.addPath(dir);
        if (!m_watcher.files().contains(path) && QFileInfo::exists(path))
            m_watcher.addPath(path);
    }
}

void Theme::reload()
{
    // Only files that really changed are read again.
    const qint64 scheme = stampOf(m_schemePath);
    const qint64 shell = stampOf(m_shellPath);
    const bool schemeChanged = scheme != m_schemeStamp;
    const bool shellChanged = shell != m_shellStamp;
    if (schemeChanged)
        loadScheme();
    if (shellChanged)
        loadShell();
    watch(); // a replaced file is a new file to the watcher
    if (schemeChanged || shellChanged)
        Q_EMIT changed();
}

void Theme::loadScheme()
{
    m_schemeStamp = stampOf(m_schemePath);
    const QJsonObject root = readJson(m_schemePath);
    const QJsonObject colours = root.value(QStringLiteral("colours")).toObject();
    if (colours.isEmpty())
        return; // keep what we had (or the fallbacks) rather than going blank

    m_colours.clear();
    for (auto it = colours.begin(); it != colours.end(); ++it) {
        const QColor c(QLatin1Char('#') + it.value().toString());
        if (c.isValid())
            m_colours.insert(it.key(), c);
    }
    m_light = root.value(QStringLiteral("mode")).toString() == QLatin1String("light");
}

void Theme::loadShell()
{
    m_shellStamp = stampOf(m_shellPath);
    const QJsonObject appearance = readJson(m_shellPath).value(QStringLiteral("appearance")).toObject();
    const QJsonObject transparency = appearance.value(QStringLiteral("transparency")).toObject();

    m_transparencyEnabled = transparency.value(QStringLiteral("enabled")).toBool(true);
    m_transparencyBase = std::clamp(transparency.value(QStringLiteral("base")).toDouble(0.85), 0.0, 1.0);
    m_transparencyLayers = std::clamp(transparency.value(QStringLiteral("layers")).toDouble(0.4), 0.0, 1.0);
    m_blur = appearance.value(QStringLiteral("blur")).toBool(true);
    m_roundingScale = appearance.value(QStringLiteral("rounding")).toObject().value(QStringLiteral("scale")).toDouble(1.0);
    m_pitchBlack = appearance.value(QStringLiteral("pitchBlack")).toBool(false);
    const QJsonObject shell = readJson(m_shellPath);
    m_frameRounding = std::max(0.0, shell.value(QStringLiteral("border")).toObject().value(QStringLiteral("rounding")).toDouble(25));
    m_islands = appearance.value(QStringLiteral("islands")).toBool(false);
    m_blurMask = appearance.value(QStringLiteral("blurMask")).toBool(true);
    // "Blur Corner Quality" lives in the shell's own settings file.
    m_blurQuality = 20;
    for (const QString &name : {QStringLiteral("/Caelestia/caelestia-shell.conf"), QStringLiteral("/Caelestia/noctalia-qs.conf")}) {
        QSettings ini(configHome() + name, QSettings::IniFormat);
        const QVariant value = ini.value(QStringLiteral("Blur/blurQuality"));
        if (value.isValid()) {
            m_blurQuality = std::clamp(value.toInt(), 1, 100);
            break;
        }
    }
    const QString bar = shell.value(QStringLiteral("bar")).toObject().value(QStringLiteral("position")).toString(QStringLiteral("left"));
    m_barEdge = bar == QLatin1String("right") ? Qt::RightEdge
        : bar == QLatin1String("top")         ? Qt::TopEdge
        : bar == QLatin1String("bottom")      ? Qt::BottomEdge
                                              : Qt::LeftEdge;

    const QJsonObject anim = appearance.value(QStringLiteral("anim")).toObject();
    m_animEnabled = anim.value(QStringLiteral("enabled")).toBool(true);
    m_animScale = std::max(0.0, anim.value(QStringLiteral("durations")).toObject().value(QStringLiteral("scale")).toDouble(1.0));
}

QColor Theme::colour(const QString &name) const
{
    const auto it = m_colours.constFind(name);
    if (it != m_colours.cend())
        return *it;
    // Readable fallbacks until Caelestia has written a scheme.
    if (name.startsWith(QLatin1String("on")))
        return m_light ? QColor(0x1d, 0x1b, 0x20) : QColor(0xe6, 0xe0, 0xe9);
    if (name == QLatin1String("error"))
        return m_light ? QColor(0xb3, 0x26, 0x1e) : QColor(0xf2, 0xb8, 0xb5);
    return m_light ? QColor(0xfe, 0xf7, 0xff) : QColor(0x14, 0x12, 0x18);
}

QFont Theme::iconFont(qreal pixelSize)
{
    QFont font(QStringLiteral("Material Symbols Rounded"));
    font.setPixelSize(std::max(8, int(std::round(pixelSize))));
    // The variable font's axes: thin strokes, outlined, sized for small use.
    font.setVariableAxis(QFont::Tag("wght"), 300);
    font.setVariableAxis(QFont::Tag("FILL"), 0);
    font.setVariableAxis(QFont::Tag("opsz"), 20);
    return font;
}

QColor Theme::frameColour() const
{
    // Caelestia's frame fills with pure black in "pitch black" mode, opaque.
    if (m_pitchBlack)
        return QColor(Qt::black);
    QColor c = colour(QStringLiteral("surface"));
    if (m_transparencyEnabled) {
        // Caelestia lowers the base layer's alpha by 0.1 in light mode.
        c.setAlphaF(std::clamp(m_transparencyBase - (m_light ? 0.1 : 0.0), 0.0, 1.0));
    }
    return c;
}

QColor Theme::frameColourWithApp(const QColor &app, qreal appShare) const
{
    QColor frame = frameColour();
    const qreal a = frame.alphaF();
    if (a >= 1.0)
        return frame;
    // Frame over a backdrop that is `k` app and (1 - k) what is behind:
    //   a * surface + (1 - a) * (k * app + (1 - k) * behind)
    // As a colour C at alpha A over what is behind the titlebar, that is
    //   A = 1 - (1 - a)(1 - k),  C = (a * surface + (1 - a) * k * app) / A
    const qreal k = std::clamp(appShare, 0.0, 1.0);
    const qreal alpha = 1 - (1 - a) * (1 - k);
    const auto mix = [&](qreal s, qreal p) {
        return std::clamp((a * s + (1 - a) * k * p) / alpha, 0.0, 1.0);
    };
    return QColor::fromRgbF(mix(frame.redF(), app.redF()), mix(frame.greenF(), app.greenF()), mix(frame.blueF(), app.blueF()), alpha);
}

qreal Theme::frameAppShare(qreal distance, bool barSide)
{
    // Away from the corners about 69% of the frame's backdrop is the window.
    // Towards the bar the blur sees more bar and less window, down to about
    // a third at the bar itself, easing off over ~80px; the thin border on
    // the other sides barely dents it.
    constexpr qreal Open = 0.69;
    constexpr qreal Falloff = 80;
    const qreal dip = barSide ? 0.36 : 0.06;
    return Open - dip * std::exp(-std::max<qreal>(0, distance) / Falloff);
}

QColor Theme::underFrame(const QColor &target, qreal coverage) const
{
    const QColor frame = frameColour();
    const qreal fa = frame.alphaF() * std::clamp(coverage, 0.0, 1.0);
    if (fa <= 0)
        return target;
    if (fa >= 1)
        return QColor(0, 0, 0, 0); // fully hidden under a solid frame
    // Frame over pixel P:  fa * frame + (1 - fa) * P  should equal target T.
    // In premultiplied terms: P = (T - fa * frame) / (1 - fa), alpha alike.
    const qreal ta = target.alphaF();
    const qreal alpha = std::clamp(1 - (1 - ta) / (1 - fa), 0.0, 1.0);
    if (alpha <= 0)
        return QColor(0, 0, 0, 0);
    const auto channel = [&](qreal t, qreal f) {
        const qreal premul = (ta * t - fa * f) / (1 - fa);
        return std::clamp(premul / alpha, 0.0, 1.0);
    };
    return QColor::fromRgbF(channel(target.redF(), frame.redF()), channel(target.greenF(), frame.greenF()),
                            channel(target.blueF(), frame.blueF()), alpha);
}

qreal Theme::frameCornerCoverage(qreal distance) const
{
    // Caelestia's blob shader: alpha = 1 - smoothstep(-w, w, d), d measured
    // into the hole. Fitted on screen: the curve sits ~0.05px inside the
    // configured radius and its soft edge reaches ~1.6px either side.
    const qreal edge = frameCornerRadius() - 0.05;
    const qreal w = 1.6;
    const qreal t = std::clamp((edge - distance + w) / (2 * w), 0.0, 1.0);
    return 1 - t * t * (3 - 2 * t);
}

QPainterPath Theme::frameCornerUnblurred() const
{
    QPainterPath area;
    const qreal r = frameCornerRadius();
    if (r <= 0 || !m_blurMask)
        return area;
    // Mirrors Caelestia's BlurCorners: strip m (1..steps) reaches up to
    // m/steps of the radius and as wide as the arc at the strip's outer
    // edge; Caelestia subtracts their union from the frame's blur.
    const qreal outer = r + 2;
    const int steps = m_blurQuality;
    for (int m = 1; m <= steps; ++m) {
        const qreal prev = qreal(m - 1) / steps;
        const qreal width = outer * std::sqrt(1 - prev * prev);
        const qreal height = outer * m / steps;
        // Snapped outward: cover every pixel the region might touch.
        const qreal x = std::floor(r - width);
        const qreal y = std::floor(r - height);
        area.addRect(QRectF(x, y, std::ceil(r + 1) - x, std::ceil(r + 1) - y));
    }
    return area.simplified();
}

qreal Theme::frameEdgeCoverage() const
{
    // The first pixel's centre is 0.5px into the window; the edge sits
    // ~0.05px inside, and is soft ~1.6px either side (see frameCornerCoverage).
    const qreal d = 0.5 - 0.05;
    const qreal w = 1.6;
    const qreal t = std::clamp((d + w) / (2 * w), 0.0, 1.0);
    return 1 - t * t * (3 - 2 * t);
}

}
