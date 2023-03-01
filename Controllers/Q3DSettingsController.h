#ifndef Q3DSETTINGSCONTROLLER_H
#define Q3DSETTINGSCONTROLLER_H

#include <QObject>
#include <QThread>
#include "../Model/Q3DSceneModel.h"

using ModelPointer  = std::shared_ptr <Q3DSceneModel>;

class Q3DSettingsController : public QObject
{
    Q_OBJECT
public:

    //! Конструктор
    explicit Q3DSettingsController(QObject *parent = nullptr);
    //! Деструктор
    ~Q3DSettingsController();

    //! Установить модель 3D - сцены
    void setModel(const ModelPointer pModel);

public slots:
    //! Изменить тип отображаемого объекта (GPS - трек, поверхность, меш)
    void chageDisplayedObjectType(const QString& type);
    //! Изменить отображаемую стадию расчитанной поверхности
    void chageDisplayedStage(const QString& stage);
    //! Отобразить/скрыть 3D - сцену
    void changeSceneVisibility(const bool visible);
    //! Отобразить/скрыть трек
    void changeBottomTrackVisibility(const bool visible);
    //! Отобразить/скрыть поверхность
    void changeSurfaceVisibility(const bool visible);
    //! Отобразить/скрыть сетку поверхности
    void changeSurfaceGridVisibility(const bool visible);
    //! Установить максимальную длину линий интерполяции
    void changeMaxTriangulationLineLength(const int length);
    //! Установить уровень интерполяции поверхности
    void setInterpolationLevel(const QString& level);
    //! Обновить данные для отображения
    void updateDisplayedObject();

private:

    //! Указатель на модель 3D - сцены
    ModelPointer mpModel;
    //! Указатель на поток расчета поверхности
    QThread* mpThread;

};

#endif // Q3DSETTINGSCONTROLLER_H
