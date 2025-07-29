#pragma once

#include <stdint.h>
#include <QHash>
#include <QPair>
#include <QReadWriteLock>
#include <QVector>
#include <QVector3D>

#include "delaunay.h"
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

    void setBottomTrackPtr(BottomTrack* bottomTrackPtr);
    void setSurfaceMeshPtr(SurfaceMesh* surfaceMeshPtr);

    void onUpdatedBottomTrackData(const QVector<int>& indxs);
    void rebuildColorIntervals();
    void fullRebuildLinesLabels();
    void rebuildTrianglesBuffers();

    void setEdgeLimit(float val);;
    void setLabelStepSize(float val);
    void setLineStepSize(float val);
    void setSurfaceStepSize(float val);
    void setThemeId(int val);

    float getEdgeLimit() const;
    float getLabelStepSize() const;
    float getLineStepSize() const;
    float getSurfaceStepSize() const;
    int getThemeId() const;

private:
    void incrementalProcessLinesLabels(const QSet<int>& updsTrIndx);
    QVector<QVector3D> generateExpandedPalette(int totalColors) const;
    void buildPolylines(const IsobathsSegVec& segs, IsobathsPolylines& polylines) const;
    void edgeIntersection(const QVector3D& vertA, const QVector3D& vertB, float level, QVector<QVector3D>& out) const;
    void filterNearbyLabels(const QVector<LabelParameters>& inputData, QVector<LabelParameters>& outputData) const;
    void updateTexture() const;

private:
    DataProcessor* dataProcessor_;
    BottomTrack* bottomTrackPtr_;
    SurfaceMesh* surfaceMeshPtr_;
    delaunay::Delaunay delaunayProc_;
    IsoState isobathsState_;
    QReadWriteLock lock_;
    QHash<uint64_t, QVector<int>> pointToTris_;
    QHash<QPair<int,int>, QVector3D>  cellPoints_; // fir - virt indx, sec - indx in tr
    QHash<QPair<int,int>, int>  cellPointsInTri_;
    QHash<int, uint64_t> bTrToTrIndxs_;
    QVector<LabelParameters> labels_; // render
    QVector<ColorInterval> colorIntervals_; // render
    QVector<QVector3D> pts_; // render
    QVector<QVector3D> edgePts_; // render
    QVector<QVector3D> lineSegments_; // render
    QPointF origin_;
    QPair<int,int> lastCellPoint_;
    float minZ_; // render
    float maxZ_; // render
    float levelStep_; // render
    float lineStepSize_; // render
    float surfaceStepSize_;
    float labelStepSize_;
    float edgeLimit_;
    int cellPx_;
    int themeId_;
    bool originSet_;
};
