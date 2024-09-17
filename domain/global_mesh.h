#pragma once

#include <cmath>
#include <QVector>

#include "drawutils.h"

#include "tile.h"


class GlobalMesh {
public:
    GlobalMesh() {};
    ~GlobalMesh();

    bool concatenate(MatrixParams& actualMatParams);
    void printMatrix() const;

    float getWidthMeters() const;
    float getHeightMeters() const;

    int getWidthPixels() const;
    int getHeightPixels() const;


    int getNumWidthTiles() const;
    int getNumHeightTiles() const;

    std::vector<std::vector<Tile*>>& getTileMatrixRef();

    float getTileMetersSize() const;
    int getTilePixelSize() const;

    int getHeightVerticeRatio() const;

    float getHeightMetersStep() const;
    int getHeightPixelStep() const;

    QVector3D getOrigin() const;

    void clear();

    QVector3D convertPhysicsCoordinateToPixel (QVector3D physicsCoordinate) const;

private:
    /*methods*/
    void initializeMatrix(int numWidthTiles, int numHeightTiles, const MatrixParams& matrixParams);

    void resizeColumnsRight(int columnsToAdd);
    void resizeRowsTop(int rowsToAdd);
    void resizeColumnsLeft(int columnsToAdd);
    void resizeRowsBottom(int rowsToAdd);

    /*data*/
    std::vector<Tile*> tiles_;
    std::vector<std::vector<Tile*>> tileMatrix_;

    QVector3D origin_;

    int numWidthTiles_ = 0;
    int numHeightTiles_ = 0;

    const int tileSizePixels_ = 256;
    const float resolution_ = 0.1; // 0.1 метр - 1 пиксель
    const float tileSizeMeters_ = tileSizePixels_ * resolution_;

    const int heightVerticeRatio_ = 16;

    int count_ = 0; // debug
};

