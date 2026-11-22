// SPDX-FileCopyrightText: 2026 COCONAT10MEN
// SPDX-License-Identifier: MIT

#pragma once

#include <KDecoration3/DecorationButton>

#include <QVariantList>

#include <memory>

namespace Caelestia
{

class Decoration;
class StateLayer;

class Button : public KDecoration3::DecorationButton
{
    Q_OBJECT

public:
    /// Used by KWin's plugin loader (previews in System Settings).
    explicit Button(QObject *parent, const QVariantList &args);
    Button(KDecoration3::DecorationButtonType type, Decoration *decoration, QObject *parent = nullptr);

    static KDecoration3::DecorationButton *create(KDecoration3::DecorationButtonType type, KDecoration3::Decoration *decoration, QObject *parent);

    ~Button() override;

    void paint(QPainter *painter, const QRectF &repaintArea) override;

private:
    void mousePressEvent(QMouseEvent *event) override;
    QString glyph() const;

    std::unique_ptr<StateLayer> m_layer;
};

}
