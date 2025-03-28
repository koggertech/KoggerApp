#include "point_group_control_menu_controller.h"
#include "scene3d_view.h"
#include "point_group.h"
#include "point_object.h"

PointGroupControlMenuController::PointGroupControlMenuController(QObject *parent)
    : QmlComponentController{parent}
    , m_pointListModel(new QStandardItemModel)
{
    auto roleNames = m_pointListModel->roleNames();

    roleNames[Qt::UserRole+1] = "color";

    m_pointListModel->setItemRoleNames(roleNames);
}

void PointGroupControlMenuController::setGraphicsSceneView(GraphicsScene3dView *sceneView)
{
    m_graphicsSceneView = sceneView;
}

QStandardItemModel *PointGroupControlMenuController::pointListModel() const
{
    return m_pointListModel.get();
}

void PointGroupControlMenuController::onVisibilityCheckBoxCheckedChanged(bool checked)
{
    if(!m_graphicsSceneView)
        return;

    auto pointGroup = m_graphicsSceneView->pointGroup();

    QMetaObject::invokeMethod(reinterpret_cast <QObject*>(pointGroup.get()), "setVisible", Q_ARG(bool, checked));
}

void PointGroupControlMenuController::onPointListItemRemoveButtonClicked(int index)
{
    if(index < 0)
        return;

    if(!m_graphicsSceneView)
        return;

    m_pointListModel->invisibleRootItem()->removeRow(index);

    auto pointGroup = m_graphicsSceneView->pointGroup();

    QMetaObject::invokeMethod(pointGroup.get(), "removeAt", Q_ARG(int, index));
}

void PointGroupControlMenuController::onCoordSpinBoxValueChanged(QVector3D coord, int row)
{
    if(row < 0)
        return;

    if(!m_graphicsSceneView)
        return;

    auto pointGroup = m_graphicsSceneView->pointGroup();
    auto point = pointGroup->at(row);

    m_pointListModel->item(row)->setData(QString("x: %1, y: %2, z: %3")
                                           .arg(coord.x())
                                           .arg(coord.y())
                                           .arg(coord.z()),
                                           Qt::DisplayRole);

    QMetaObject::invokeMethod(point.get(), "setPosition", Q_ARG(QVector3D, coord));
}

void PointGroupControlMenuController::onPointColorDialogAccepted(QColor color, int row)
{
    if(!m_graphicsSceneView)
        return;

    if(row < 0)
        return;

    auto pointGroup = m_graphicsSceneView->pointGroup();
    auto point = pointGroup->at(row);

    auto item = m_pointListModel->item(row);
    item->setData(color, Qt::UserRole + 1);

    QMetaObject::invokeMethod(point.get(), "setColor", Q_ARG(QColor, color));
}

void PointGroupControlMenuController::onPointWidthSpinBoxValueChanged(int width, int row)
{
    if(!m_graphicsSceneView)
        return;

    if(row < 0)
        return;

    auto pointGroup = m_graphicsSceneView->pointGroup();
    auto point = pointGroup->at(row);

    QMetaObject::invokeMethod(point.get(), "setWidth", Q_ARG(qreal, static_cast <qreal>(width)));
}

void PointGroupControlMenuController::onAddPointButtonClicked()
{
    if(!m_graphicsSceneView)
        return;

    auto pointGroup = m_graphicsSceneView->pointGroup();
    auto point = std::make_shared<PointObject>();

    m_pointListModel->invisibleRootItem()->appendRow(new QStandardItem(QString("x: %1, y: %2, z: %3")
                                                                       .arg(point->x())
                                                                       .arg(point->y())
                                                                       .arg(point->z())));

    QMetaObject::invokeMethod(pointGroup.get(), "append", Q_ARG(std::shared_ptr<PointObject>, point));
}

PointObject *PointGroupControlMenuController::pointAt(int row)
{
    if(!m_graphicsSceneView)
        return nullptr;

    auto pointGroup = m_graphicsSceneView->pointGroup();

    return pointGroup->at(row).get();
}

QVector3D PointGroupControlMenuController::pointCoord(int row) const
{
    if(!m_graphicsSceneView)
        return {};

    if(row < 0)
        return {};

    auto pointGroup = m_graphicsSceneView->pointGroup();
    auto point = pointGroup->at(row);

    return {point->x(), point->y(), point->z()};
}

void PointGroupControlMenuController::findComponent()
{

}

PointGroup *PointGroupControlMenuController::pointGroup() const
{
    if(!m_graphicsSceneView)
        return nullptr;

    return m_graphicsSceneView->pointGroup().get();
}
