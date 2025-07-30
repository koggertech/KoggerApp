#pragma once

#include <stdint.h>
#include <QHash>
#include <QPair>
#include <QPointF>
#include <QReadWriteLock>
#include <QVector>
#include <QVector3D>
#include "delaunay.h"


class BottomTrack;
class DataProcessor;
class SurfaceMesh;
class SurfaceProcessor
{
public:
    explicit SurfaceProcessor(DataProcessor* parent);
    ~SurfaceProcessor();

    void clear();

    void setBottomTrackPtr(BottomTrack* bottomTrackPtr);
    void setSurfaceMeshPtr(SurfaceMesh* surfaceMeshPtr);
    void onUpdatedBottomTrackData(const QVector<int>& indxs);

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
    QPointF origin_;
    QPair<int,int> lastCellPoint_;
    float minZ_;
    float maxZ_;
    float edgeLimit_;
    int cellPx_;
    bool originSet_;
};
