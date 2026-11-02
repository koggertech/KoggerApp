#include "surface_grid_export.h"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <limits>
#include <string>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include "data_processor_defs.h"
#include "surface_raster_rules.h"
#include "surface_tile.h"


namespace {

constexpr int kStepPix = defaultTileSidePixelSize / defaultTileHeightMatrixRatio;
constexpr int kBlockNodes = 1024;
constexpr int kCancelCheckEvery = 4096;
constexpr size_t kWriteChunkBytes = size_t(1) << 20;
constexpr int kLatLonDecimals = 8;
constexpr int kMetricDecimals = 3;

struct NodeRange
{
    int i0 = 0;
    int i1 = -1;
    int j0 = 0;
    int j1 = -1;

    bool isEmpty() const { return i1 < i0 || j1 < j0; }
};

struct BlockRef
{
    int by = 0;
    int bx = 0;
    int idx = 0;

    bool operator<(const BlockRef& o) const {
        if (by != o.by) {
            return by < o.by;
        }
        if (bx != o.bx) {
            return bx < o.bx;
        }
        return idx < o.idx;
    }
};

bool sameBlock(const BlockRef& a, const BlockRef& b)
{
    return a.by == b.by && a.bx == b.bx;
}

bool blockLess(const BlockRef& a, const BlockRef& b)
{
    return a.by != b.by ? a.by < b.by : a.bx < b.bx;
}

struct PixTriangle
{
    double au = 0.0, av = 0.0;
    double bu = 0.0, bv = 0.0;
    double cu = 0.0, cv = 0.0;
    double za = 0.0, zb = 0.0, zc = 0.0;
    double denom = 0.0;
    NodeRange nodes;
};

struct PixDisk
{
    qint64 cx = 0;
    qint64 cy = 0;
    qint64 r2 = 0;
    float z = 0.0f;
    bool useDir = false;
    QVector2D dir;
    NodeRange nodes;
};

int floorDiv(int a, int b)
{
    const int q = a / b;
    return (a % b != 0 && ((a < 0) != (b < 0))) ? q - 1 : q;
}

class Block
{
public:
    Block() :
        z_(size_t(kBlockNodes) * kBlockNodes, 0.0f),
        marks_(size_t(kBlockNodes) * kBlockNodes, HeightType::kUndefined)
    {}

    void reset(int bx, int by)
    {
        if (!touched_.isEmpty()) {
            for (int j = touched_.j0; j <= touched_.j1; ++j) {
                HeightType* row = marks_.data() + cell(touched_.i0, j);
                std::fill(row, row + (touched_.i1 - touched_.i0 + 1), HeightType::kUndefined);
            }
        }
        i0_ = bx * kBlockNodes;
        j0_ = by * kBlockNodes;
        touched_ = NodeRange();
    }

    NodeRange clip(const NodeRange& r) const
    {
        NodeRange out;
        out.i0 = std::max(r.i0, i0_);
        out.i1 = std::min(r.i1, i0_ + kBlockNodes - 1);
        out.j0 = std::max(r.j0, j0_);
        out.j1 = std::min(r.j1, j0_ + kBlockNodes - 1);
        return out;
    }

    HeightType mark(int i, int j) const { return marks_[cell(i, j)]; }

    void set(int i, int j, float z, HeightType mark)
    {
        const size_t c = cell(i, j);
        z_[c] = z;
        marks_[c] = mark;

        if (touched_.isEmpty()) {
            touched_ = { i, i, j, j };
            return;
        }
        touched_.i0 = std::min(touched_.i0, i);
        touched_.i1 = std::max(touched_.i1, i);
        touched_.j0 = std::min(touched_.j0, j);
        touched_.j1 = std::max(touched_.j1, j);
    }

    template<typename Fn>
    void forEachNode(Fn&& fn) const
    {
        if (touched_.isEmpty()) {
            return;
        }
        for (int j = touched_.j0; j <= touched_.j1; ++j) {
            for (int i = touched_.i0; i <= touched_.i1; ++i) {
                const size_t c = cell(i, j);
                if (marks_[c] != HeightType::kUndefined) {
                    fn(i, j, z_[c], marks_[c]);
                }
            }
        }
    }

private:
    size_t cell(int i, int j) const
    {
        return size_t(j - j0_) * kBlockNodes + size_t(i - i0_);
    }

    std::vector<float> z_;
    std::vector<HeightType> marks_;
    NodeRange touched_;
    int i0_ = 0;
    int j0_ = 0;
};

bool prepareTriangle(const SurfaceExportSnapshot& s, const std::array<int, 3>& t, double invMpp, PixTriangle& out)
{
    const int count = static_cast<int>(s.points.size());
    for (int idx : t) {
        if (idx < 0 || idx >= count) {
            return false;
        }
    }

    const QVector3D& A = s.points[t[0]];
    const QVector3D& B = s.points[t[1]];
    const QVector3D& C = s.points[t[2]];

    if (!SurfaceRasterRules::withinEdgeLimit(A, B, C, s.edgeLimit)) {
        return false;
    }

    out.au = A.x() * invMpp;
    out.av = A.y() * invMpp;
    out.bu = B.x() * invMpp;
    out.bv = B.y() * invMpp;
    out.cu = C.x() * invMpp;
    out.cv = C.y() * invMpp;
    if (!std::isfinite(out.au) || !std::isfinite(out.av) ||
        !std::isfinite(out.bu) || !std::isfinite(out.bv) ||
        !std::isfinite(out.cu) || !std::isfinite(out.cv)) {
        return false;
    }

    out.denom = SurfaceRasterRules::twiceArea(out.au, out.av, out.bu, out.bv, out.cu, out.cv);
    if (SurfaceRasterRules::isDegenerate(out.denom)) {
        return false;
    }

    out.za = A.z();
    out.zb = B.z();
    out.zc = C.z();

    out.nodes.i0 = static_cast<int>(std::floor(std::min({ out.au, out.bu, out.cu }) / kStepPix)) - 1;
    out.nodes.i1 = static_cast<int>(std::ceil (std::max({ out.au, out.bu, out.cu }) / kStepPix)) + 1;
    out.nodes.j0 = static_cast<int>(std::floor(std::min({ out.av, out.bv, out.cv }) / kStepPix)) - 1;
    out.nodes.j1 = static_cast<int>(std::ceil (std::max({ out.av, out.bv, out.cv }) / kStepPix)) + 1;
    return true;
}

bool prepareDisk(const SurfaceExportSnapshot& s, int k, double invMpp, PixDisk& out)
{
    const QVector3D& P = s.track[k];
    if (!std::isfinite(P.x()) || !std::isfinite(P.y()) || !std::isfinite(P.z())) {
        return false;
    }

    const double pu = P.x() * invMpp;
    const double pv = P.y() * invMpp;
    const double radiusM = 0.5 * static_cast<double>(s.extraWidth);
    const int radiusPx = SurfaceRasterRules::extrapolationRadiusPx(radiusM * invMpp, kStepPix);

    auto roundToGrid = [](double v) -> int {
        return static_cast<int>(std::round(v / kStepPix)) * kStepPix;
    };

    out.nodes.i0 = roundToGrid(pu - radiusPx) / kStepPix;
    out.nodes.i1 = roundToGrid(pu + radiusPx) / kStepPix;
    out.nodes.j0 = roundToGrid(pv - radiusPx) / kStepPix;
    out.nodes.j1 = roundToGrid(pv + radiusPx) / kStepPix;
    out.cx = std::llround(pu);
    out.cy = std::llround(pv);
    out.r2 = qint64(radiusPx) * radiusPx;
    out.z = P.z();
    out.useDir = SurfaceRasterRules::trackDirection(s.track, k, static_cast<float>(invMpp), out.dir);
    return true;
}

void rasterTriangle(Block& block, const PixTriangle& t)
{
    const NodeRange r = block.clip(t.nodes);

    for (int j = r.j0; j <= r.j1; ++j) {
        const double pv = double(j) * kStepPix;
        for (int i = r.i0; i <= r.i1; ++i) {
            const double pu = double(i) * kStepPix;
            double z = 0.0;
            if (SurfaceRasterRules::interpolateInTriangle(t.au, t.av, t.bu, t.bv, t.cu, t.cv, t.denom,
                                                          t.za, t.zb, t.zc, pu, pv, z)) {
                block.set(i, j, static_cast<float>(z), HeightType::kTriangulation);
            }
        }
    }
}

void paintDisk(Block& block, const PixDisk& d)
{
    const NodeRange r = block.clip(d.nodes);

    for (int j = r.j0; j <= r.j1; ++j) {
        const qint64 dy = qint64(j) * kStepPix - d.cy;
        for (int i = r.i0; i <= r.i1; ++i) {
            const qint64 dx = qint64(i) * kStepPix - d.cx;
            if (SurfaceRasterRules::diskNodeWritable(dx, dy, d.r2, d.useDir, d.dir.x(), d.dir.y(), block.mark(i, j))) {
                block.set(i, j, d.z, HeightType::kExrtapolation);
            }
        }
    }
}

void appendRefs(std::vector<BlockRef>& refs, const NodeRange& nodes, int idx)
{
    const int bx0 = floorDiv(nodes.i0, kBlockNodes);
    const int bx1 = floorDiv(nodes.i1, kBlockNodes);
    const int by0 = floorDiv(nodes.j0, kBlockNodes);
    const int by1 = floorDiv(nodes.j1, kBlockNodes);
    for (int by = by0; by <= by1; ++by) {
        for (int bx = bx0; bx <= bx1; ++bx) {
            refs.push_back({ by, bx, idx });
        }
    }
}

int countBlocks(const std::vector<BlockRef>& a, const std::vector<BlockRef>& b)
{
    int blocks = 0;
    size_t ia = 0;
    size_t ib = 0;
    const BlockRef* last = nullptr;
    while (ia < a.size() || ib < b.size()) {
        const BlockRef* next = nullptr;
        if (ib >= b.size() || (ia < a.size() && !blockLess(b[ib], a[ia]))) {
            next = &a[ia++];
        }
        else {
            next = &b[ib++];
        }
        if (!last || !sameBlock(*last, *next)) {
            ++blocks;
        }
        last = next;
    }
    return blocks;
}

bool hasInput(const SurfaceExportSnapshot& s, bool extrapolation)
{
    return !s.triangles.empty() || (extrapolation && s.extraWidth > 0 && !s.track.isEmpty());
}

template<typename Canceled, typename Sink>
bool rasterize(QPromise<SurfaceExportResult>& promise, const SurfaceExportSnapshot& s, int zoom, bool extrapolation,
               Canceled&& canceled, Sink&& sink)
{
    const double invMpp = 1.0 / static_cast<double>(mppFromZoom(zoom));

    std::vector<BlockRef> triRefs;
    triRefs.reserve(s.triangles.size());
    PixTriangle tri;
    for (int t = 0; t < static_cast<int>(s.triangles.size()); ++t) {
        if (t % kCancelCheckEvery == 0 && canceled()) {
            return false;
        }
        if (prepareTriangle(s, s.triangles[t], invMpp, tri)) {
            appendRefs(triRefs, tri.nodes, t);
        }
    }

    std::vector<BlockRef> diskRefs;
    if (extrapolation && s.extraWidth > 0) {
        diskRefs.reserve(s.track.size());
        PixDisk disk;
        for (int k = 0; k < s.track.size(); ++k) {
            if (k % kCancelCheckEvery == 0 && canceled()) {
                return false;
            }
            if (prepareDisk(s, k, invMpp, disk)) {
                appendRefs(diskRefs, disk.nodes, k);
            }
        }
    }

    std::sort(triRefs.begin(), triRefs.end());
    std::sort(diskRefs.begin(), diskRefs.end());

    const int blocks = countBlocks(triRefs, diskRefs);
    promise.setProgressRange(0, std::max(1, blocks));
    promise.setProgressValue(0);

    Block block;
    PixDisk disk;
    size_t ti = 0;
    size_t di = 0;
    int done = 0;
    while (ti < triRefs.size() || di < diskRefs.size()) {
        if (canceled()) {
            return false;
        }

        BlockRef current;
        if (di >= diskRefs.size() || (ti < triRefs.size() && !blockLess(diskRefs[di], triRefs[ti]))) {
            current = triRefs[ti];
        }
        else {
            current = diskRefs[di];
        }
        block.reset(current.bx, current.by);

        for (int n = 0; ti < triRefs.size() && sameBlock(triRefs[ti], current); ++ti, ++n) {
            if (n % kCancelCheckEvery == 0 && canceled()) {
                return false;
            }
            if (prepareTriangle(s, s.triangles[triRefs[ti].idx], invMpp, tri)) {
                rasterTriangle(block, tri);
            }
        }

        for (int n = 0; di < diskRefs.size() && sameBlock(diskRefs[di], current); ++di, ++n) {
            if (n % kCancelCheckEvery == 0 && canceled()) {
                return false;
            }
            if (prepareDisk(s, diskRefs[di].idx, invMpp, disk)) {
                paintDisk(block, disk);
            }
        }

        if (!sink(block)) {
            return false;
        }
        promise.setProgressValue(++done);
    }

    return true;
}

char* appendFixed(char* p, char* end, double v, int decimals)
{
    const auto res = std::to_chars(p, end, v, std::chars_format::fixed, decimals);
    return res.ec == std::errc() ? res.ptr : p;
}

char* appendCsvRow(char* p, char* end, double lat, double lon, double z, double east, double north)
{
    p = appendFixed(p, end, lat, kLatLonDecimals);
    *p++ = ',';
    p = appendFixed(p, end, lon, kLatLonDecimals);
    *p++ = ',';
    p = appendFixed(p, end, z, kMetricDecimals);
    *p++ = ',';
    p = appendFixed(p, end, east, kMetricDecimals);
    *p++ = ',';
    p = appendFixed(p, end, north, kMetricDecimals);
    *p++ = '\n';
    return p;
}

char* appendXyzRow(char* p, char* end, double lat, double lon, double z)
{
    p = appendFixed(p, end, lon, kLatLonDecimals);
    *p++ = ' ';
    p = appendFixed(p, end, lat, kLatLonDecimals);
    *p++ = ' ';
    p = appendFixed(p, end, z, kMetricDecimals);
    *p++ = '\n';
    return p;
}

class ExportFile
{
public:
    explicit ExportFile(const QString& path)
    {
        if (path.contains(QStringLiteral("://"))) {
            plain_ = std::make_unique<QFile>(path);
            return;
        }
        QDir().mkpath(QFileInfo(path).absolutePath());
        save_ = std::make_unique<QSaveFile>(path);
        save_->setDirectWriteFallback(true);
    }

    bool open() { return save_ ? save_->open(QIODevice::WriteOnly) : plain_->open(QIODevice::WriteOnly | QIODevice::Truncate); }
    bool write(const char* data, qint64 size) { return device()->write(data, size) == size; }
    QString errorString() const { return save_ ? save_->errorString() : plain_->errorString(); }

    bool commit()
    {
        if (save_) {
            return save_->commit();
        }
        const bool flushed = plain_->flush();
        plain_->close();
        return flushed && plain_->error() == QFileDevice::NoError;
    }

    void discard()
    {
        if (save_) {
            save_->cancelWriting();
            return;
        }
        plain_->close();
        plain_->remove();
    }

private:
    QFileDevice* device() const { return save_ ? static_cast<QFileDevice*>(save_.get()) : plain_.get(); }

    std::unique_ptr<QSaveFile> save_;
    std::unique_ptr<QFile> plain_;
};

}


namespace SurfaceGridExport {

int zoomFromQuality(int quality)
{
    return std::clamp(quality, kMinQuality, kMaxQuality) + 1;
}

double nodeStepMeters(int zoom)
{
    return static_cast<double>(kStepPix) * static_cast<double>(mppFromZoom(zoom));
}

void writeFile(QPromise<SurfaceExportResult>& promise, SurfaceExportSnapshotPtr snapshot, SurfaceExportParams params)
{
    SurfaceExportResult result;
    result.nodeStepM = nodeStepMeters(params.zoom);

    const bool xyz = params.format == SurfaceExportFormat::kXyz;
    if (xyz && !params.llaRef.isInit) {
        result.status = SurfaceExportStatus::kNoGeoReference;
        promise.addResult(result);
        return;
    }

    if (!snapshot || !hasInput(*snapshot, params.extrapolation)) {
        result.status = SurfaceExportStatus::kNoData;
        promise.addResult(result);
        return;
    }

    auto canceled = [&]() {
        return promise.isCanceled() || (params.cancel && params.cancel->load());
    };

    ExportFile file(params.path);
    if (!file.open()) {
        result.status = SurfaceExportStatus::kFileError;
        result.fileError = file.errorString();
        promise.addResult(result);
        return;
    }

    std::string buffer;
    buffer.reserve(kWriteChunkBytes + 512);
    if (!xyz) {
        buffer.append("lat,lon,z,x,y\n");
    }

    bool writeOk = true;
    auto flush = [&]() {
        if (!writeOk || buffer.empty()) {
            return;
        }
        writeOk = file.write(buffer.data(), static_cast<qint64>(buffer.size()));
        buffer.clear();
    };

    const double stepM = result.nodeStepM;
    const double depthOffset = params.depthOffset;
    const LLARef& ref = params.llaRef;

    const bool finished = rasterize(promise, *snapshot, params.zoom, params.extrapolation, canceled, [&](const Block& block) {
        block.forEachNode([&](int i, int j, float z, HeightType mark) {
            if (!writeOk) {
                return;
            }

            if (mark == HeightType::kTriangulation) {
                ++result.triangulationNodes;
            }
            else {
                ++result.extrapolationNodes;
            }

            const double north = double(i) * stepM;
            const double east = double(j) * stepM;
            double lat = std::numeric_limits<double>::quiet_NaN();
            double lon = std::numeric_limits<double>::quiet_NaN();
            if (ref.isInit) {
                const NED ned(north, east, 0.0);
                const LLA lla(&ned, &ref);
                lat = lla.latitude;
                lon = lla.longitude;
            }

            const double zOut = double(z) - depthOffset;
            char row[256];
            char* const end = row + sizeof(row);
            char* const p = xyz ? appendXyzRow(row, end, lat, lon, zOut)
                                : appendCsvRow(row, end, lat, lon, zOut, east, north);
            buffer.append(row, size_t(p - row));

            if (buffer.size() >= kWriteChunkBytes) {
                flush();
            }
        });
        return writeOk;
    });

    if (finished) {
        flush();
    }

    if (!writeOk) {
        result.status = SurfaceExportStatus::kFileError;
        result.fileError = file.errorString();
        file.discard();
    }
    else if (!finished || canceled()) {
        result.status = SurfaceExportStatus::kCanceled;
        file.discard();
    }
    else if (result.triangulationNodes + result.extrapolationNodes == 0) {
        result.status = SurfaceExportStatus::kNoData;
        file.discard();
    }
    else if (!file.commit()) {
        result.status = SurfaceExportStatus::kFileError;
        result.fileError = file.errorString();
    }

    promise.addResult(result);
}

void countNodes(QPromise<SurfaceExportResult>& promise, SurfaceExportSnapshotPtr snapshot, int zoom)
{
    SurfaceExportResult result;
    result.nodeStepM = nodeStepMeters(zoom);

    if (!snapshot || !hasInput(*snapshot, true)) {
        result.status = SurfaceExportStatus::kNoData;
        promise.addResult(result);
        return;
    }

    auto canceled = [&]() { return promise.isCanceled(); };

    const bool finished = rasterize(promise, *snapshot, zoom, true, canceled, [&](const Block& block) {
        block.forEachNode([&](int, int, float, HeightType mark) {
            if (mark == HeightType::kTriangulation) {
                ++result.triangulationNodes;
            }
            else {
                ++result.extrapolationNodes;
            }
        });
        return true;
    });

    if (!finished) {
        result.status = SurfaceExportStatus::kCanceled;
    }
    else if (result.triangulationNodes + result.extrapolationNodes == 0) {
        result.status = SurfaceExportStatus::kNoData;
    }

    promise.addResult(result);
}

}
