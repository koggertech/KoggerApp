#pragma once

#include <atomic>
#include <memory>
#include <QPromise>
#include <QString>
#include "dataset_defs.h"
#include "surface_export_snapshot.h"


enum class SurfaceExportFormat {
    kCsv,
    kXyz
};

struct SurfaceExportParams
{
    QString path;
    LLARef llaRef;
    int zoom = 1;
    float depthOffset = 0.0f;
    bool extrapolation = true;
    SurfaceExportFormat format = SurfaceExportFormat::kCsv;
    std::shared_ptr<std::atomic_bool> cancel;
};

enum class SurfaceExportStatus {
    kOk,
    kCanceled,
    kNoData,
    kNoGeoReference,
    kFileError
};

struct SurfaceExportResult
{
    SurfaceExportStatus status = SurfaceExportStatus::kOk;
    qint64 triangulationNodes = 0;
    qint64 extrapolationNodes = 0;
    double nodeStepM = 0.0;
    QString fileError;
};

namespace SurfaceGridExport {

constexpr int kMinQuality = 0;
constexpr int kMaxQuality = 6;
constexpr int kEstimateZoom = 4;

int zoomFromQuality(int quality);
double nodeStepMeters(int zoom);

void writeFile(QPromise<SurfaceExportResult>& promise, SurfaceExportSnapshotPtr snapshot, SurfaceExportParams params);
void countNodes(QPromise<SurfaceExportResult>& promise, SurfaceExportSnapshotPtr snapshot, int zoom);

}
