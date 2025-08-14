#pragma once

#include <stdint.h>
#include <QHash>
#include <QPair>
#include <QPointF>
#include <QReadWriteLock>
#include <QVector>
#include <QVector3D>
#include "delaunay.h"
#include "math_defs.h"
#include "isobaths_defs.h"


class BottomTrack;
class DataProcessor;
class SurfaceMesh;
class SurfaceTile;
class SurfaceProcessor
{
public:
    explicit SurfaceProcessor(DataProcessor* parent);
    ~SurfaceProcessor();

    void clear();

    void setBottomTrackPtr(BottomTrack* bottomTrackPtr);
    void setSurfaceMeshPtr(SurfaceMesh* surfaceMeshPtr);
    void onUpdatedBottomTrackData(const QVector<int>& indxs);
    void setTileResolution(float tileResolution);
    void setEdgeLimit(float val);
    void rebuildColorIntervals();
    void setSurfaceStepSize(float val);
    void setThemeId(int val);
    float getEdgeLimit() const;
    float getSurfaceStepSize() const;
    int getThemeId() const;

private:
    void writeTriangleToMesh(const QVector3D& A, const QVector3D& B, const QVector3D& C, QSet<SurfaceTile*>& updatedTiles);
    QVector<QVector3D> generateExpandedPalette(int totalColors) const;
    void updateTexture() const;
    void propagateBorderHeights(QSet<SurfaceTile*>& changedTiles);
    void refreshAfterEdgeLimitChange();

private:
    DataProcessor* dataProcessor_;
    BottomTrack* bottomTrackPtr_;
    SurfaceMesh* surfaceMeshPtr_;
    delaunay::Delaunay delaunayProc_;
    QReadWriteLock lock_;
    QHash<uint64_t, QVector<int>> pointToTris_;
    QHash<QPair<int,int>, QVector3D>  cellPoints_; // fir - virt indx, sec - indx in tr
    QHash<QPair<int,int>, int>  cellPointsInTri_;
    QHash<int, uint64_t> bTrToTrIndxs_;
    QVector<IsobathUtils::ColorInterval> colorIntervals_; // render
    QPointF origin_;
    QPair<int,int> lastCellPoint_;
    float levelStep_ = 3.0f;
    float tileResolution_;
    float minZ_;
    float maxZ_;
    float edgeLimit_;
    float surfaceStepSize_ = 0.0f;
    int tileSidePixelSize_;
    int tileHeightMatrixRatio_;
    int themeId_ = 0;
    int cellPx_;
    bool originSet_;

    kmath::MatrixParams lastMatParams_;
};
