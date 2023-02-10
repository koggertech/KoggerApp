#ifndef Q3DSCENEMODEL_H
#define Q3DSCENEMODEL_H

#include <QObject>
#include <QSet>
#include <QVector>
#include <QVector3D>
#include <QMutex>

#include <memory>

#include "Domain/DelaunayTriangulation.h"

using Vector3            = QVector <QVector3D>;
using Vector3Pointer     = std::shared_ptr <Vector3>;

static const QString OBJECT_TYPE_BOTTOM_TRACK = "Track";
static const QString OBJECT_TYPE_SURFACE_POLY = "Surface";
static const QString OBJECT_TYPE_SURFACE_MESH = "Mesh";

class Q3DSceneModel : public QObject
{
    Q_OBJECT

public:

    //! Конструктор
    explicit Q3DSceneModel(QObject *parent = nullptr);

    //! Передать указатель на данные трека
    void setBottomTrack(const Vector3Pointer pBottomTrack);
    //! Изменить признак видимости 3D - сцены
    void changeSceneVisibility(const bool visible);
    //! Изменить тип отображаемого объекта (GPS - трек, поверхность, меш)
    void changeDisplayedObjectType(const QString& type);
    //! Обновить поверхность
    void updateSurface();

public slots:
    //! Return - признак видимости 3D - сцены
    bool sceneVisibility();
    //! Return - признак доступности триангулятора
    bool triangulationAvailable();
    //! Return - тип отображаемого объекта
    QString displayedObjectType();
    //! Return - точки триангулированной поверхности
    const Vector3Pointer triangles();
    //! Return - трек морского дна
    const Vector3Pointer bottomTrack();

private:

    //! Признак видимости 3D - сцены
    bool mSceneVisible;
    //! Признак доступности триангулятора
    std::atomic_bool mTriangulationAvailable;
    //! Тип отображаемого объекта
    QString mDisplayedObjectType;
    //! Указатель на данные GPS
    Vector3Pointer mpBottomTrack;
    //! Указатель на контейнер вершин треугольников
    Vector3Pointer mpTriangles;
    //! Объект триангуляции Делоне
    Delaunay <double> mTriangulator;
    //! Мьютекс для синхронизации доступа к данным трека из разных потоков
    QMutex mBottomTrackMutex;
    //! Мьютекс для синхронизации доступа к данным триангулированной поверхности
    QMutex mTrianglesMutex;

signals:

    //! Сигнал - оповещение об изменении состояния модели 3D - сцены
    void stateChanged();
};

//! Хешер для типа данных точки (необходим для устранения повторяющихся
//! точек с помощью QSet при триангуляции)
template <typename T>
inline uint qHash(const Point3D <T> &p, uint seed = 0)
{
    uint h1 = qHash(p.x());
    uint h2 = qHash(p.y());
    uint h3 = qHash(p.z());
    return h1 ^ (h2 << 1) ^ (h3 << 2);
}

#endif // Q3DSCENEMODEL_H
