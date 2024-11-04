#include "map_defs.h"

#include <QPolygonF>
#include <QPointF>
#include <cmath>


namespace map {


TileCalculator::TileCalculator(float tileSize)
    : tileSize_(tileSize)
{
}

void TileCalculator::setTrapezoid(const QVector<QVector3D>& trapezoidVertices)
{
    if (trapezoidVertices.size() != 4) {
        return;
    }
    trapezoid_ = trapezoidVertices;
}

QVector<Tile> TileCalculator::calculateTiles() const
{
    QVector<Tile> tiles;

    if (trapezoid_.size() != 4) {
        return tiles;
    }

    QVector<QVector3D> bBox = getBoundingBox();
    float minX = bBox[0].x();
    float minY = bBox[0].y();
    float maxX = bBox[1].x();
    float maxY = bBox[1].y();

    for (float x = std::floor(minX); x <= (maxX + tileSize_); x += tileSize_) {
        for (float y = std::floor(minY); y <= (maxY + tileSize_); y += tileSize_) {
            QVector3D tileCenter(x + tileSize_ / 2, y + tileSize_ / 2, 0.0f);
            if (isPointInsideTrapezoid(tileCenter)) {
                Tile tile;
                tile.setVertexNed(tileCenter);
                tiles.append(tile);
            }
        }
    }

    return tiles;
}

bool TileCalculator::isPointInsideTrapezoid(const QVector3D& point) const
{
    QPolygonF polygon;
    for (const auto& vertex : trapezoid_) {
        polygon << QPointF(vertex.x(), vertex.y());
    }

    return polygon.containsPoint(QPointF(point.x(), point.y()), Qt::OddEvenFill);
}

QVector<QVector3D> TileCalculator::getBoundingBox() const
{
    float minX = std::numeric_limits<float>::max();
    float minY = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float maxY = std::numeric_limits<float>::lowest();

    for (const auto& vertex : trapezoid_) {
        if (vertex.x() < minX) minX = vertex.x();
        if (vertex.y() < minY) minY = vertex.y();
        if (vertex.x() > maxX) maxX = vertex.x();
        if (vertex.y() > maxY) maxY = vertex.y();
    }

    QVector<QVector3D> bBox;
    bBox.append(QVector3D(minX, minY, 0.0f));
    bBox.append(QVector3D(maxX, maxY, 0.0f));

    return bBox;
}


} // namespace map
