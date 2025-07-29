#pragma once

#include <vector>
#include <QVector3D>
#include "draw_utils.h"
#include "surface_tile.h"


using namespace mosaic;

class SurfaceMesh {
public:
    /*methods*/
    SurfaceMesh(int tileSidePixelSize, int tileHeightMatrixRatio, float tileResolution);
    ~SurfaceMesh();

    void reinit(int tileSidePixelSize, int heightMatrixRatio, float tileResolution);
    bool concatenate(MatrixParams& actualMatParams);    
    QVector3D convertPhToPixCoords(QVector3D phCoords) const;
    void printMatrix() const;
    void clear();

    void setGenerateGridContour(bool state);
    const std::vector<SurfaceTile*>&        getTilesCRef() const;
    std::vector<std::vector<SurfaceTile*>>& getTileMatrixRef();
    SurfaceTile*                            getTilePtrById(QUuid tileId);
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
    std::vector<SurfaceTile*> tiles_;
    std::vector<std::vector<SurfaceTile*>> tileMatrix_;
    QVector3D origin_;
    float tileResolution_;
    float tileSideMeterSize_;
    int numWidthTiles_;
    int numHeightTiles_;
    int tileSidePixelSize_;
    int tileHeightMatrixRatio_;
    bool generateGridContour_;
};
