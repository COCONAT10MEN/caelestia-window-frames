// SPDX-FileCopyrightText: 2026 COCONAT10MEN
// SPDX-License-Identifier: MIT

#include "decoration.h"

// KWin internals (see CMakeLists.txt)
#include "window.h"
#include "workspace.h"
#include "button.h"
#include "motion.h"
#include "statelayer.h"
#include "theme.h"

#include <KDecoration3/DecoratedWindow>
#include <KDecoration3/DecorationSettings>

#include <QFontMetricsF>
#include <QHoverEvent>
#include <QMouseEvent>
#include <QLinearGradient>
#include <QRadialGradient>
#include <QPainter>
#include <QTransform>
#include <QtMath>
#include <QPainterPath>

#include <algorithm>

namespace Caelestia
{

namespace
{
// Caelestia's spacing and rounding tokens (shell/plugin/.../tokens.hpp).
constexpr qreal PaddingSmall = 8;
constexpr qreal SpacingSmall = 8;
constexpr qreal RoundingLarge = 16;
// Gap between the icon buttons: close enough to read as one group, wide
// enough that each is its own target.
constexpr qreal ButtonSpacing = 4;
// The thin line around windowed windows.
constexpr qreal OutlineWidth = 1;
}

Decoration::Decoration(QObject *parent, const QVariantList &args)
    : KDecoration3::Decoration(parent, args)
{
}

Decoration::~Decoration() = default;

bool Decoration::init()
{
    m_theme = Theme::instance();

    auto *w = window();
    m_active = std::make_unique<Tween>(w->isActive() ? 1 : 0);
    m_chevronLayer = std::make_unique<StateLayer>(m_theme.get());
    connect(m_chevronLayer.get(), &StateLayer::changed, this, [this] {
        update(chevronRect());
    });
    connect(m_active.get(), &Tween::valueChanged, this, &Decoration::updateColours);

    // Repaint on whatever changes how the frame looks.
    connect(m_theme.get(), &Theme::changed, this, &Decoration::updateLayout);
    connect(w, &KDecoration3::DecoratedWindow::activeChanged, this, [this] {
        m_active->animateTo(window()->isActive() ? 1 : 0,
                            m_theme->duration(Motion::baseDuration(Motion::Token::DefaultEffects)),
                            Motion::curve(Motion::Token::DefaultEffects));
    });
    connect(w, &KDecoration3::DecoratedWindow::captionChanged, this, [this] {
        update(titleBar());
    });
    connect(w, &KDecoration3::DecoratedWindow::widthChanged, this, &Decoration::updateLayout);
    connect(w, &KDecoration3::DecoratedWindow::maximizedChanged, this, &Decoration::updateLayout);
    connect(w, &KDecoration3::DecoratedWindow::adjacentScreenEdgesChanged, this, &Decoration::updateLayout);
    connect(w, &KDecoration3::DecoratedWindow::paletteChanged, this, &Decoration::updateLayout);
    connect(w, &KDecoration3::DecoratedWindow::nextScaleChanged, this, &Decoration::updateLayout);
    connect(settings().get(), &KDecoration3::DecorationSettings::fontChanged, this, &Decoration::updateLayout);
    connect(settings().get(), &KDecoration3::DecorationSettings::alphaChannelSupportedChanged, this, &Decoration::updateLayout);

    m_leftButtons = new KDecoration3::DecorationButtonGroup(KDecoration3::DecorationButtonGroup::Position::Left, this, &Button::create);
    m_rightButtons = new KDecoration3::DecorationButtonGroup(KDecoration3::DecorationButtonGroup::Position::Right, this, &Button::create);
    m_leftButtons->setSpacing(ButtonSpacing);
    m_rightButtons->setSpacing(ButtonSpacing);

    updateLayout();
    // KWin creates the decoration before it lists the window: look it up
    // when KWin adds a window, on layout updates, and shortly after.
    if (auto *workspace = KWin::workspace()) {
        connect(workspace, &KWin::Workspace::windowAdded, this, [this] {
            findKWinWindow();
        });
    }
    for (int delay : {0, 50, 250, 1000})
        QTimer::singleShot(delay, this, &Decoration::findKWinWindow);
    return true;
}

void Decoration::findKWinWindow()
{
    auto *workspace = KWin::workspace();
    if (!workspace || m_kwinWindow)
        return;
    for (KWin::Window *w : workspace->windows()) {
        if (w->decoration() != this)
            continue;
        m_kwinWindow = w;
        if (auto *workspace = KWin::workspace())
            disconnect(workspace, &KWin::Workspace::windowAdded, this, nullptr);
        connect(w, &KWin::Window::frameGeometryChanged, this, &Decoration::updatePlacement);
        connect(w, &KWin::Window::outputChanged, this, &Decoration::updatePlacement);
        // Applied on the next turn of the event loop: this can be reached
        // from inside updateLayout().
        QTimer::singleShot(0, this, &Decoration::updatePlacement);
        return;
    }
}

void Decoration::updatePlacement()
{
    auto *workspace = KWin::workspace();
    auto *w = qobject_cast<KWin::Window *>(m_kwinWindow.data());
    if (!workspace || !w)
        return;
    const QRectF frame = w->frameGeometry();
    const QRectF area = workspace->clientArea(KWin::MaximizeArea, w);
    // Within a pixel counts as touching (fractional scaling rounds).
    const auto near = [](qreal a, qreal b) {
        return std::abs(a - b) < 1.5;
    };
    Qt::Edges edges;
    if (near(frame.top(), area.top()))
        edges |= Qt::TopEdge;
    if (near(frame.left(), area.left()))
        edges |= Qt::LeftEdge;
    if (near(frame.right(), area.right()))
        edges |= Qt::RightEdge;
    if (near(frame.bottom(), area.bottom()))
        edges |= Qt::BottomEdge;

    const bool edgesChanged = !m_placed || edges != m_placedEdges || area != m_areaRect;
    const bool moved = frame.topLeft() != m_frameRect.topLeft() || frame.width() != m_frameRect.width();
    m_frameRect = frame;
    m_areaRect = area;
    m_placedEdges = edges;
    m_placed = true;
    if (edgesChanged) {
        updateLayout();
    } else if (moved && isTranslucent()) {
        // The shading follows the distance to the bar: only redraw when it
        // can show (near the bar or the right border).
        const qreal reach = 480;
        if (frame.left() - area.left() < reach || area.right() - frame.right() < reach)
            update(titleBar());
    }
}

Qt::Edges Decoration::frameEdges() const
{
    if (isMaximized())
        return Qt::LeftEdge | Qt::RightEdge | Qt::TopEdge | Qt::BottomEdge;
    if (m_placed)
        return m_placedEdges;
    return window()->adjacentScreenEdges();
}

qreal Decoration::distanceFromBar(qreal x) const
{
    if (!m_placed)
        return -1;
    switch (m_theme->barEdge()) {
    case Qt::LeftEdge:
        return m_frameRect.left() + x - m_areaRect.left();
    case Qt::RightEdge:
        return m_areaRect.right() - (m_frameRect.left() + x);
    default:
        return -1;
    }
}

qreal Decoration::titleHeight() const
{
    return std::round(buttonSize() + PaddingSmall);
}

qreal Decoration::buttonSize() const
{
    const QFontMetricsF fm(settings()->font());
    return std::round(std::max<qreal>(28, fm.height() * 1.6));
}

qreal Decoration::cornerRadius() const
{
    if (isMaximized())
        return 0;
    return RoundingLarge * (m_theme ? m_theme->roundingScale() : 1.0);
}

KDecoration3::BorderRadius Decoration::corners() const
{
    // Square where the window meets a screen edge (tiles fit together, and
    // corners sitting in a corner of the screen lie under Caelestia's frame:
    // the titlebar shapes its own top ones to it, see frameCornerPath()).
    // Free corners get the usual rounding.
    const qreal free = isMaximized() ? 0 : cornerRadius();
    const Qt::Edges e = frameEdges();
    const auto round = [&](Qt::Edge a, Qt::Edge b) {
        return isMaximized() || (e & a) || (e & b) ? 0.0 : free;
    };
    return KDecoration3::BorderRadius(round(Qt::TopEdge, Qt::LeftEdge),
                                      round(Qt::TopEdge, Qt::RightEdge),
                                      round(Qt::BottomEdge, Qt::RightEdge),
                                      round(Qt::BottomEdge, Qt::LeftEdge));
}

bool Decoration::inFrameCorner(Qt::Edge side) const
{
    // A top corner sitting in a corner of the screen, under the frame's curve.
    if (m_theme->frameCornerRadius() <= 0 || !isTranslucent())
        return false;
    const Qt::Edges e = frameEdges();
    return (e & Qt::TopEdge) && (e & side);
}

QPointF Decoration::frameCornerCentre(Qt::Edge side) const
{
    // Centre of Caelestia's corner as configured: where its blur cut sits.
    const qreal r = m_theme->frameCornerRadius();
    return side == Qt::LeftEdge ? QPointF(r, r) : QPointF(window()->width() - r, r);
}

QPointF Decoration::frameCurveCentre(Qt::Edge side) const
{
    // Centre of the frame's drawn curve. When the shell part moves the
    // frame's edges over the window, the curve moves with both edges, so its
    // centre shifts diagonally (the blur cut does not move).
    const qreal shift = m_theme->frameEdgeShift();
    const QPointF c = frameCornerCentre(side);
    return c + QPointF(side == Qt::LeftEdge ? shift : -shift, shift);
}

QPainterPath Decoration::frameCornerOutside(Qt::Edge side) const
{
    // The part of the corner square that is plain, blurred frame: past the
    // curve and past the unblurred band along it. Nothing of ours goes there.
    const qreal r = m_theme->frameCornerRadius();
    const QPointF c = frameCornerCentre(side);
    QPainterPath square;
    square.addRect(side == Qt::LeftEdge ? QRectF(0, 0, r, r) : QRectF(c.x(), 0, r, r));
    return square.subtracted(frameCornerReach(side));
}

QPainterPath Decoration::frameCornerReach(Qt::Edge side) const
{
    // How far under the frame the titlebar reaches at a corner: past the
    // curve's soft edge, and over all of Caelestia's unblurred staircase.
    const qreal r = m_theme->frameCornerRadius();
    const QPointF c = frameCornerCentre(side);
    QPainterPath reach;
    reach.addEllipse(c, r + Theme::FrameCornerBand, r + Theme::FrameCornerBand);
    QPainterPath steps = m_theme->frameCornerUnblurred();
    if (side == Qt::RightEdge) {
        // Mirror the staircase into the right corner.
        steps = QTransform(-1, 0, 0, 1, window()->width(), 0).map(steps);
    }
    return reach.united(steps);
}

bool Decoration::isMaximized() const
{
    return window()->isMaximized();
}

QColor Decoration::titleBarColour() const
{
    // Exactly the colour of Caelestia's bar and screen frame: surface at the
    // shell's base transparency (solid when transparency is off).
    // The same material whether windowed, tiled or maximized.
    return m_theme->frameColour();
}

bool Decoration::touchesFrame() const
{
    return frameEdges() & Qt::TopEdge;
}

QColor Decoration::titleBarColourAt(qreal x) const
{
    if (!isTranslucent())
        return titleBarColour();

    // One material for every titlebar: Caelestia's frame as it looks over a
    // window, i.e. with the app's own background blended in the way the
    // frame's blur takes it in, lighter towards the bar. A titlebar gets the
    // same shading a maximized one has at that same spot on screen, wherever
    // the window is.
    const QColor app = window()->palette().color(QPalette::Active, QPalette::Window);
    const Qt::Edge bar = m_theme->barEdge();
    if (bar == Qt::TopEdge || bar == Qt::BottomEdge)
        return m_theme->frameColourWithApp(app, Theme::frameAppShare(touchesFrame() ? 0 : 1e6, true));

    const qreal open = Theme::frameAppShare(1e6, false);
    qreal share = open;
    const auto dent = [&](qreal distance, bool barSide) {
        share -= open - Theme::frameAppShare(distance, barSide);
    };
    if (m_placed) {
        const qreal screenX = m_frameRect.left() + x;
        const bool barLeft = bar == Qt::LeftEdge;
        dent(barLeft ? screenX - m_areaRect.left() : m_areaRect.right() - screenX, true);
        dent(barLeft ? m_areaRect.right() - screenX : screenX - m_areaRect.left(), false);
    } else if (touchesFrame()) {
        // Without KWin (e.g. a preview): by the edges the window reports.
        const Qt::Edges edges = frameEdges();
        if (edges & Qt::LeftEdge)
            dent(x, bar == Qt::LeftEdge);
        if (edges & Qt::RightEdge)
            dent(window()->width() - x, bar == Qt::RightEdge);
    }
    return m_theme->frameColourWithApp(app, share);
}

QBrush Decoration::titleBarBrush() const
{
    if (!isTranslucent())
        return titleBarColour();
    if (!m_placed && !touchesFrame())
        return titleBarColourAt(window()->width() / 2);

    const qreal width = window()->width();
    QLinearGradient gradient(0, 0, width, 0);
    // Dense near the ends where it changes, one stop in the open middle.
    for (qreal d : {0.0, 8.0, 16.0, 32.0, 48.0, 64.0, 96.0, 128.0, 176.0, 240.0, 320.0, 420.0}) {
        if (d > width / 2)
            break;
        gradient.setColorAt(d / width, titleBarColourAt(d));
        gradient.setColorAt(1 - d / width, titleBarColourAt(width - d));
    }
    gradient.setColorAt(0.5, titleBarColourAt(width / 2));
    return gradient;
}

void Decoration::rebuildGeometry()
{
    m_geometryValid = false;
    m_titlePath = buildTitleBarPath();
    const qreal r = m_theme->frameCornerRadius();
    const qreal inner = std::max<qreal>(0, r - 2);
    const qreal outer = r + Theme::FrameCornerBand;
    constexpr int Wedges = 8;
    for (int i = 0; i < 2; ++i) {
        const Qt::Edge side = i == 0 ? Qt::LeftEdge : Qt::RightEdge;
        m_cornerWedges[i].clear();
        if (!inFrameCorner(side))
            continue;
        const QPointF corner = frameCornerCentre(side);
        const QPointF c = frameCurveCentre(side);
        QPainterPath square;
        square.addRect(side == Qt::LeftEdge ? QRectF(0, 0, r, r) : QRectF(corner.x(), 0, r, r));
        QPainterPath hole;
        hole.addEllipse(c, inner, inner);
        const QPainterPath area = square.intersected(frameCornerReach(side).subtracted(hole));
        // Qt angles: 0 = right, 90 = up. Left corner faces 90..180, right 0..90.
        const qreal start = side == Qt::LeftEdge ? 90 : 0;
        for (int w = 0; w < Wedges; ++w) {
            QPainterPath wedge;
            wedge.moveTo(c);
            wedge.arcTo(QRectF(c.x() - outer - 4, c.y() - outer - 4, (outer + 4) * 2, (outer + 4) * 2), start + 90.0 * w / Wedges, 90.0 / Wedges);
            wedge.closeSubpath();
            m_cornerWedges[i].append(area.intersected(wedge));
        }
    }
    m_geometryValid = true;
}

void Decoration::paintFrameCorners(QPainter *painter) const
{
    // Under the frame's curve: across its soft edge and its unblurred band,
    // hold exactly what makes "frame over us" look like the titlebar next to
    // it, so no line of wallpaper or double tint shows. Done in thin wedges,
    // each from the titlebar colour at that spot, so it meets the rest of
    // the titlebar without a step. The wedge shapes are built per layout
    // (rebuildGeometry); only their colours are worked out here.
    const qreal r = m_theme->frameCornerRadius();
    const qreal inner = std::max<qreal>(0, r - 2);
    const qreal outer = r + Theme::FrameCornerBand;
    for (int i = 0; i < 2; ++i) {
        const QList<QPainterPath> &wedges = m_cornerWedges[i];
        if (wedges.isEmpty())
            continue;
        const Qt::Edge side = i == 0 ? Qt::LeftEdge : Qt::RightEdge;
        const QPointF c = frameCurveCentre(side);
        const qreal start = side == Qt::LeftEdge ? 90 : 0;
        const int count = wedges.size();
        for (int w = 0; w < count; ++w) {
            const qreal mid = qDegreesToRadians(start + 90.0 * (w + 0.5) / count);
            const QColor target = titleBarColourAt(c.x() + std::cos(mid) * r);
            QRadialGradient gradient(c, outer);
            gradient.setColorAt(0, target);
            for (qreal d = inner; d <= outer + 0.001; d += 0.25)
                gradient.setColorAt(std::min<qreal>(1, d / outer), m_theme->underFrame(target, m_theme->frameCornerCoverage(d)));
            painter->fillPath(wedges[w], gradient);
        }
    }
}

void Decoration::paintFrameEdges(QPainter *painter) const
{
    // The frame's soft straight edge also lies over the titlebar's outermost
    // pixel rows (and columns, where it meets the bar or the side border),
    // tinting them twice. Hold there what makes "frame over it" look like
    // the titlebar next to it.
    if (!touchesFrame() || !isTranslucent())
        return;
    const qreal width = window()->width();
    const qreal height = titleHeight();
    const qreal r = m_theme->frameCornerRadius();
    const KDecoration3::BorderRadius c = corners();
    const Qt::Edges edges = frameEdges();
    const qreal left = inFrameCorner(Qt::LeftEdge) ? r : c.topLeft();
    const qreal right = inFrameCorner(Qt::RightEdge) ? r : c.topRight();

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, false);
    painter->setCompositionMode(QPainter::CompositionMode_Source);
    for (int px = 0; px < 2; ++px) {
        const qreal coverage = m_theme->frameEdgeCoverage(px);
        if (coverage < 0.01)
            continue;
        if ((edges & Qt::TopEdge) && width - left - right > 0) {
            QLinearGradient row(left, 0, width - right, 0);
            const int stops = 24;
            for (int i = 0; i <= stops; ++i) {
                const qreal x = left + (width - left - right) * i / stops;
                row.setColorAt(qreal(i) / stops, m_theme->underFrame(titleBarColourAt(x), coverage));
            }
            painter->fillRect(QRectF(left, px, width - left - right, 1), row);
        }
        const qreal top = (edges & Qt::TopEdge) ? 2 : 0;
        if ((edges & Qt::LeftEdge) && height > top) {
            const qreal from = (edges & Qt::TopEdge) && inFrameCorner(Qt::LeftEdge) ? r : top;
            if (height > from)
                painter->fillRect(QRectF(px, from, 1, height - from), m_theme->underFrame(titleBarColourAt(px + 0.5), coverage));
        }
        if ((edges & Qt::RightEdge) && height > top) {
            const qreal from = (edges & Qt::TopEdge) && inFrameCorner(Qt::RightEdge) ? r : top;
            if (height > from)
                painter->fillRect(QRectF(width - 1 - px, from, 1, height - from), m_theme->underFrame(titleBarColourAt(width - px - 0.5), coverage));
        }
    }
    painter->restore();
}

QColor Decoration::outlineColour() const
{
    QColor c = m_theme->colour(QStringLiteral("outlineVariant"));
    c.setAlphaF(0.55 + (0.9 - 0.55) * std::clamp<qreal>(m_active->value(), 0, 1));
    return c;
}

bool Decoration::isTranslucent() const
{
    return m_theme->transparencyEnabled() && titleBarColour().alphaF() < 1.0 && settings()->isAlphaChannelSupported();
}

QPainterPath Decoration::titleBarPath() const
{
    return m_geometryValid ? m_titlePath : buildTitleBarPath();
}

QPainterPath Decoration::buildTitleBarPath() const
{
    const QRectF bar(0, 0, window()->width(), titleHeight());
    const KDecoration3::BorderRadius c = corners();
    const qreal tl = c.topLeft();
    const qreal tr = c.topRight();

    // Rounded on top (where allowed), square where it meets the window.
    QPainterPath path;
    path.moveTo(bar.left(), bar.bottom());
    path.lineTo(bar.left(), bar.top() + tl);
    if (tl > 0)
        path.arcTo(QRectF(bar.left(), bar.top(), tl * 2, tl * 2), 180, -90);
    path.lineTo(bar.right() - tr, bar.top());
    if (tr > 0)
        path.arcTo(QRectF(bar.right() - tr * 2, bar.top(), tr * 2, tr * 2), 90, -90);
    path.lineTo(bar.right(), bar.bottom());
    path.closeSubpath();
    for (Qt::Edge side : {Qt::LeftEdge, Qt::RightEdge}) {
        if (inFrameCorner(side))
            path = path.subtracted(frameCornerOutside(side));
    }
    return path;
}

void Decoration::updateBlur()
{
    // Blur behind the titlebar with KWin's blur effect, the same one that
    // blurs behind Caelestia's bar, when the shell blurs too.
    if (isTranslucent() && m_theme->blurEnabled())
        setBlurRegion(QRegion(titleBarPath().toFillPolygon().toPolygon()));
    else
        setBlurRegion(QRegion());
}

QColor Decoration::foregroundColour() const
{
    const QColor a = m_theme->colour(QStringLiteral("onSurface"));
    const QColor b = m_theme->colour(QStringLiteral("onSurfaceVariant"));
    const qreal t = std::clamp<qreal>(m_active->value(), 0, 1);
    return QColor::fromRgbF(b.redF() + (a.redF() - b.redF()) * t,
                            b.greenF() + (a.greenF() - b.greenF()) * t,
                            b.blueF() + (a.blueF() - b.blueF()) * t);
}

void Decoration::updateLayout()
{
    if (!m_kwinWindow)
        findKWinWindow();
    const qreal height = titleHeight();
    const qreal width = window()->width();

    setBorders(QMarginsF(0, height, 0, 0));

    // Invisible edges to grab for resizing, since the frame has no side borders.
    const qreal grab = isMaximized() ? 0 : 6;
    setResizeOnlyBorders(QMarginsF(grab, grab, grab, grab));

    setTitleBar(QRectF(0, 0, width, height));
    const KDecoration3::BorderRadius radius = corners();
    setBorderRadius(radius);

    // A thin line around windowed windows only; maximized ones blend into
    // Caelestia's own frame instead.
    if (isMaximized())
        setBorderOutline(KDecoration3::BorderOutline());
    else
        setBorderOutline(KDecoration3::BorderOutline(OutlineWidth, outlineColour(), radius));

    setOpaque(!isTranslucent());
    rebuildGeometry();
    updateBlur();

    updateButtonsGeometry();
    update();
}

void Decoration::updateColours()
{
    // Focus fades touch only colours: the outline and a titlebar repaint.
    if (!isMaximized())
        setBorderOutline(KDecoration3::BorderOutline(OutlineWidth, outlineColour(), corners()));
    update(titleBar());
}

void Decoration::updateButtonsGeometry()
{
    if (!m_leftButtons || !m_rightButtons)
        return;

    const qreal size = buttonSize();
    for (auto *group : {m_leftButtons, m_rightButtons}) {
        for (const auto &button : group->buttons())
            button->setGeometry(QRectF(0, 0, size, size));
    }

    const qreal height = titleHeight();
    // Where a top corner follows the frame's curve, keep the buttons (even
    // pressed, as rounded squares) clear of it.
    const qreal left = PaddingSmall + (inFrameCorner(Qt::LeftEdge) ? 4 : 0);
    const qreal right = PaddingSmall + (inFrameCorner(Qt::RightEdge) ? 4 : 0);
    const qreal top = (height - size) / 2;
    m_leftButtons->setPos(QPointF(left, top));
    m_rightButtons->setPos(QPointF(window()->width() - right - m_rightButtons->geometry().width(), top));
}

QFont Decoration::captionFont() const
{
    QFont font = settings()->font();
    font.setWeight(QFont::Medium);
    return font;
}

QRectF Decoration::chevronRect() const
{
    // The small "expand_more" next to the title (it opens the window menu),
    // placed right after the caption as drawn.
    if (!m_leftButtons || !m_rightButtons)
        return {};
    const qreal size = buttonSize() * 0.8;
    const qreal left = m_leftButtons->geometry().right() + SpacingSmall;
    const qreal right = m_rightButtons->geometry().left() - SpacingSmall * 2;
    const qreal space = right - left - size;
    if (space <= 0)
        return {};
    const qreal text = std::min(QFontMetricsF(captionFont()).horizontalAdvance(window()->caption()), space);
    return QRectF(left + text + 2, (titleHeight() - size) / 2, size, size);
}

void Decoration::paint(QPainter *painter, const QRectF &repaintArea)
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    // Source, not over: the translucent colour replaces what is there, so the
    // alpha is exactly Caelestia's and does not stack.
    painter->setCompositionMode(QPainter::CompositionMode_Source);
    painter->fillPath(titleBarPath(), titleBarBrush());
    paintFrameCorners(painter);
    paintFrameEdges(painter);
    painter->setCompositionMode(QPainter::CompositionMode_SourceOver);

    // App icon (the left buttons), then the caption and its chevron.
    const QRectF chevron = chevronRect();
    if (!chevron.isEmpty()) {
        const QFont font = captionFont();
        const qreal left = m_leftButtons->geometry().right() + SpacingSmall;
        const QRectF caption(left, 0, chevron.left() - 2 - left, titleHeight());
        const QString text = QFontMetricsF(font).elidedText(window()->caption(), Qt::ElideRight, caption.width() + 1);
        painter->setFont(font);
        painter->setPen(foregroundColour());
        painter->drawText(caption, Qt::AlignVCenter | Qt::AlignLeft | Qt::TextSingleLine, text);

        m_chevronLayer->paint(painter, chevron, foregroundColour(), 0.08);
        painter->setFont(Theme::iconFont(chevron.height() * 0.78));
        painter->setPen(foregroundColour());
        painter->drawText(chevron, Qt::AlignCenter, QStringLiteral("expand_more"));
    }

    m_leftButtons->paint(painter, repaintArea);
    m_rightButtons->paint(painter, repaintArea);

    painter->restore();
}

void Decoration::hoverMoveEvent(QHoverEvent *event)
{
    KDecoration3::Decoration::hoverMoveEvent(event);
    const bool hovered = chevronRect().contains(event->position());
    if (hovered != m_chevronHovered) {
        m_chevronHovered = hovered;
        m_chevronLayer->setHovered(hovered);
    }
}

void Decoration::hoverLeaveEvent(QHoverEvent *event)
{
    KDecoration3::Decoration::hoverLeaveEvent(event);
    if (m_chevronHovered || m_chevronPressed) {
        m_chevronHovered = m_chevronPressed = false;
        m_chevronLayer->setHovered(false);
        m_chevronLayer->release();
    }
}

void Decoration::mousePressEvent(QMouseEvent *event)
{
    // The chevron opens the window menu; taking the press keeps KWin from
    // starting a move. Everywhere else the titlebar drags as usual.
    const QRectF chevron = chevronRect();
    if (event->button() == Qt::LeftButton && chevron.contains(event->position())) {
        m_chevronPressed = true;
        m_chevronLayer->press(event->position());
        event->accept();
        return;
    }
    KDecoration3::Decoration::mousePressEvent(event);
}

void Decoration::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_chevronPressed && event->button() == Qt::LeftButton) {
        m_chevronPressed = false;
        m_chevronLayer->release();
        const QRectF chevron = chevronRect();
        if (chevron.contains(event->position()))
            requestShowWindowMenu(chevron.toAlignedRect());
        event->accept();
        return;
    }
    KDecoration3::Decoration::mouseReleaseEvent(event);
}

}
