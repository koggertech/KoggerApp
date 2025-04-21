#include "isobaths_processor.h"

#include <QThread>
#include <QDebug>
#include <limits>
#include <cmath>
#include <unordered_set>


static const QString& underlyingThreadName()
{
    static const QString name = QStringLiteral("IsobathsProcessorThread");
    return name;
}

IsobathsProcessor::IsobathsProcessor(QObject *parent) :
    QObject(parent),
    isBusy_(false)
{
    qDebug() << "   ISOBATHS_PROCESSOR: ctr in thread" << QThread::currentThreadId();
    qRegisterMetaType<IsobathsProcessor::IsobathProcessorResult>("IsobathsProcessor::IsobathProcessorResult");
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

IsobathsProcessor::IsobathProcessorResult IsobathsProcessor::result() const
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

    constexpr float eps = 1e-6f;
    if (zMax - zMin < eps) {
        isBusy_ = false;
        emit taskFinished(result_);
        stopInThread();
        return;
    }

    // Строим изобаты
    constexpr float zShift = 0.1; // смещение по Z
    const int uniqTrianglesSize = uniqTriangles.size() / 3;
    const int levelCnt = int(std::floor((zMax - zMin) / task_.step)) + 1;
    result_.data.reserve(levelCnt * uniqTrianglesSize);

    auto edgeIsect = [&](const QVector3D& vertA, const QVector3D& vertB, float level, QVector<QVector3D>& out) {
        float zA = vertA.z();
        float zB = vertB.z();

        QVector3D fVertA = { vertA.x(), vertA.y(), vertA.z() + zShift };
        QVector3D fVertB = { vertB.x(), vertB.y(), vertB.z() + zShift };


        if (std::fabs(zA - level) < eps && std::fabs(zB - level) < eps) { // обе на уровне
            out.append(fVertA);
            out.append(fVertB);
            return;
        }

        if ((zA - level) * (zB - level) > 0.f) { // нет пересечения
            return;
        }

        if (std::fabs(zA - level) < eps) { // a на уровне
            out.append(fVertA);
            return;
        }

        if (std::fabs(zB - level) < eps) { // b на уровне
            out.append(fVertB);
            return;
        }

        out.append(fVertA + ((level - zA) / (zB - zA)) * (fVertB - fVertA)); // пересечение
    };

    auto addSeg = [&](const QVector3D& vertA, const QVector3D& vertB) {
        result_.data.append(vertA);
        result_.data.append(vertB);
    };

    for (int i = 0; i < uniqTrianglesSize; ++i) {
        const QVector3D& A = uniqTriangles[i * 3];
        const QVector3D& B = uniqTriangles[i * 3 + 1];
        const QVector3D& C = uniqTriangles[i * 3 + 2];

        for (int j = 0; j < levelCnt; ++j) {

            const float L = zMin + j * task_.step;
            QVector<QVector3D> jSegments;
            jSegments.reserve(3);

            edgeIsect(A, B, L, jSegments);
            edgeIsect(B, C, L, jSegments);
            edgeIsect(C, A, L, jSegments);

            if (jSegments.size() == 2) {
                addSeg(jSegments[0], jSegments[1]);
            }
            else if (jSegments.size() == 3) {
                if (jSegments[0] != jSegments[1]) {
                    addSeg(jSegments[0], jSegments[1]);
                }
                if (jSegments[1] != jSegments[2]) {
                    addSeg(jSegments[1], jSegments[2]);
                }
            }
        }
    }

    isBusy_ = false;
    emit taskFinished(result_);
    stopInThread();
}

QVector<QVector3D> IsobathsProcessor::buildGridTriangles(const QVector<QVector3D>& pts, int gridWidth, int gridHeight)
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
