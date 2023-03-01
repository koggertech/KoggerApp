#include "Q3DSceneModel.h"

Q3DSceneModel::Q3DSceneModel(QObject *parent)
    : QObject{parent}
    , mInterpLevel(1)
    , mSceneVisible(false)
    , mBottomTrackVisible(false)
    , mSurfaceVisible(false)
    , mSurfaceGridVisible(false)
    , mDisplayedObjectType("Track")
    , mpBottomTrack(std::make_shared <Vector3> ())
    , mpTriangles(std::make_shared <Vector3> ())
    , mpTriangleGrid(std::make_shared <Vector3> ())
    , mpQuads(std::make_shared <Vector3> ())
    , mpGrid(std::make_shared <Vector3> ())
{
    mTriangulationAvailable.store(true);
}

void Q3DSceneModel::setBottomTrack(const Vector3Pointer pBottomTrack)
{
    QMutexLocker locker(&mBottomTrackMutex);

    mpBottomTrack = pBottomTrack;

    if (!mpBottomTrack || mpBottomTrack->isEmpty()) return;

    for (const auto& p : *mpBottomTrack){
        mMaxZ = std::max(p.z(), mMaxZ);
        mMinZ = std::min(p.z(), mMinZ);
    }

    emit stateChanged();
}

void Q3DSceneModel::changeSceneVisibility(const bool visible)
{
    mSceneVisible = visible;

    emit stateChanged();
}

void Q3DSceneModel::changeBottomTrackVisibility(const bool visible)
{
    mBottomTrackVisible = visible;

    emit stateChanged();
}

void Q3DSceneModel::changeSurfaceVisibility(const bool visible)
{
    mSurfaceVisible = visible;

    emit stateChanged();
}

void Q3DSceneModel::changeSurfaceGridVisibility(const bool visible)
{
    mSurfaceGridVisible = visible;

    emit stateChanged();
}

void Q3DSceneModel::changeDisplayedObjectType(const QString& type)
{
    mDisplayedObjectType = type;

    emit stateChanged();
}

void Q3DSceneModel::chageDisplayedStage(const QString& stage)
{
    mDisplayedStage = stage;

    emit stateChanged();
}


void Q3DSceneModel::changeMaxTriangulationLineLength(const int length)
{
    mTriEdgeLengthLimit = length;
}

void Q3DSceneModel::setInterpolationLevel(const uint8_t level)
{
    mInterpLevel = level;
}

void Q3DSceneModel::interpolate()
{
    mTriangulationAvailable.store(false);

    //QMutexLocker trianglesMutexLocker(&mTrianglesMutex);

    if (!mpRawTriangles){
        mTriangulationAvailable.store(true);
        return;
    }

    emit stateChanged();

    mInterpolator.setInterpolationLevel(mInterpLevel);
    auto pQuads = mInterpolator.interpolate(mpRawTriangles);

    //trianglesMutexLocker.unlock();

    mpGrid->clear();
    mpQuads->clear();

    for (const auto& q : *pQuads){
        QVector3D A(q.A().x(), q.A().y(), q.A().z());
        QVector3D B(q.B().x(), q.B().y(), q.B().z());
        QVector3D C(q.C().x(), q.C().y(), q.C().z());
        QVector3D D(q.D().x(), q.D().y(), q.D().z());

        mpQuads->append({A,B,C,D});

        mpGrid->append({A,B});
        mpGrid->append({B,C});
    }

    emit stateChanged();

    mTriangulationAvailable.store(true);
}

void Q3DSceneModel::updateSurface()
{
    // Говорим, что процедура триангуляции недоступна
    mTriangulationAvailable.store(false);

    QMutexLocker locker(&mBottomTrackMutex);

    if (!mpBottomTrack || mpBottomTrack->isEmpty()){
        mTriangulationAvailable.store(true);
        return;
    }

    emit stateChanged();

    // Удаляем повторяющиеся точки используя множество
    QSet <Point3D <double>> set;

    for (auto i = 0; i < mpBottomTrack->size(); i++){

        Point3D <double> point((*mpBottomTrack)[i].x()
                              ,(*mpBottomTrack)[i].y()
                              ,(*mpBottomTrack)[i].z());

        set.insert(point);
    }

    locker.unlock();

    // Формируем вектор точек для триангуляции
    std::vector <Point3D <double>> points;

    for (const auto& point : set){
        points.push_back(point);
    }

    // Выполняем триангуляцию
    mpRawTriangles = mTriangulator.trinagulate(points, mTriEdgeLengthLimit);

    mInterpolator.setInterpolationLevel(mInterpLevel);
    auto pQuads = mInterpolator.interpolate(mpRawTriangles);

    mpTriangles->clear();
    mpGrid->clear();
    mpQuads->clear();

    for (const auto& q : *mpRawTriangles){
        QVector3D A(q.A().x(), q.A().y(), q.A().z());
        QVector3D B(q.B().x(), q.B().y(), q.B().z());
        QVector3D C(q.C().x(), q.C().y(), q.C().z());

        mpTriangles->append({A,B,C});

        mpTriangleGrid->append({A,B});
        mpTriangleGrid->append({B,C});
        mpTriangleGrid->append({A,C});
    }

    for (const auto& q : *pQuads){
        QVector3D A(q.A().x(), q.A().y(), q.A().z());
        QVector3D B(q.B().x(), q.B().y(), q.B().z());
        QVector3D C(q.C().x(), q.C().y(), q.C().z());
        QVector3D D(q.D().x(), q.D().y(), q.D().z());

        mpQuads->append({A,B,C,D});

        mpGrid->append({A,B});
        mpGrid->append({B,C});
        mpGrid->append({C,D});
        mpGrid->append({A,D});

    }

    emit stateChanged();

    // Делаем процедуру триангуляции вновь доступной
    mTriangulationAvailable.store(true);
}

bool Q3DSceneModel::sceneVisibility()
{
    return mSceneVisible;
}

bool Q3DSceneModel::bottomTrackVisible()
{
    return mBottomTrackVisible;
}

bool Q3DSceneModel::surfaceVisible()
{
    return mSurfaceVisible;
}

bool Q3DSceneModel::surfaceGridVisible()
{
    return mSurfaceGridVisible;
}

bool Q3DSceneModel::triangulationAvailable()
{
    return mTriangulationAvailable.load();
}

QString Q3DSceneModel::displayedObjectType()
{
    return mDisplayedObjectType;
}

const Vector3Pointer Q3DSceneModel::triangles()
{
    QMutexLocker trianglesLocker(&mTrianglesMutex);

    return mpTriangles;
}

const Vector3Pointer Q3DSceneModel::triangleGrid()
{
    return mpTriangleGrid;
}

const Vector3Pointer Q3DSceneModel::quads()
{
    QMutexLocker quadsLocker(&mQuadsMutex);

    return mpQuads;
}

const Vector3Pointer Q3DSceneModel::grid()
{
    QMutexLocker gridLocker(&mGridMutex);

    return mpGrid;
}

const Vector3Pointer Q3DSceneModel::bottomTrack()
{
    QMutexLocker bottomTrackLocker(&mBottomTrackMutex);

    return mpBottomTrack;
}

double Q3DSceneModel::objectMaximumZ() const
{
    return mInterpolator.maximumZ();
}

double Q3DSceneModel::objectMinimumZ() const
{
    return mInterpolator.minimumZ();
}

int Q3DSceneModel::maxTriEdgeLength()
{
    return mTriangulator.maxEdgeLength();
}

int Q3DSceneModel::minTriEdgeLength()
{
    return mTriangulator.minEdgeLength();
}

QString Q3DSceneModel::displayedStage()
{
    return mDisplayedStage;
}
