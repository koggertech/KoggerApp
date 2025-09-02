#pragma once

#include <QVector>
#include <QVector3D>
#include "isobaths_defs.h"
#include "surface_mesh.h"

using namespace IsobathUtils;


class DataProcessor;
class IsobathsProcessor
{
public:
    explicit IsobathsProcessor(DataProcessor* dataProcessorPtr);
    ~IsobathsProcessor() = default;

    void clear();
    void setSurfaceMeshPtr(SurfaceMesh* surfaceMeshPtr);
    void onUpdatedBottomTrackData();

    void  setMinZ(float v);
    void  setMaxZ(float v);
    void  setLineStepSize(float v);
    void  setLabelStepSize(float v);
    float getLineStepSize()  const;
    float getLabelStepSize() const;

private:
    void fullRebuildLinesLabels();

    void buildPolylines(const IsobathsSegVec& segs, IsobathsPolylines& p) const;
    void edgeIntersection(const QVector3D& u,const QVector3D& v,float L, QVector<QVector3D>& out) const;
    void filterNearbyLabels(const QVector<LabelParameters>& in, QVector<LabelParameters>& out) const;

    bool canceled() const noexcept;

private:
    DataProcessor* dataProcessor_;
    SurfaceMesh* surfaceMeshPtr_;
    std::vector<QVector3D> vertPool_;
    std::vector<HeightType> vertMark_;
    std::vector<TrIndxs> tris_;
    QVector<QVector3D> lineSegments_;
    QVector<LabelParameters> labels_;
    float minZ_;
    float maxZ_;
    float lineStepSize_;
    float labelStepSize_;
};
