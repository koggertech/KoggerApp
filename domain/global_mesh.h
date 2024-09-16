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
    int getWidthMeters() const;
    int getHeightMeters() const;

    int getNumWidthTiles() const;
    int getNumHeightTiles() const;

    std::vector<std::vector<Tile*>>& getTileMatrixRef();

    int getTileSize() const;
    int getHeightVerticeRatio() const;
    int getHeightStep() const;


    QVector3D getOrigin() const;

    void clear();


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

    const int tileSizeMeters_ = 10; // meterpx w,h
    const float resolution = 0.1; // 0.1 метр - 1 пиксель

    const int heightVerticeRatio_ = 5; // в пять раз меньше чем пикселей

    int count_ = 0; // debug
};

