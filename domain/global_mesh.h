#pragma once

#include <cmath>
#include <QVector>

#include "drawutils.h"

#include "tile.h"


class GlobalMesh {
public:
    GlobalMesh() = default;
    ~GlobalMesh();

    bool concatenate(MatrixParams& actualMatParams);
    void printMatrix() const;
    int getPixelWidth() const;
    int getPixelHeight() const;
    std::vector<std::vector<Tile*>>& getTileMatrixRef();

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

    const float pixelStep_ = 0.1;
    const int tileSize_ = 16; // px w,h
    const int heightRatio_ = 4;

    int count_ = 0; // debug

};
