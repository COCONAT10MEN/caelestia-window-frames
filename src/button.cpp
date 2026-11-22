// SPDX-FileCopyrightText: 2026 COCONAT10MEN
// SPDX-License-Identifier: MIT

#include "button.h"
#include "decoration.h"
#include "icons.h"
#include "statelayer.h"
#include "theme.h"

#include <KDecoration3/DecoratedWindow>

#include <QMouseEvent>
#include <QPainter>

#include <algorithm>
#include <cmath>

namespace Caelestia
{

using KDecoration3::DecorationButtonType;

Button::Button(QObject *parent, const QVariantList &args)
    : Button(args.at(0).value<DecorationButtonType>(), args.at(1).value<Decoration *>(), parent)
{
}

Button::Button(DecorationButtonType type, Decoration *decoration, QObject *parent)
    : KDecoration3::DecorationButton(type, decoration, parent)
    , m_layer(std::make_unique<StateLayer>(decoration->theme()))
{
    // Only this button's square repaints while its state animates.
    connect(m_layer.get(), &StateLayer::changed, this, [this] {
        update();
    });
    connect(this, &DecorationButton::hoveredChanged, this, [this] {
        m_layer->setHovered(isHovered());
    });
    connect(this, &DecorationButton::pressedChanged, this, [this] {
        if (!isPressed())
            m_layer->release();
    });
    connect(this, &DecorationButton::checkedChanged, this, [this] {
        update();
    });
}

Button::~Button() = default;

void Button::mousePressEvent(QMouseEvent *event)
{
    KDecoration3::DecorationButton::mousePressEvent(event);
    if (isPressed())
        m_layer->press(event->position());
}

KDecoration3::DecorationButton *Button::create(DecorationButtonType type, KDecoration3::Decoration *decoration, QObject *parent)
{
    auto *d = qobject_cast<Decoration *>(decoration);
    if (!d)
        return nullptr;
    return new Button(type, d, parent);
}

QString Button::glyph() const
{
    // Material Symbols Rounded ligatures, the icon font Caelestia uses.
    switch (type()) {
    case DecorationButtonType::Close:
        return QStringLiteral("close");
    case DecorationButtonType::Minimize:
        return QStringLiteral("minimize");
    case DecorationButtonType::Maximize:
        return isChecked() ? QStringLiteral("filter_none") : QStringLiteral("crop_square");
    case DecorationButtonType::OnAllDesktops:
        return QStringLiteral("keep");
    case DecorationButtonType::KeepAbove:
        return QStringLiteral("keyboard_double_arrow_up");
    case DecorationButtonType::KeepBelow:
        return QStringLiteral("keyboard_double_arrow_down");
    case DecorationButtonType::Shade:
        return isChecked() ? QStringLiteral("expand_more") : QStringLiteral("expand_less");
    case DecorationButtonType::ContextHelp:
        return QStringLiteral("help");
    case DecorationButtonType::ApplicationMenu:
        return QStringLiteral("menu");
    case DecorationButtonType::ExcludeFromCapture:
        return QStringLiteral("visibility_off");
    default:
        return {};
    }
}

void Button::paint(QPainter *painter, const QRectF &repaintArea)
{
    Q_UNUSED(repaintArea)
    auto *d = qobject_cast<Decoration *>(decoration());
    if (!d || !isVisible())
        return;

    const QRectF r = geometry();
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    if (type() == DecorationButtonType::Menu) {
        // The window's own icon.
        const qreal icon = std::round(r.height() * 0.68);
        const QRectF iconRect(r.center().x() - icon / 2, r.center().y() - icon / 2, icon, icon);
        d->window()->icon().paint(painter, iconRect.toRect());
        painter->restore();
        return;
    }

    Theme *theme = d->theme();
    const bool close = type() == DecorationButtonType::Close;
    QColor fg = d->foregroundColour();

    if (isEnabled()) {
        if (close) {
            // Close fills with the error colour on hover, its icon turning
            // onError as the fill comes in.
            const qreal h = std::clamp<qreal>(m_layer->hover(), 0, 1);
            if (h > 0) {
                QColor err = theme->colour(QStringLiteral("error"));
                err.setAlphaF(h);
                painter->setPen(Qt::NoPen);
                painter->setBrush(err);
                painter->drawPath(m_layer->shape(r));
                const QColor on = theme->colour(QStringLiteral("onError"));
                fg = QColor::fromRgbF(fg.redF() + (on.redF() - fg.redF()) * h,
                                      fg.greenF() + (on.greenF() - fg.greenF()) * h,
                                      fg.blueF() + (on.blueF() - fg.blueF()) * h);
            }
            m_layer->paint(painter, r, theme->colour(QStringLiteral("onError")), 0);
        } else {
            m_layer->paint(painter, r, fg, 0.08);
        }
    }

    if (!isEnabled())
        fg.setAlphaF(0.38);

    // The three window controls are drawn strokes; the rarer extra buttons
    // use Caelestia's icon font.
    switch (type()) {
    case DecorationButtonType::Minimize:
        Icons::paint(painter, Icons::Kind::Minimize, r, fg);
        break;
    case DecorationButtonType::Maximize:
        Icons::paint(painter, isChecked() ? Icons::Kind::Restore : Icons::Kind::Maximize, r, fg);
        break;
    case DecorationButtonType::Close:
        Icons::paint(painter, Icons::Kind::Close, r, fg);
        break;
    default:
        painter->setFont(Theme::iconFont(r.height() * 0.62));
        painter->setPen(fg);
        painter->drawText(r, Qt::AlignCenter, glyph());
        break;
    }

    painter->restore();
}

}
