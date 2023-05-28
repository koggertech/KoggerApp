#ifndef Q3DSCENEMODEL_H
#define Q3DSCENEMODEL_H

#include <QObject>
#include <QSet>
#include <QVector>
#include <QVector3D>
#include <QMutex>
#include <QFile>
#include <QDataStream>
#include <qdebug.h>
#include <QMap>
#include <QStringListModel>
#include <QOpenGLShaderProgram>
#include <QStack>

#include <memory>
#include <algorithm>
#include <numeric>
#include <stack>


#include "surface.h"
#include "bottomtrack.h"
#include "contour.h"
#include <markupgrid.h>
#include "sceneparams.h"
#include "processingcasefactory.h"
#include <abstractbottomtrackfilter.h>
#include <bottomtrackprovider.h>
#include <gridgenerator.h>

using Vector3             = QVector <QVector3D>;
using Vector3Pointer      = std::shared_ptr <Vector3>;
using RawTrianglesPointer = std::shared_ptr <std::vector <Triangle <double>>>;

class Q3DSceneModel : public QObject
{
    Q_OBJECT

public:

    //! @brief Конструктор
    //! @param[in] pParent - указатель на объект родителя
    explicit Q3DSceneModel(std::shared_ptr <BottomTrackProvider> bottomTrackProvider,
                           QObject *pParent = nullptr);

    //! @brief Передать указатель на данные трека
    //! @param[in] указатель на данные трека
    void setBottomTrack(const QVector <QVector3D>& pBottomTrack);

    void setBottomTrackFilter(std::shared_ptr <AbstractBottomTrackFilter> filter);

    //! @brief Отобразить/скрыть 3D - сцену
    //! @param[in]  - признак необходимости отобразить
    //! / скрыть 3D - цену
    void changeSceneVisibility(const bool visible);

    //! @brief Отобразить/скрыть трек
    //! @param[in]  - признак необходимости отобразить
    //! / скрыть трек
    void changeBottomTrackVisibility(const bool visible);

    //! @brief Отобразить/скрыть поверхность
    //! @param[in]  - признак необходимости отобразить
    //! / скрыть полигоны поверхности
    void changeSurfaceVisibility(const bool visible);

    //! @brief Отобразить/скрыть сетку поверхности
    //! @param[in] visible - признак необходимости
    //! отобразить/скрыть сетку поверхности
    void changeSurfaceGridVisibility(const bool visible);

    //! @brief Отобразить/скрыть контур.
    //! @param[in] visible - признак необходимости
    //! отобразить/скрыть контур.
    void changeContourVisibility(bool visible);

    //! @brief Установить цвет контура.
    //! @param[in] color Цвет контура.
    void changeContourColor(QColor color);

    //! @brief Установить толщину линии контура.
    //! @param[in] width Толщина линии контура.
    void changeContourLineWidth(float width);

    /**
     * @brief Отобразить/скрыть ключевые точки контура
     * @param[in] visible - признак необходимости
     * отобразить/скрыть ключевые точки контура.
     */
    void changeContourKeyPointsVisibility(bool visible);

    /**
     * @brief Установить цвет ключевых точек контура.
     * @param[in] color Цвет ключевых точек контура.
     */
    void changeContourKeyPointsColor(QColor color);

    //! @brief Изменить метод расчета поверхности
    //! @param[in] method - метод расчета в строковом формате
    void changeCalculationMethod(const QString& method);

    //! @brief Изменить метод сглаживания поверхности
    //! @param[in] method - метод сглаживания в строковом формате
    void changeSmoothingMethod(const QString& method);

    //! @brief Установить максимальную длину линий интерполяции
    //! @param[in] length - значение ограничения длины граней треугольников
    //! @note Параметр относится к методам расчета поверхности
    void setTriangulationEdgeLengthLimit(double length);

    void changeGridType(QString type);

    void changeGridCellSize(double size);

    void changeMarkupGridCellSize(float size);

    //! @brief Обновить данные для отображения
    void updateSurface();

    /**
     * @brief Очистить данные 3D - сцены
     */
    void clear();

    //! @brief Возвращает максимальное значение поверхность по оси Z
    //! @return Максимальное значение поверхность по оси Z
    float objectMaximumZ() const;

    //! @brief Возвращает минимальное значение поверхность по оси Z
    //! @return Минимальное значение поверхность по оси Z
    float objectMinimumZ() const;

    BottomTrack bottomTrackDisplayedObject();

    Surface surfaceDisplayedObject();

    Contour contourDisplayedObject();

    MarkupGrid markupGridDisplayedObject();

    /**
     * @brief Возвращает куб, описывающий все объекты на сцене.
     * @return Куб, описывающий все объекты на сцене.
     */
    Cube bounds();

    Q_INVOKABLE QColor contourColor() const;

    Q_INVOKABLE float contourLineWidth() const;

    Q_INVOKABLE bool contourVisibility() const;

    Q_INVOKABLE bool contourKeyPointsVisibility() const;

    Q_INVOKABLE QColor contourKeyPointsColor() const;

    //! Return - признак видимости 3D - сцены
    Q_INVOKABLE bool sceneVisibility();

    //! Return - признак видимости трека
    Q_INVOKABLE bool bottomTrackVisible();

    //! Return - признак видимости поверхности
    Q_INVOKABLE bool surfaceVisible();

    //! Return - признак видимости сетки поверхности
    Q_INVOKABLE bool surfaceGridVisible();

    //! Return - признак доступности триангулятора
    Q_INVOKABLE bool triangulationAvailable();

    //! @brief Возвращает текущий установленный метод расчета поверхности
    //! в строковом формате
    //! @return Текущий установленный метод расчета поверхности в строковом формате
    Q_INVOKABLE QString calculationMethod() const;

    //! @brief Возвращает текущий установленный метод сглаживания поверхности
    //! в строковом формате
    //! @return Текущий установленный метод сглаживания поверхности в строковом формате
    Q_INVOKABLE QString smoothingMethod() const;

private:

    void createContour();
    void createContourEx();
    void createContourEx_2();

private:

    bool mSceneVisible = false;              //! Признак видимости 3D - сцены
    std::atomic_bool mIsProcessingAvailable; //< Признак доступности процедуры расчета поверхности
    QMutex mBottomTrackMutex;                //< Мьютекс для синхронизации доступа к данным трека из разных потоков
    QMutex mSurfaceMutex;                    //< Мьютекс для синхронизации доступа к данным поверхности
    QMutex mContourMutex;                    //< Мьютекс для синхронизации доступа к данным контура
    QMutex mMarkupGridMutex;                 //< Мьютекс для синхронизации доступа к данным разметочной сетки.
    QMutex mBoundsMutex;                     //< Мьютекс для синхронизации доступа к данным границ всей сцены.
    BottomTrack mBottomTrackDisplayedObject; //< Отображаемый объект "Трек"
    Surface mSurfaceDisplayedObject;         //< Отображаемый объект "Поверхность"
    Contour mContourDisplayedObject;         //< Отображаемый объект "Контур поверхности"
    MarkupGrid mMarkupGrid;                 //< Сетка разметки сцены
    Cube mBounds;
    SceneParams mParams;                     //< Объект параметров обработки и расчета
    std::shared_ptr <BottomTrackProvider> mpBottomTrackProvider;
    std::shared_ptr <AbstractBottomTrackFilter> mpBottomTrackFilter;

signals:

    /**
     *  @brief Сигнал - оповещение об изменении состояния модели 3D - сцены
     */
    void stateChanged();

    /**
     *  @brief Сигнал - оповещение об изменении данных трека.
     */
    void bottomTrackDataChanged();

    /**
     *  @brief Сигнал - оповещение об изменении свойств трека.
     */
    void bottomTrackPropertiesChanged();

    /**
     *  @brief Сигнал - оповещение об изменении данных поверхности.
     */
    void surfaceDataChanged();

    /**
     *  @brief Сигнал - оповещение об изменении свойств поверхности.
     */
    void surfacePropertiesChanged();

    /**
     *  @brief Сигнал - оповещение об изменении данных сглаженной поверхности.
     */
    void smoothedSurfaceDataChanged();

    /**
     *  @brief Сигнал - оповещение об изменении свойств сглаженной поверхности.
     */
    void smoothedSurfacePropertiesChanged();

    /**
     *  @brief Сигнал - оповещение об изменении данных контура.
     */
    void contourDataChanged();

    /**
     *  @brief Сигнал - оповещение об изменении свойств контура.
     */
    void contourPropertiesChanged();

    void markupGridDataChanged();

};

#endif // Q3DSCENEMODEL_H
