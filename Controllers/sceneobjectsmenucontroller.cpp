#include "sceneobjectsmenucontroller.h"

#include <QQmlProperty>

#include <QmlObjectNames.h>
#include <bottomtrack.h>
#include <surface.h>

SceneObjectsMenuController::SceneObjectsMenuController(GraphicsScene3dView* sceneView,
                                                       QObject *parent)
: QmlComponentController(parent)
, m_sceneView(sceneView)
, m_objectListModel(new QStandardItemModel(this))
{
    auto roleNames = m_objectListModel->roleNames();

    roleNames[Qt::UserRole + 1] = "type";

    m_objectListModel->setItemRoleNames(roleNames);
}

void SceneObjectsMenuController::onObjectListIndexChanged(int row)
{
    if(!m_component)
        return;

    auto item = m_objectListModel->item(row);

    auto type = item->data(Qt::UserRole+1);

    QUrl componentUrl("MenuPlaceholder.qml");

    switch(type.toInt()){
    case static_cast <int>(SceneObject::SceneObjectType::BottomTrack):
        componentUrl.setUrl("BottomTrackParamsMenu.qml");
        break;
    case static_cast <int>(SceneObject::SceneObjectType::Surface):
        componentUrl.setUrl("SurfaceParamsMenu.qml");
        break;
    default:
        return;
    }

    QMetaObject::invokeMethod(m_component, "loadComponent", Q_ARG(QUrl, componentUrl));
}

void SceneObjectsMenuController::onObjectListItemNameChanged(int row, const QString &name)
{
    Q_UNUSED(row)
    Q_UNUSED(name)
}

void SceneObjectsMenuController::onObjectListItemRemoveButtonClicked(int row)
{
    Q_UNUSED(row)
}

void SceneObjectsMenuController::onAddObjectButtonClicked(SceneObject::SceneObjectType type)
{
    if(!m_component || !m_sceneView || !m_sceneView->scene())
        return;

    std::shared_ptr <SceneGraphicsObject> graphicsObject;

    switch (type) {
    case SceneObject::SceneObjectType::BottomTrack:
        graphicsObject = std::make_shared <BottomTrack>();
    case SceneObject::SceneObjectType::Surface:
        graphicsObject = std::make_shared <Surface>();
        break;
    default:
        return;
    }

    graphicsObject->setName(QStringLiteral("Scene object"));

    //m_sceneView->scene()->addGraphicsObject(graphicsObject);

    auto item = new QStandardItem(graphicsObject->name());

    item->setData(static_cast <int>(type));

    m_objectListModel->appendRow(item);
}

void SceneObjectsMenuController::onLoaderComponentLoaded(QObject* component, const QUrl& url)
{
    Q_UNUSED(url)
}

void SceneObjectsMenuController::setSceneView(GraphicsScene3dView *sceneView)
{
    m_sceneView = sceneView;
}

void SceneObjectsMenuController::findComponent()
{
    m_component = m_engine->findChild<QObject*>(QmlObjectNames::activeObjectParamsMenuLoaderName);
}

QStandardItemModel *SceneObjectsMenuController::objectListModel() const
{
    return m_objectListModel;
}
