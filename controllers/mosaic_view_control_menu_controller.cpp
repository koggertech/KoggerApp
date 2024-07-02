#include "mosaic_view_control_menu_controller.h"

//#include "QmlObjectNames.h"
#include "graphicsscene3dview.h"

// #include <bottomtrack.h>
// #include <nearestpointfilter.h>
// #include <maxpointsfilter.h>


MosaicViewControlMenuController::MosaicViewControlMenuController(QObject *parent) :
    QmlComponentController(parent)
{

}

void MosaicViewControlMenuController::setGraphicsSceneView(GraphicsScene3dView *sceneView)
{
    m_graphicsSceneView = sceneView;
}

void MosaicViewControlMenuController::findComponent()
{
    m_component = m_engine->findChild<QObject*>("activeObjectParamsMenuLoader"); // ?
}

void MosaicViewControlMenuController::onMosaicViewVisibilityCheckBoxCheckedChanged(bool checked)
{
    qDebug() << "onMosaicViewVisibilityCheckBoxCheckedChanged: " << checked;

    if (!m_graphicsSceneView)
        return;

    m_graphicsSceneView->getMosaicViewPtr()->setVisible(checked);
}

void MosaicViewControlMenuController::onUpdateMosaicViewButtonClicked()
{
    qDebug() << "onUpdateMosaicViewButtonClicked";

    if (!m_graphicsSceneView) {
        qDebug().noquote() << "m_graphicsSceneView is nullptr!";
        return;
    }
}

MosaicView *MosaicViewControlMenuController::getMosaicViewPtr() const
{
    if(!m_graphicsSceneView)
        return nullptr;

    return m_graphicsSceneView->getMosaicViewPtr().get();
}
