#include "isobaths_processor.h"

#include <QThread>
#include <QDebug>
#include <QVector2D>
#include <limits>
#include <cmath>
#include <unordered_set>


static const float epsilon_ = 1e-6f;

static const QString& underlyingThreadName()
{
    static const QString name = QStringLiteral("IsobathsProcessorThread");
    return name;
}

IsobathsProcessor::IsobathsProcessor(QObject *parent) :
    QObject(parent),
    isBusy_(false)
{
    qRegisterMetaType<IsobathsProcessorResult>("IsobathsProcessorResult");
}

IsobathsProcessor::~IsobathsProcessor()
{
    stopInThread();
}

bool IsobathsProcessor::setTask(const IsobathsProcessorTask& task)
{
    if (isBusy_.load()) {
        return false;
    }

    task_ = task;

    return true;
}

bool IsobathsProcessor::startInThread()
{
    if (isBusy_.load()) {
        qDebug() << "IsobathsProcessor::startInThread: isBusy_.load() failed";
        return false;
    }

    if (parent()) {
        qDebug() << "IsobathsProcessor::startInThread: parent() failed";

        return false;
    }

    auto currentThread = thread();

    if ((currentThread && currentThread->objectName() != underlyingThreadName()) || !currentThread) {
        currentThread = new QThread(this);
        currentThread->setObjectName(underlyingThreadName());
        QObject::connect(currentThread, &QThread::started, this, &IsobathsProcessor::process);
        moveToThread(currentThread);
    }

    currentThread->start();
    return true;
}

bool IsobathsProcessor::startInThread(const IsobathsProcessorTask &task)
{
    if (!setTask(task)) {
        return false;
    }

    return startInThread();
}

bool IsobathsProcessor::stopInThread(unsigned long time)
{
    auto* currentThread = thread();

    if (parent() || !(currentThread && currentThread->objectName() == underlyingThreadName())) {
        return true;
    }

    if (QThread::currentThread() == currentThread) {
        currentThread->quit();
        return true;
    }

    currentThread->quit();

    return currentThread->wait(time);
}

IsobathsProcessorTask IsobathsProcessor::task() const
{
    return task_;
}

const IsobathsProcessorTask &IsobathsProcessor::ctask() const
{
    return task_;
}

bool IsobathsProcessor::isBusy() const
{
    return isBusy_.load();
}

IsobathsProcessorResult IsobathsProcessor::result() const
{
    return result_;
}

void IsobathsProcessor::process()
{
    if (isBusy_.exchange(true)) {
        return;
    }

    emit taskStarted();
    result_.data.clear();

    const auto& data = task_.grid;
    const int dataSize = data.size();

    if (dataSize < 3 || task_.step <= 0.f) {
        isBusy_ = false;
        emit taskFinished(result_);
        stopInThread();
        return;
    }

    // Размеры сетки
    if (task_.gridWidth <= 1 || task_.gridHeight <= 1) {
        constexpr float scaleCoef = 1e4f;
        std::unordered_set<int> xSet, ySet;
        xSet.reserve(dataSize);
        ySet.reserve(dataSize);

        for (const auto& itm : data) {
            xSet.insert(static_cast<int>(std::round(itm.x() * scaleCoef)));
            ySet.insert(static_cast<int>(std::round(itm.y() * scaleCoef)));
        }

        task_.gridWidth  = xSet.size();
        task_.gridHeight = ySet.size();
    }

    // Формирование треугольников
    QVector<QVector3D> triangles;
    if (task_.gridWidth > 1 && task_.gridHeight > 1) {
        triangles = buildGridTriangles(data, task_.gridWidth, task_.gridHeight);
    }
    if (triangles.isEmpty()) {
        const int triCnt = dataSize / 3;
        triangles.reserve(triCnt * 3);
        for (int i = 0; i < triCnt; ++i) {
            triangles.append(data[i * 3 + 0]);
            triangles.append(data[i * 3 + 1]);
            triangles.append(data[i * 3 + 2]);
        }
    }

    if (triangles.isEmpty()) {
        isBusy_ = false;
        emit taskFinished(result_);
        stopInThread();
        return;
    }

    // Убираем дубликаты треугольников
    std::unordered_set<uint64_t> uniqHash;
    uniqHash.reserve(triangles.size() / 3);
    QVector<QVector3D> uniqTriangles;
    uniqTriangles.reserve(triangles.size());

    auto vhash = [](const QVector3D& v) -> uint64_t {
        return (uint64_t(qRound64(v.x() * 1e5)) << 21) ^
               (uint64_t(qRound64(v.y() * 1e5)) << 10) ^
                uint64_t(qRound64(v.z() * 1e4));
    };

    for (int i = 0; i < triangles.size(); i += 3) {
        uint64_t h = vhash(triangles[i]) ^ vhash(triangles[i+1]) ^ vhash(triangles[i+2]);

        if (uniqHash.insert(h).second) {
            uniqTriangles.append(triangles[i]);
            uniqTriangles.append(triangles[i + 1]);
            uniqTriangles.append(triangles[i + 2]);
        }
    }

    // Вычисляем диапазон глубин
    float zMin =  std::numeric_limits<float>::max();
    float zMax = -zMin;

    for (const auto& v : uniqTriangles) {
        zMin = std::min(zMin, v.z());
        zMax = std::max(zMax, v.z());
    }

    if (zMax - zMin < epsilon_) {
        isBusy_ = false;
        emit taskFinished(result_);
        stopInThread();
        return;
    }

    // Строим изобаты, лейблы
    const int uniqTrianglesSize = uniqTriangles.size() / 3;
    const int levelCnt = int(std::floor((zMax - zMin) / task_.step)) + 1;
    result_.data.reserve(levelCnt * uniqTrianglesSize);
    result_.labels.clear();
    result_.labels.reserve(levelCnt);

    QHash<int, IsobathsSegVec> hashSegsByLvl;
    auto addSeg = [&](int lvl,const QVector3D& a,const QVector3D& b) -> void {
        hashSegsByLvl[lvl].append(qMakePair(a, b));
    };

    // постройка сегментов
    for (int i = 0; i < uniqTrianglesSize; ++i) {
        const QVector3D& A = uniqTriangles[i * 3];
        const QVector3D& B = uniqTriangles[i * 3 + 1];
        const QVector3D& C = uniqTriangles[i * 3 + 2];

        for (int j = 0; j < levelCnt; ++j) {
            const float L = zMin + j * task_.step;

            QVector<QVector3D> jSegments;
            jSegments.reserve(3);

            edgeIntersection(A, B, L, jSegments);
            edgeIntersection(B, C, L, jSegments);
            edgeIntersection(C, A, L, jSegments);

            if (jSegments.size() == 2) {
                addSeg(j, jSegments[0], jSegments[1]);
            }
            else if (jSegments.size() == 3) {
                if (jSegments[0] != jSegments[1]) {
                    addSeg(j, jSegments[0], jSegments[1]);
                }
                if (jSegments[1] != jSegments[2]) {
                    addSeg(j, jSegments[1], jSegments[2]);
                }
            }
        }
    }

    QVector<QVector3D> resLines;
    QVector<LabelInfo> resLabels;

    for (auto it = hashSegsByLvl.begin(); it != hashSegsByLvl.end(); ++it) {
        IsobathsPolylines polylines; // несколько на уровень
        buildPolylines(it.value(), polylines);

        if (polylines.isEmpty()) {
            continue;
        }

        const auto& cPolylines = polylines;
        for (const auto& polyline : cPolylines) {
            for (int i = 0; i < polyline.size() - 1; ++i) {
                resLines.append(polyline[i]);
                resLines.append(polyline[i + 1]);
            }
        }

        // Подписи по объединённой длине
        float nextMark = 0.0f;
        float cumulativeShift = 0.0f;

        for (const auto& polyline : cPolylines) {
            // Длины сегментов текущей полилинии
            QVector<float> segLen(polyline.size() - 1);
            float polylineLen = 0.0f;

            for (int i = 0; i < polyline.size() - 1; ++i) {
                segLen[i] = (polyline[i + 1] - polyline[i]).length();
                polylineLen += segLen[i];
            }

            int curSeg = 0;
            float curOff = 0.0f;

            bool placedLabel = false;

            while (nextMark < cumulativeShift + polylineLen + epsilon_) {
                // Сдвигаем указатель сегмента до нужного отрезка
                while (curSeg < segLen.size() &&
                       curOff + segLen[curSeg] < nextMark - cumulativeShift - epsilon_) {
                    curOff += segLen[curSeg];
                    ++curSeg;
                }

                if (curSeg >= segLen.size()) {
                    break;
                }

                // Положение подписи внутри сегмента
                float localMark = nextMark - cumulativeShift;
                float t = (localMark - curOff) / segLen[curSeg];

                QVector3D interpPoint = polyline[curSeg] + t * (polyline[curSeg + 1] - polyline[curSeg]);
                QVector3D direction = (polyline[curSeg + 1] - polyline[curSeg]).normalized();
                direction.setZ(0);

                // смещение от линии
                //QVector3D perp(-direction.y(), direction.x(), 0.0f);
                //perp.normalize();
                //float labelOffset = 10.0f;
                //interpPoint = interpPoint + perp * labelOffset;

                resLabels.append(LabelInfo{ interpPoint, direction, std::fabs(zMin + it.key() * task_.step) });
                nextMark += task_.labelStep;
                placedLabel = true;
            }

            // Если не было поставлено ни одной метки — ставим одну в начало
            if (!placedLabel && polyline.size() >= 2) {
                QVector3D point = polyline.first();
                QVector3D direction = (polyline[1] - polyline[0]).normalized();
                direction.setZ(0);

                resLabels.append(LabelInfo{ point, direction, std::fabs(zMin + it.key() * task_.step) });
            }

            cumulativeShift += polylineLen;
        }
    }

    // filter labels
    QVector<LabelInfo> filteredLabels;
    filteredLabels.reserve(resLabels.size());
    filterNearbyLabels(resLabels, filteredLabels);
    result_.labels = std::move(filteredLabels);

    // filter lines
    //QVector<QVector3D> filteredLines;
    //filteredLines.reserve(resLines.size());
    //filterLinesBehindLabels(result_.labels, resLines, filteredLines);
    //result_.data = std::move(filteredLines);
    result_.data = std::move(resLines);

    isBusy_ = false;
    emit taskFinished(result_);
    stopInThread();
}

QVector<QVector3D> IsobathsProcessor::buildGridTriangles(const QVector<QVector3D>& pts, int gridWidth, int gridHeight) const
{
    if (pts.size() < 3 || gridWidth <= 1 || gridHeight <= 1) {
        return {};
    }

    // Карта
    QVector<int> grid(gridWidth * gridHeight, -1);

    float minX = std::numeric_limits<float>::max();
    float minY = std::numeric_limits<float>::max();
    float maxX = -std::numeric_limits<float>::max();
    float maxY = -std::numeric_limits<float>::max();

    for (const auto& v : pts) {
        minX = std::min(minX, v.x());
        minY = std::min(minY, v.y());
        maxX = std::max(maxX, v.x());
        maxY = std::max(maxY, v.y());
    }

    const float stepX = (maxX - minX) / (gridWidth - 1);
    const float stepY = (maxY - minY) / (gridHeight - 1);

    const float invStepX = 1.f / stepX;
    const float invStepY = 1.f / stepY;

    for (int k = 0; k < pts.size(); ++k) {
        int ix = int(std::round((pts[k].x() - minX) * invStepX));
        int iy = int(std::round((pts[k].y() - minY) * invStepY));
        if (ix >= 0 && ix < gridWidth && iy >= 0 && iy < gridHeight) {
            grid[ix + iy * gridWidth] = k;
        }
    }

    // Генерация треугольников по клеткам сетки
    QVector<QVector3D> triangles;
    triangles.reserve((gridWidth - 1) * (gridHeight - 1) * 6);

    auto id = [&](int x, int y) -> int {
        return grid[x + y * gridWidth];
    };

    for (int j = 0; j < gridHeight - 1; ++j) {
        for (int i = 0; i < gridWidth - 1; ++i) {
            int i00 = id(i, j);
            int i10 = id(i + 1, j);
            int i01 = id(i, j + 1);
            int i11 = id(i + 1, j + 1);
            if (i00 < 0 || i10 < 0 || i01 < 0 || i11 < 0) {
                continue;
            }
            triangles.append(pts[i00]);
            triangles.append(pts[i10]);
            triangles.append(pts[i11]);
            triangles.append(pts[i00]);
            triangles.append(pts[i11]);
            triangles.append(pts[i01]);
        }
    }

    return triangles;
}

void IsobathsProcessor::buildPolylines(const IsobathsSegVec &segs, IsobathsPolylines &polylines) const
{
    auto isEqual = [&](const QVector3D& a,const QVector3D& b) -> bool {
        return (a - b).lengthSquared() < std::pow(epsilon_, 2);
    };

    QVector<bool> usedSegs(segs.size(), false);
    for (int s = 0; s < segs.size(); ++s) {
        if (usedSegs[s]) {
            continue;
        }

        QList<QVector3D> polyline;
        polyline.append(segs[s].first);
        polyline.append(segs[s].second);
        usedSegs[s] = true;

        bool extended = true;
        while (extended) {
            extended = false;
            for (int t = 0; t < segs.size(); ++t) {
                if (!usedSegs[t]) {
                    if (isEqual(polyline.last(), segs[t].first )) {
                        polyline.append(segs[t].second);
                        usedSegs[t] = true;
                        extended = true;
                    }
                    else if (isEqual(polyline.last(), segs[t].second)) {
                        polyline.append(segs[t].first);
                        usedSegs[t] = true;
                        extended = true;
                    }
                    else if (isEqual(polyline.first(), segs[t].second)) {
                        polyline.prepend(segs[t].first);
                        usedSegs[t] = true;
                        extended = true;
                    }
                    else if (isEqual(polyline.first(), segs[t].first)) {
                        polyline.prepend(segs[t].second);
                        usedSegs[t] = true;
                        extended = true;
                    }
                }
            }
        }

        if (polyline.size() > 1) {
            polylines.append(QVector<QVector3D>(polyline.begin(), polyline.end()));
        }
    }
}

void IsobathsProcessor::edgeIntersection(const QVector3D &vertA, const QVector3D &vertB, float level, QVector<QVector3D> &out) const
{
    const float zShift = 0.03; // смещение по Z

    float zA = vertA.z();
    float zB = vertB.z();

    QVector3D fVertA = { vertA.x(), vertA.y(), vertA.z() + zShift };
    QVector3D fVertB = { vertB.x(), vertB.y(), vertB.z() + zShift };

    if (std::fabs(zA - level) < epsilon_ && std::fabs(zB - level) < epsilon_) { // обе на уровне
        out.append(fVertA);
        out.append(fVertB);
        return;
    }
    if ((zA - level) * (zB - level) > 0.f) { // нет пересечения
        return;
    }
    if (std::fabs(zA - level) < epsilon_) { // a на уровне
        out.append(fVertA);
        return;
    }
    if (std::fabs(zB - level) < epsilon_) { // b на уровне
        out.append(fVertB);
        return;
    }

    float diff = zB - zA;
    if (!qFuzzyIsNull(diff)) {
        QVector3D intersec = (fVertA + ((level - zA) / (zB - zA)) * (fVertB - fVertA));
        if (std::isfinite(intersec.x()) && std::isfinite(intersec.y()) && std::isfinite(intersec.z())) {
            out.append(intersec);
        }
    }
}

void IsobathsProcessor::filterNearbyLabels(const QVector<LabelInfo> &inputData, QVector<LabelInfo> &outputData) const
{
    const float cellSize = 20.0f;
    const float cellSizeInv = 1.0f / cellSize;
    const float minLabelDist2 = cellSize * cellSize;

    QHash<QPair<int, int>, QVector<QVector3D>> spatialGrid;

    for (const auto& label : inputData) {
        int cellX = int(std::floor(label.pos.x() * cellSizeInv));
        int cellY = int(std::floor(label.pos.y() * cellSizeInv));

        bool tooClose = false;

        for (int dx = -1; dx <= 1; ++dx) {
            for (int dy = -1; dy <= 1; ++dy) {
                QPair<int, int> neighbor(cellX + dx, cellY + dy);

                if (spatialGrid.contains(neighbor)) {
                    const auto& list = spatialGrid[neighbor];
                    for (const auto& p : list) {
                        if ((label.pos - p).lengthSquared() < minLabelDist2) {
                            tooClose = true;
                            break;
                        }
                    }
                }

                if (tooClose) {
                    break;
                }
            }
            if (tooClose) {
                break;
            }
        }

        if (!tooClose) {
            outputData.append(label);
            spatialGrid[qMakePair(cellX, cellY)].append(label.pos);
        }
    }
}

void IsobathsProcessor::filterLinesBehindLabels(const QVector<LabelInfo> &filteredLabels, const QVector<QVector3D> &inputData, QVector<QVector3D> &outputData) const
{
    auto segmentIntersectsLabelBox = [](const QVector3D& p1, const QVector3D& p2, const LabelInfo& lbl, float width = 24.0f, float height = 5.0f) -> bool {
        QVector2D a(p1.x(), p1.y());
        QVector2D b(p2.x(), p2.y());

        float labelOffset = 10;

        QVector2D right = QVector2D(-lbl.dir.y(), -lbl.dir.x()).normalized();
        QVector2D center = QVector2D(lbl.pos.x(), lbl.pos.y()) + right * labelOffset;

        QVector2D boxMin = center - QVector2D(width * 0.5f, height * 0.5f);
        QVector2D boxMax = center + QVector2D(width * 0.5f, height * 0.5f);

        float minX = std::min(a.x(), b.x());
        float maxX = std::max(a.x(), b.x());
        float minY = std::min(a.y(), b.y());
        float maxY = std::max(a.y(), b.y());

        if (maxX < boxMin.x() || minX > boxMax.x() ||
            maxY < boxMin.y() || minY > boxMax.y()) {
            return false;
        }

        QVector2D mid = (a + b) * 0.5f;
        if (mid.x() >= boxMin.x() && mid.x() <= boxMax.x() &&
            mid.y() >= boxMin.y() && mid.y() <= boxMax.y()) {
            return true;
        }

        return false;
    };

    for (int i = 0; i + 1 < inputData.size(); i += 2) {
        const QVector3D& p1 = inputData[i];
        const QVector3D& p2 = inputData[i + 1];

        bool skip = false;
        for (const auto& lbl : filteredLabels) {
            if (segmentIntersectsLabelBox(p1, p2, lbl)) {
                skip = true;
                break;
            }
        }

        if (!skip) {
            outputData.append(p1);
            outputData.append(p2);
        }
    }
}
