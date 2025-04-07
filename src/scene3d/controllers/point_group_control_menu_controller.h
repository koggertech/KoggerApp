#ifndef POINTGROUPCONTROLMENUCONTROLLER_H
#define POINTGROUPCONTROLMENUCONTROLLER_H

#include <QStandardItemModel>
#include <QVector3D>

#include "qml_component_controller.h"

class GraphicsScene3dView;
class PointGroup;
class PointObject;
class PointGroupControlMenuController : public QmlComponentController
{
    Q_OBJECT
    Q_PROPERTY(PointGroup* pointGroup READ pointGroup CONSTANT)
    Q_PROPERTY(QStandardItemModel* pointListModel READ pointListModel CONSTANT)

public:
    explicit PointGroupControlMenuController(QObject *parent = nullptr);

    void setGraphicsSceneView(GraphicsScene3dView* sceneView);

    QStandardItemModel* pointListModel() const;

    /**
     * @brief Process of point group visibility check box checked state changing
     * @param[in] checked - visibility check box checked state
     */
    Q_INVOKABLE void onVisibilityCheckBoxCheckedChanged(bool checked);

    Q_INVOKABLE void onPointListItemRemoveButtonClicked(int index);

    Q_INVOKABLE void onCoordSpinBoxValueChanged(QVector3D coord, int row);

    Q_INVOKABLE void onPointColorDialogAccepted(QColor color, int row);

    Q_INVOKABLE void onPointWidthSpinBoxValueChanged(int width, int row);

    Q_INVOKABLE void onAddPointButtonClicked();

    Q_INVOKABLE PointObject* pointAt(int row);

    Q_INVOKABLE QVector3D pointCoord(int index) const;

protected:
    void findComponent() override;

private:
    PointGroup* pointGroup() const;

private:
    std::unique_ptr <QStandardItemModel> m_pointListModel;
    GraphicsScene3dView* m_graphicsSceneView = nullptr;
};

#endif // POINTGROUPCONTROLMENUCONTROLLER_H
