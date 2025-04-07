#include "polygon_group_control_menu_controller.h"

#include "scene3d_view.h"
#include "polygon_group.h"
#include "polygon_object.h"
#include "point_object.h"

PolygonGroupControlMenuController::PolygonGroupControlMenuController(QObject *parent)
    : QmlComponentController(parent)
    , m_polygonListModel(new QStandardItemModel)
{
    auto roleNames = m_polygonListModel->roleNames();

    roleNames[Qt::UserRole+1] = "color";

    m_polygonListModel->setItemRoleNames(roleNames);
}

void PolygonGroupControlMenuController::onVisibilityCheckBoxCheckedChanged(bool checked)
{
    if(!m_graphicsSceneView)
        return;

    auto polygonGroup = m_graphicsSceneView->polygonGroup();

    QMetaObject::invokeMethod(reinterpret_cast <QObject*>(polygonGroup.get()), "setVisible", Q_ARG(bool, checked));
}

void PolygonGroupControlMenuController::onPolygonListItemRemoveButtonClicked(const QModelIndex &index)
{
    if(!index.isValid())
        return;

    if(!m_graphicsSceneView)
        return;

    auto polygonGroup = m_graphicsSceneView->polygonGroup();

    if(index.parent() == m_polygonListModel->invisibleRootItem()->index()){
        QMetaObject::invokeMethod(reinterpret_cast <QObject*>(polygonGroup.get()), "removePolygonAt", Q_ARG(int, index.row()));
    }else{
        auto polygon = polygonGroup->at(index.parent().row());
        QMetaObject::invokeMethod(reinterpret_cast <QObject*>(polygon.get()), "removeAt", Q_ARG(int, index.row()));
    }

    m_polygonListModel->removeRows(index.row(), 1, index.parent());
}

void PolygonGroupControlMenuController::onPointCoordSpinBoxValueChanged(QVector3D coord, const QModelIndex &index)
{
    if(!m_graphicsSceneView)
        return;

    if(!index.isValid() || !index.parent().isValid())
        return;

    auto polygonGroup = m_graphicsSceneView->polygonGroup();
    auto polygon = polygonGroup->at(index.parent().row());
    auto point = polygon->at(index.row());

    auto item = m_polygonListModel->itemFromIndex(index);

    item->setData(QString("x: %1, y: %2, z: %3")
                          .arg(coord.x())
                          .arg(coord.y())
                          .arg(coord.z()),
                  Qt::DisplayRole);

    QMetaObject::invokeMethod(point.get(),
                              "setPosition",
                              Q_ARG(QVector3D, coord));
}

void PolygonGroupControlMenuController::onAddPolygonButtonClicked()
{
    if(!m_graphicsSceneView)
        return;

    auto polygonGroup = m_graphicsSceneView->polygonGroup();
    auto polygon = std::make_shared<PolygonObject>();

    auto item = new QStandardItem("Polygon");
    item->setData(polygon->color(), Qt::UserRole + 1);

    m_polygonListModel->invisibleRootItem()->setChild(m_polygonListModel->rowCount(), item);

    QMetaObject::invokeMethod(polygonGroup.get(), "addPolygon", Q_ARG(std::shared_ptr<PolygonObject>, polygon));
}

void PolygonGroupControlMenuController::onAddPointButtonClicked(const QModelIndex &index)
{
    if(!m_graphicsSceneView)
        return;

    if(!index.isValid() || index.parent().isValid())
        return;

    auto polygonGroup = m_graphicsSceneView->polygonGroup();
    auto polygon      = polygonGroup->at(index.row());

    auto point = std::make_shared<PointObject>();
    auto item = m_polygonListModel->itemFromIndex(index);

    item->setChild(item->rowCount(), new QStandardItem(QString("x: %1, y: %2, z: %3")
                                                                .arg(point->x())
                                                                .arg(point->y())
                                                                .arg(point->z())));

    QMetaObject::invokeMethod(polygon.get(), "append", Q_ARG(std::shared_ptr<PointObject>, point));
}

void PolygonGroupControlMenuController::onPolygonColorDialogAccepted(QColor color, const QModelIndex& index)
{
    if(!m_graphicsSceneView)
        return;

    if(!index.isValid() || index.parent().isValid())
        return;

    auto polygonGroup = m_graphicsSceneView->polygonGroup();
    auto polygon      = polygonGroup->at(index.row());

    auto item = m_polygonListModel->itemFromIndex(index);
    item->setData(color, Qt::UserRole + 1);

    QMetaObject::invokeMethod(polygon.get(), "setColor", Q_ARG(QColor, color));
}

void PolygonGroupControlMenuController::setGraphicsSceneView(GraphicsScene3dView *sceneView)
{
    m_graphicsSceneView = sceneView;
}

QVector3D PolygonGroupControlMenuController::getPointCoord(const QModelIndex &pointIndex)
{
    if(!m_graphicsSceneView)
        return {};

    if(!pointIndex.isValid() || !pointIndex.parent().isValid())
        return {};

    auto polygonGroup = m_graphicsSceneView->polygonGroup();
    auto polygon      = polygonGroup->at(pointIndex.parent().row());

    return {}/*polygon->at(pointIndex.row())*/;
}

PointObject *PolygonGroupControlMenuController::pointAt(const QModelIndex &pointIndex) const
{
    if(!m_graphicsSceneView)
        return nullptr;

    if(!pointIndex.isValid() || !pointIndex.parent().isValid())
        return nullptr;

    auto polygonGroup = m_graphicsSceneView->polygonGroup();
    auto polygon      = polygonGroup->at(pointIndex.parent().row());

    return polygon->at(pointIndex.row()).get();
}

QStandardItemModel *PolygonGroupControlMenuController::polygonListModel() const
{
    return m_polygonListModel.get();
}

PolygonGroup *PolygonGroupControlMenuController::polygonGroup() const
{
    if(!m_graphicsSceneView)
        return nullptr;

    return m_graphicsSceneView->polygonGroup().get();
}

void PolygonGroupControlMenuController::findComponent()
{}
