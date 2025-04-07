#include "usbl_view_control_menu_controller.h"

#include "scene3d_view.h"
#include <random>


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
    if (!m_graphicsSceneView) {
        return;
    }

    m_graphicsSceneView->getUsblViewPtr()->setVisible(checked);
}

void UsblViewControlMenuController::onUpdateUsblViewButtonClicked()
{
    if (!m_graphicsSceneView) {
        return;
    }

    QMap<int, UsblView::UsblObjectParams> tracks;

    for (int j = 0; j < 5; ++j) {
        UsblView::UsblObjectParams params;
        params.isTrackVisible_ = static_cast<bool>(std::rand() % 2);
        !j ? params.type_ = UsblView::UsblObjectType::kUsbl : params.type_ = UsblView::UsblObjectType::kBeacon;
        params.objectColor_ = QColor(std::rand() % 255,std::rand() % 255,std::rand() % 255);
        params.lineWidth_ = static_cast<qreal>(std::rand() % 7 + 1);
        params.pointRadius_ = static_cast<float>(std::rand() % 20 + 15);
        QVector <QVector3D> data;
        int dataSize = std::rand() % 150 + 50;
        for (int i = 0; i < dataSize; ++i){
            QVector3D A;
            A.setX(i + std::rand() % 3 + 1);
            A.setY(i + std::rand() % 7 - 1);
            A.setZ(i + std::rand() % 10 + 1);
            data.append(A);
        }
        params.data_ = data;
        tracks.insert(j, params);
    }
    m_graphicsSceneView->getUsblViewPtr()->setTrackRef(tracks);
}


void UsblViewControlMenuController::onClearUsblViewButtonClicked()
{
    if (!m_graphicsSceneView) {
        return;
    }

    m_graphicsSceneView->getUsblViewPtr()->clearTracks();
}

UsblView *UsblViewControlMenuController::getUsblViewPtr() const
{
    if (!m_graphicsSceneView) {
        return nullptr;
    }

    return m_graphicsSceneView->getUsblViewPtr().get();
}
