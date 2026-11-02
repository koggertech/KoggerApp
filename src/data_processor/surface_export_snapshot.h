#pragma once

#include <array>
#include <memory>
#include <vector>
#include <QMetaType>
#include <QVector>
#include <QVector3D>


struct SurfaceExportSnapshot
{
    std::vector<QVector3D> points;
    std::vector<std::array<int, 3>> triangles;
    QVector<QVector3D> track;
    float edgeLimit = 0.0f;
    int extraWidth = 0;
    quint64 revision = 0;
    bool unchanged = false;
};

using SurfaceExportSnapshotPtr = std::shared_ptr<SurfaceExportSnapshot>;
Q_DECLARE_METATYPE(SurfaceExportSnapshotPtr)
