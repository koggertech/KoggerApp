#include "Q3DSceneModel.h"

Q3DSceneModel::Q3DSceneModel(QObject *parent)
    : QObject{parent}
    , mDisplayedObjectType("Track")
    , mpBottomTrack(std::make_shared <Vector3> ())
    , mpTriangles(std::make_shared <Vector3> ())
{
    mTriangulationAvailable.store(true);
}

void Q3DSceneModel::setBottomTrack(const Vector3Pointer pBottomTrack)
{
    QMutexLocker locker(&mBottomTrackMutex);

    mpBottomTrack = pBottomTrack;

    if (mDisplayedObjectType == "Track")
        emit stateChanged();
}

void Q3DSceneModel::changeSceneVisibility(const bool visible)
{
    mSceneVisible = visible;

    emit stateChanged();
}

void Q3DSceneModel::changeDisplayedObjectType(const QString& type)
{
    mDisplayedObjectType = type;

    emit stateChanged();
}

void Q3DSceneModel::updateSurface()
{
    // Говорим, что процедура триангуляции недоступна
    mTriangulationAvailable.store(false);

    // Обеспечиваем исключительный доступ к данным трека
    QMutexLocker bottomTrackLocker(&mBottomTrackMutex);

    if (!mpBottomTrack){
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

    // Работа с данными трека окончена, отпускаем примитив синхронизации
    bottomTrackLocker.unlock();

    // Формируем вектор точек для триангуляции
    std::vector <Point3D <double>> points;

    for (const auto& point : set){
        points.push_back(point);
    }

    // Выполняем триангуляцию
    auto pTriangles = mTriangulator.trinagulate(points);

    // Обеспечиваем исключительный доступ к данным триангулированной поверхности
    QMutexLocker trianglesLocker(&mTrianglesMutex);

    // Конвертируем в формат для отображения на сцене
    mpTriangles->clear();

    for (const auto& t : *pTriangles){

        QVector3D A(t.A().x(), t.A().y(), t.A().z());
        QVector3D B(t.B().x(), t.B().y(), t.B().z());
        QVector3D C(t.C().x(), t.C().y(), t.C().z());

        mpTriangles->append(A);
        mpTriangles->append(B);
        mpTriangles->append(C);
    }

    emit stateChanged();

    // Делаем процедуру триангуляции вновь доступной
    mTriangulationAvailable.store(true);
}


bool Q3DSceneModel::sceneVisibility()
{
    return mSceneVisible;
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

const Vector3Pointer Q3DSceneModel::bottomTrack()
{
    QMutexLocker bottomTrackLocker(&mBottomTrackMutex);

    return mpBottomTrack;
}
