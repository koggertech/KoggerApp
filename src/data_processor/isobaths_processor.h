#pragma once

#include <stdint.h>
#include <QHash>
#include <QPair>
#include <QReadWriteLock>
#include <QVector>
#include <QVector3D>

#include "isobaths_defs.h"

using namespace IsobathUtils;


class BottomTrack;
class SurfaceMesh;
class DataProcessor;
class IsobathsProcessor
{
public:
    explicit IsobathsProcessor(DataProcessor* parent);
    ~IsobathsProcessor();

    void clear();

    void setSurfaceMeshPtr(SurfaceMesh* surfaceMeshPtr);

    void onUpdatedBottomTrackData(const QVector<int>& indxs); // ON UPDATED MESH

    void fullRebuildLinesLabels();
    void rebuildTrianglesBuffers();

    void setLabelStepSize(float val);
    void setLineStepSize(float val);

    float getLabelStepSize() const;
    float getLineStepSize() const;


private:
    void incrementalProcessLinesLabels(const QSet<int>& updsTrIndx);
    void buildPolylines(const IsobathsSegVec& segs, IsobathsPolylines& polylines) const;
    void edgeIntersection(const QVector3D& vertA, const QVector3D& vertB, float level, QVector<QVector3D>& out) const;
    void filterNearbyLabels(const QVector<LabelParameters>& inputData, QVector<LabelParameters>& outputData) const;

private:
    DataProcessor* dataProcessor_;
    SurfaceMesh* surfaceMeshPtr_;

    IsoState isobathsState_;
    QReadWriteLock lock_;
    QHash<uint64_t, QVector<int>> pointToTris_;
    QHash<QPair<int,int>, QVector3D>  cellPoints_; // fir - virt indx, sec - indx in tr
    QHash<QPair<int,int>, int>  cellPointsInTri_;
    QHash<int, uint64_t> bTrToTrIndxs_;
    QVector<LabelParameters> labels_; // render
    QVector<QVector3D> pts_; // render
    QVector<QVector3D> edgePts_; // render
    QVector<QVector3D> lineSegments_; // render
    QPointF origin_;
    QPair<int,int> lastCellPoint_;
    float minZ_; // render
    float maxZ_; // render
    float lineStepSize_; // render
    float labelStepSize_;
    int themeId_;
};
