// SPDX-FileCopyrightText: 2026 COCONAT10MEN
// SPDX-License-Identifier: MIT

#pragma once

#include <KDecoration3/Decoration>
#include <KDecoration3/DecorationButtonGroup>

#include <QBrush>
#include <QColor>
#include <QFont>
#include <QPainterPath>
#include <QPointer>
#include <QVariantList>

#include <memory>

namespace Caelestia
{

class Theme;
class StateLayer;
class Tween;

class Decoration : public KDecoration3::Decoration
{
    Q_OBJECT

public:
    explicit Decoration(QObject *parent = nullptr, const QVariantList &args = QVariantList());
    ~Decoration() override;

    bool init() override;
    void paint(QPainter *painter, const QRectF &repaintArea) override;

    Theme *theme() const { return m_theme.get(); }

    /// Background of the titlebar.
    QColor titleBarColour() const;
    /// The titlebar fill: titleBarColour(), or where the titlebar meets
    /// Caelestia's frame, the frame's look over this app (see Theme).
    QBrush titleBarBrush() const;
    QColor titleBarColourAt(qreal x) const;
    bool touchesFrame() const;
    /// The sides where this window meets Caelestia's frame (the edges of the
    /// work area inside the frame and bar), from its real place on screen.
    Qt::Edges frameEdges() const;
    /// Distance in px from the bar's edge to titlebar x, or -1 if unknown.
    qreal distanceFromBar(qreal x) const;
    bool inFrameCorner(Qt::Edge side) const;
    QPointF frameCornerCentre(Qt::Edge side) const;
    QPointF frameCurveCentre(Qt::Edge side) const;
    QPainterPath frameCornerOutside(Qt::Edge side) const;
    QPainterPath frameCornerReach(Qt::Edge side) const;
    void paintFrameCorners(QPainter *painter) const;
    /// Builds the titlebar's shape and the corner wedges once per layout
    /// (size, state, frame settings), so a repaint only fills them.
    void rebuildGeometry();
    QPainterPath buildTitleBarPath() const;
    void paintFrameEdges(QPainter *painter) const;
    /// Caption and icon colour.
    QColor foregroundColour() const;

    qreal titleHeight() const;
    qreal buttonSize() const;
    qreal cornerRadius() const;
    /// Per-corner radii: square on maximized windows and on tiled edges.
    KDecoration3::BorderRadius corners() const;
    QColor outlineColour() const;
    bool isMaximized() const;
    /// The titlebar is see-through (Caelestia's transparency is on).
    bool isTranslucent() const;

private:
    void updateLayout();
    void updateColours();
    void updateButtonsGeometry();
    void updateBlur();
    QPainterPath titleBarPath() const;
    QFont captionFont() const;
    QRectF chevronRect() const;

    void hoverMoveEvent(QHoverEvent *event) override;
    void hoverLeaveEvent(QHoverEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

    bool m_chevronHovered = false;
    bool m_chevronPressed = false;
    std::unique_ptr<StateLayer> m_chevronLayer;
    // 1 focused, 0 not: the caption, icons and outline fade between the two.
    std::unique_ptr<Tween> m_active;

    void findKWinWindow();
    void updatePlacement();
    // The window as KWin knows it: where it is, and the work area around it.
    QPointer<QObject> m_kwinWindow;
    QRectF m_frameRect;
    QRectF m_areaRect;
    Qt::Edges m_placedEdges;
    bool m_placed = false;

    QPainterPath m_titlePath;
    bool m_geometryValid = false;
    // Per top corner (0 left, 1 right): the wedges under the frame's curve.
    QList<QPainterPath> m_cornerWedges[2];

    std::shared_ptr<Theme> m_theme;
    KDecoration3::DecorationButtonGroup *m_leftButtons = nullptr;
    KDecoration3::DecorationButtonGroup *m_rightButtons = nullptr;
};

}
