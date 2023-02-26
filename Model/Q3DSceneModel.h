#ifndef Q3DSCENEMODEL_H
#define Q3DSCENEMODEL_H

#include <QObject>
#include <QSet>
#include <QVector>
#include <QVector3D>
#include <QMutex>
#include <QFile>
#include <QDataStream>

#include <memory>
#include <algorithm>

#include "Domain/DelaunayTriangulation.h"
#include "Domain/Interpolator.h"

using Vector3             = QVector <QVector3D>;
using Vector3Pointer      = std::shared_ptr <Vector3>;
using RawTrianglesPointer = std::shared_ptr <std::vector <Triangle <double>>>;

static const QString OBJECT_BOTTOM_TRACK      = "Track";
static const QString OBJECT_SURFACE_POLY_GRID = "Surface with grid";
static const QString OBJECT_SURFACE_POLY      = "Surface";
static const QString OBJECT_SURFACE_GRID      = "Grid";

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
    //! Включить/отключить сглаживание 3D - объекта
    void setDisplayedObjectSmoothingEnabled(const bool enabled);
    //! Установить уровень интерполяции поверхности
    void setInterpolationLevel(const uint8_t level);
    //! Обновить поверхность
    void updateSurface();
    //! Провести интерполяцию поверхности
    void interpolate();

    double objectMaximumZ() const;
    double objectMinimumZ() const;

public slots:
    //! Return - признак видимости 3D - сцены
    bool sceneVisibility();
    //! Return - признак доступности триангулятора
    bool triangulationAvailable();
    //! Return - тип отображаемого объекта
    QString displayedObjectType();
    //! Return - точки триангулированной поверхности
    const Vector3Pointer triangles();
    //! Return - точки интерполированой поверхности (квадраты)
    const Vector3Pointer quads();
    //! Return - точки интерполированой поверхности (линии)
    const Vector3Pointer grid();

    //! Return - трек морского дна
    const Vector3Pointer bottomTrack();

private:

    void findHeightDimensions();

    //! Уровень интерполяции
    uint8_t mInterpLevel;
    //! Признак видимости 3D - сцены
    bool mSceneVisible;
    //! Признак доступности триангулятора
    std::atomic_bool mTriangulationAvailable;
    //! Тип отображаемого объекта
    QString mDisplayedObjectType;

    //! Указатель на вектор "сырых" треугольников, предназначенных для дальнейшей интерполяции
    RawTrianglesPointer mpRawTriangles;

    //! Указатель на данные GPS
    Vector3Pointer mpBottomTrack;
    //! Указатель на контейнер вершин треугольников, готовых для отображения
    Vector3Pointer mpTriangles;
    //! Указатель на контейнер вершин квадратов, готовых для отображения
    Vector3Pointer mpQuads;
    //! Указатель на контейнер вершин линий квадратной сетки, готовых для отображения
    Vector3Pointer mpGrid;

    std::shared_ptr <std::vector <Triangle <double>>> mpTri;


    //! Объект триангуляции Делоне
    Delaunay <double> mTriangulator;
    //! Объект интерполяции
    Interpolator <double> mInterpolator;
    //! Мьютекс для синхронизации доступа к данным трека из разных потоков
    QMutex mBottomTrackMutex;
    //! Мьютекс для синхронизации доступа к данным триангулированной поверхности
    QMutex mTrianglesMutex;
    //! Мьютекс для синхронизации доступа к данным сетки
    QMutex mQuadsMutex;

    QMutex mGridMutex;

    float mMaxZ;
    float mMinZ;


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
