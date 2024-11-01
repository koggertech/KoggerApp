#pragma once

#include <QVector>
#include <QVector3D>
#include <QPair>
#include "plotcash.h"


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
    bool isPointInsideTrapezoid(const QVector3D& point) const;
    QVector<QVector3D> getBoundingBox() const;

    /*data*/
    float tileSize_;
    QVector<QVector3D> trapezoid_;
};

} // namespace map

