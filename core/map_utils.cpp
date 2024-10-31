#include "map_utils.h"

#include <QPolygonF>
#include <QPointF>
#include <cmath>


namespace map {


TileCalculator::TileCalculator(float tileSize)
    : m_tileSize(tileSize)
{
}

void TileCalculator::setTrapezoid(const QVector<QVector3D>& trapezoidVertices)
{
    if (trapezoidVertices.size() != 4) {
        return;
    }
    m_trapezoid = trapezoidVertices;
}

QVector<Tile> TileCalculator::calculateTiles() const
{
    QVector<Tile> tiles;

    if (m_trapezoid.size() != 4) {
        return tiles;
    }

    QVector<QVector3D> bbox = getBoundingBox();
    float minX = bbox[0].x();
    float minY = bbox[0].y();
    float maxX = bbox[1].x();
    float maxY = bbox[1].y();

    for (float x = std::floor(minX); x <= (maxX + m_tileSize); x += m_tileSize) {
        for (float y = std::floor(minY); y <= (maxY + m_tileSize); y += m_tileSize) {
            QVector3D tileCenter(x + m_tileSize / 2, y + m_tileSize / 2, 0.0f);
            if (isPointInsideTrapezoid(tileCenter)) {
                Tile tile;
                tile.position = tileCenter;
                tiles.append(tile);
            }
        }
    }

    return tiles;
}

bool TileCalculator::isPointInsideTrapezoid(const QVector3D& point) const
{
    QPolygonF polygon;
    for (const auto& vertex : m_trapezoid) {
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

    for (const auto& vertex : m_trapezoid) {
        if (vertex.x() < minX) minX = vertex.x();
        if (vertex.y() < minY) minY = vertex.y();
        if (vertex.x() > maxX) maxX = vertex.x();
        if (vertex.y() > maxY) maxY = vertex.y();
    }

    QVector<QVector3D> bbox;
    bbox.append(QVector3D(minX, minY, 0.0f));
    bbox.append(QVector3D(maxX, maxY, 0.0f));

    return bbox;
}

}
