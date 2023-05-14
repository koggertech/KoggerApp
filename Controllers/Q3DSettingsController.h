#ifndef Q3DSETTINGSCONTROLLER_H
#define Q3DSETTINGSCONTROLLER_H

#include <QObject>
#include <QThread>
#include "Q3DSceneModel.h"

using ModelPointer  = std::shared_ptr <Q3DSceneModel>;

class Q3DSettingsController : public QObject
{
    Q_OBJECT
public:

    //! @brief Конструктор
    //! @param[in] pParent - указатель на объект родителя
    explicit Q3DSettingsController(QObject *parent = nullptr);

    //! @brief Деструктор
    ~Q3DSettingsController();

    //! @brief Установить модель 3D - сцены
    //! @param[in] pModel - указатель на объект модели 3D - сцены
    void setModel(const ModelPointer pModel);

    Q_INVOKABLE void changeContourVisibility(bool visible);

    Q_INVOKABLE void changeContourColor(QColor color);

    Q_INVOKABLE void changeContourLineWidth(float width);

    Q_INVOKABLE void changeContourKeyPointsVisibility(bool visible);

    Q_INVOKABLE void changeContourKeyPointsColor(QColor color);

    Q_INVOKABLE void changeGridType(QString type);

    Q_INVOKABLE void changeGridCellSize(double length);

public slots:

    //! @brief Изменить метод расчета поверхности
    //! @param[in] method - метод расчета в строковом формате
    void changeCalculationMethod(const QString& method);

    //! @brief Изменить метод сглаживания поверхности
    //! @param[in] method - метод сглаживания в строковом формате
    void changeSmoothingMethod(const QString& method);

    //! @brief Отобразить/скрыть 3D - сцену
    //! @param[in] visible - признак необходимости отобразить
    //! / скрыть 3D - цену
    void changeSceneVisibility(const bool visible);

    //! @brief Отобразить/скрыть трек
    //! @param[in] visible - признак необходимости отобразить
    //! / скрыть трек
    void changeBottomTrackVisibility(const bool visible);

    //! @brief Отобразить/скрыть поверхность
    //! @param[in] visible - признак необходимости отобразить
    //! / скрыть полигоны поверхности
    void changeSurfaceVisibility(const bool visible);

    //! @brief Отобразить/скрыть сетку поверхности
    //! @param[in] visible - признак необходимости
    //! отобразить/скрыть сетку поверхности
    void changeSurfaceGridVisibility(const bool visible);



    //! @brief Установить максимальную длину линий интерполяции
    //! @param[in] length - значение ограничения длины граней треугольников
    //! @note Параметр относится к методам расчета поверхности
    void setTriangulationEdgeLengthLimit(double length);

    //! @brief Установить уровень интерполяции поверхности
    //! @param[in] level - текстовое значение уровня
    //! @note Параметр относится к методу сглаживания UGIN
    void setInterpolationLevel(const QString& level);

    //! @brief Обновить данные для отображения
    void updateDisplayedObject();

private:

    //! Указатель на модель 3D - сцены
    ModelPointer mpModel;
    //! Указатель на поток расчета поверхности
    QThread* mpThread;

};

#endif // Q3DSETTINGSCONTROLLER_H
