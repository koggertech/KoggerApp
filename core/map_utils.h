#pragma once

#include <QVector>
#include <QVector3D>
#include <QPair>


namespace map {

struct Tile {
    QVector3D position;
};

class TileCalculator
{
public:
    TileCalculator(float tileSize = 1.0f);

    void setTrapezoid(const QVector<QVector3D>& trapezoidVertices);

    QVector<Tile> calculateTiles() const;

private:
    float m_tileSize;
    QVector<QVector3D> m_trapezoid;

    bool isPointInsideTrapezoid(const QVector3D& point) const;
    QVector<QVector3D> getBoundingBox() const;
};

} // namespace map

