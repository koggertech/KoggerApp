#ifndef POLYGONGROUPCONTROLMENUCONTROLLER_H
#define POLYGONGROUPCONTROLMENUCONTROLLER_H

#include <QStandardItemModel>
#include <QVector3D>

#include <memory>

#include "qml_component_controller.h"

class PolygonGroup;
class GraphicsScene3dView;
class PointObject;
class PolygonGroupControlMenuController : public QmlComponentController
{
    Q_OBJECT
    Q_PROPERTY(PolygonGroup*       polygonGroup     READ polygonGroup     CONSTANT)
    Q_PROPERTY(QStandardItemModel* polygonListModel READ polygonListModel CONSTANT)

public:
    explicit PolygonGroupControlMenuController(QObject *parent = nullptr);

    /**
     * @brief Process of polygon group visibility check box checked state changing
     * @param[in] checked - visibility check box checked state
     */
    Q_INVOKABLE void onVisibilityCheckBoxCheckedChanged(bool checked);

    /**
     * @brief Process of polygon group item remove button clicking
     * @param[in] index - model index of polygon item
     */
    Q_INVOKABLE void onPolygonListItemRemoveButtonClicked(const QModelIndex& index);

    /**
     * @brief Process of point coord spinbox value changing
     * @param[in] coord - coord to be set to point
     * @param[in] index - model index of point item
     */
    Q_INVOKABLE void onPointCoordSpinBoxValueChanged(QVector3D coord, const QModelIndex& index);

    /**
     * @brief Process of add polygon button clicking
     */
    Q_INVOKABLE void onAddPolygonButtonClicked();

    /**
     * @brief Process of add point button clicking
     * @param[in] index - model index of polygon item
     */
    Q_INVOKABLE void onAddPointButtonClicked(const QModelIndex& index);

    /**
     * @brief Process of polygon color dialog accepting
     * @param[in] color - color to be set to polygon
     * @param[in] index - model index of polygon item
     */
    Q_INVOKABLE void onPolygonColorDialogAccepted(QColor color, const QModelIndex& index);

    /**
     * @brief Sets pointer to graphics scene view
     * @param[in] sceneView - pointer to graphics scene view
     */
    void setGraphicsSceneView(GraphicsScene3dView* sceneView);

    Q_INVOKABLE QVector3D getPointCoord(const QModelIndex& pointIndex);

    Q_INVOKABLE PointObject* pointAt(const QModelIndex& pointIndex) const;

private:
    /**
     * @brief Returns pointer to model of polygon group
     * @return Pointer to model of polygon group
     */
    QStandardItemModel* polygonListModel() const;

    /**
     * @brief Returns pointer to polygon group
     * @return Pointer to polygon group
     */
    PolygonGroup* polygonGroup() const;



protected:
    void findComponent() override;

private:
    GraphicsScene3dView* m_graphicsSceneView = nullptr;
    std::unique_ptr <QStandardItemModel> m_polygonListModel;
};

#endif // POLYGONGROUPCONTROLMENUCONTROLLER_H
