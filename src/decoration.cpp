// SPDX-FileCopyrightText: 2026 COCONAT10MEN
// SPDX-License-Identifier: MIT

#include "decoration.h"
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
#include <QPainter>
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
    return true;
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
    // A corner sitting in a corner of the screen meets Caelestia's frame,
    // whose inner corners are rounded: take exactly that curve, so the frame
    // never lies over the titlebar there (it would tint it twice). Corners
    // along one screen edge stay square so tiles fit together; free corners
    // get the usual rounding.
    const qreal free = isMaximized() ? 0 : cornerRadius();
    const qreal frame = m_theme->frameCornerRadius();
    const Qt::Edges e = isMaximized() ? Qt::Edges(Qt::TopEdge | Qt::BottomEdge | Qt::LeftEdge | Qt::RightEdge) : window()->adjacentScreenEdges();
    const auto round = [&](Qt::Edge a, Qt::Edge b) {
        const bool onA = e & a;
        const bool onB = e & b;
        if (onA && onB)
            return frame;
        return onA || onB ? 0.0 : free;
    };
    return KDecoration3::BorderRadius(round(Qt::TopEdge, Qt::LeftEdge),
                                      round(Qt::TopEdge, Qt::RightEdge),
                                      round(Qt::BottomEdge, Qt::RightEdge),
                                      round(Qt::BottomEdge, Qt::LeftEdge));
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
    return isMaximized() || (window()->adjacentScreenEdges() & Qt::TopEdge);
}

QBrush Decoration::titleBarBrush() const
{
    if (!touchesFrame() || !isTranslucent())
        return titleBarColour();

    // Mirror what Caelestia's blur does to the frame right above: blend in
    // the app's own background, by how much the frame's blur takes it in at
    // each point along the top (less towards the bar).
    const QColor app = window()->palette().color(QPalette::Active, QPalette::Window);
    const Qt::Edges edges = isMaximized() ? Qt::Edges(Qt::LeftEdge | Qt::RightEdge | Qt::TopEdge) : window()->adjacentScreenEdges();
    const Qt::Edge bar = m_theme->barEdge();
    const qreal width = window()->width();
    if (bar == Qt::TopEdge)
        return m_theme->frameColourWithApp(app, Theme::frameAppShare(0, true));

    const auto shareAt = [&](qreal x) {
        qreal share = Theme::frameAppShare(1e6, false);
        const auto dent = [&](qreal distance, bool barSide) {
            share -= Theme::frameAppShare(1e6, false) - Theme::frameAppShare(distance, barSide);
        };
        if (edges & Qt::LeftEdge)
            dent(x, bar == Qt::LeftEdge);
        if (edges & Qt::RightEdge)
            dent(width - x, bar == Qt::RightEdge);
        return share;
    };

    QLinearGradient gradient(0, 0, width, 0);
    // Dense near the ends where it changes, one stop in the open middle.
    for (qreal d : {0.0, 8.0, 16.0, 32.0, 48.0, 64.0, 96.0, 128.0, 176.0, 240.0, 320.0, 420.0}) {
        if (d > width / 2)
            break;
        gradient.setColorAt(d / width, m_theme->frameColourWithApp(app, shareAt(d)));
        gradient.setColorAt(1 - d / width, m_theme->frameColourWithApp(app, shareAt(width - d)));
    }
    gradient.setColorAt(0.5, m_theme->frameColourWithApp(app, shareAt(width / 2)));
    return gradient;
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
    const KDecoration3::BorderRadius c = corners();
    const qreal left = PaddingSmall + (c.topLeft() > cornerRadius() ? 4 : 0);
    const qreal right = PaddingSmall + (c.topRight() > cornerRadius() ? 4 : 0);
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
