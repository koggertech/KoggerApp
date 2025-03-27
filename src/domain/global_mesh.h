#pragma once

#include <vector>
#include <QVector3D>
#include "draw_utils.h"
#include "tile.h"


using namespace sscan;

class SideScanView;
class GlobalMesh {
public:
    /*methods*/
    GlobalMesh(int tileSidePixelSize, int tileHeightMatrixRatio, float tileResolution);
    ~GlobalMesh();

    void reinit(int tileSidePixelSize, int heightMatrixRatio, float tileResolution);
    bool concatenate(MatrixParams& actualMatParams);    
    QVector3D convertPhToPixCoords(QVector3D phCoords) const;
    void printMatrix() const;
    void clear();

    void setGenerateGridContour(bool state);
    std::vector<std::vector<Tile*>>& getTileMatrixRef();
    Tile*                            getTilePtrById(QUuid tileId);
    int                              getPixelWidth() const;
    int                              getPixelHeight() const;
    int                              getTileSidePixelSize() const;
    int                              getNumWidthTiles() const;
    int                              getNumHeightTiles() const;
    int                              getStepSizeHeightMatrix() const;
    bool                             getIsInited() const;

private:
    /*methods*/
    void initializeMatrix(int numWidthTiles, int numHeightTiles, const MatrixParams& matrixParams);
    void resizeColumnsRight(int columnsToAdd);
    void resizeRowsTop(int rowsToAdd);
    void resizeColumnsLeft(int columnsToAdd);
    void resizeRowsBottom(int rowsToAdd);    
    float getWidthMeters() const;
    float getHeightMeters() const;

    /*data*/
    std::vector<Tile*> tiles_;
    std::vector<std::vector<Tile*>> tileMatrix_;
    QVector3D origin_;
    float tileResolution_;
    float tileSideMeterSize_;
    int numWidthTiles_;
    int numHeightTiles_;
    int tileSidePixelSize_;
    int tileHeightMatrixRatio_;
    bool generateGridContour_;
};
