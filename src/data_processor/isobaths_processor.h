#pragma once

#include <QVector>
#include <QVector3D>

#include "dataset.h"
#include "delaunay.h"
#include "isobaths_defs.h"


using namespace IsobathUtils;

class BottomTrack;
class DataProcessor;
class IsobathsProcessor
{
public:
    explicit IsobathsProcessor(DataProcessor* parent);
    ~IsobathsProcessor();

    void setDatasetPtr(Dataset* datasetPtr);
    void setBottomTrackPtr(BottomTrack* bottomTrackPtr);

signals:

public slots:
    void clear();

private:
    friend class DataProcessor;

    void onUpdatedBottomTrackData(const QVector<int>& indxs);
    void rebuildColorIntervals();
    QVector<QVector3D> generateExpandedPalette(int totalColors) const;
    void fullRebuildLinesLabels();
    void incrementalProcessLinesLabels(const QSet<int>& updsTrIndx);
    QVector<QVector3D> buildGridTriangles(const QVector<QVector3D>& pts, int gridWidth, int gridHeight) const;
    void buildPolylines(const IsobathsSegVec& segs, IsobathsPolylines& polylines) const;
    void edgeIntersection(const QVector3D& vertA, const QVector3D& vertB, float level, QVector<QVector3D>& out) const;
    void filterNearbyLabels(const QVector<LabelParameters>& inputData, QVector<LabelParameters>& outputData) const;
    void filterLinesBehindLabels(const QVector<LabelParameters>& filteredLabels, const QVector<QVector3D>& inputData, QVector<QVector3D>& outputData) const;
    void rebuildTrianglesBuffers();
    void updateTexture() const;

private:
    DataProcessor* dataProcessor_;
    Dataset* datasetPtr_ = nullptr;

    // external buffs
    QVector<QVector3D> pts_; // для треугольников
    QVector<QVector3D> edgePts_; // для ребер
    float minZ_;
    float maxZ_;
    QVector<ColorInterval> colorIntervals_;
    float levelStep_;
    QVector<QVector3D> lineSegments_;
    QVector<LabelParameters> labels_;
    float lineStepSize_; // from parent

    // internal buffs
    delaunay::Delaunay del_;
    BottomTrack* bottomTrackPtr_ = nullptr;
    QHash<int, uint64_t> bTrToTrIndxs_;
    bool originSet_ = false;
    QHash<QPair<int,int>, QVector3D>  cellPoints_; // fir - virt indx, sec - indx in tr
    QHash<QPair<int,int>, int>  cellPointsInTri_;
    QPair<int,int> lastCellPoint_;
    int cellPx_;
    QPointF origin_;
    float surfaceStepSize_;
    float labelStepSize_;
    int themeId_;
    float edgeLimit_;
    QHash<uint64_t, QVector<int>> pointToTris_;
    IsoState isoState_;
    QReadWriteLock lock_;
};
