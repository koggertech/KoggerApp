#include "usbl_view_control_menu_controller.h"

#include "graphicsscene3dview.h"


UsblViewControlMenuController::UsblViewControlMenuController(QObject *parent) :
    QmlComponentController(parent)
{

}

void UsblViewControlMenuController::setGraphicsSceneView(GraphicsScene3dView *sceneView)
{
    m_graphicsSceneView = sceneView;
}

void UsblViewControlMenuController::findComponent()
{
    m_component = m_engine->findChild<QObject*>("activeObjectParamsMenuLoader"); // ?
}

void UsblViewControlMenuController::onUsblViewVisibilityCheckBoxCheckedChanged(bool checked)
{
    qDebug() << "onUsblViewVisibilityCheckBoxCheckedChanged: " << checked;

    if (!m_graphicsSceneView) {
        return;
    }

    m_graphicsSceneView->getUsblViewPtr()->setVisible(checked);
}

void UsblViewControlMenuController::onUpdateUsblViewButtonClicked()
{
    qDebug() << "onUpdateUsblViewButtonClicked";

    if (!m_graphicsSceneView) {
        return;
    }

    //m_graphicsSceneView->getUsblViewPtr()->updateData();
    //m_graphicsSceneView->setNeedToRefreshUsblTexture(true);
}

UsblView *UsblViewControlMenuController::getUsblViewPtr() const
{
    if (!m_graphicsSceneView) {
        return nullptr;
    }

    return m_graphicsSceneView->getUsblViewPtr().get();
}
