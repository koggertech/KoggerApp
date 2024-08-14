#include "usbl_view_control_menu_controller.h"

#include "graphicsscene3dview.h"
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

    // data
    QVector <QVector3D> data;
    for (int i = 0; i < 100; ++i){
        QVector3D A;
        A.setX(i + std::rand() % 3 + 1);
        A.setY(i + std::rand() % 7 - 1);
        A.setZ(i + std::rand() % 10 + 1);
        data.append(A);
    }
    m_graphicsSceneView->getUsblViewPtr()->setData(data , GL_LINE_STRIP);

    // color
    auto currColor = QColor(std::rand() % 255,std::rand() % 255,std::rand() % 255);
    m_graphicsSceneView->getUsblViewPtr()->setColor(currColor);

    // width
    m_graphicsSceneView->getUsblViewPtr()->setWidth(static_cast<qreal>(std::rand() % 7)); // line
    m_graphicsSceneView->getUsblViewPtr()->setPointRadius(static_cast<float>(std::rand() % 50 + 15)); // point

    // track visible
    //m_graphicsSceneView->getUsblViewPtr()->setTrackVisible(std::rand() % 2 == 0);
}

UsblView *UsblViewControlMenuController::getUsblViewPtr() const
{
    if (!m_graphicsSceneView) {
        return nullptr;
    }

    return m_graphicsSceneView->getUsblViewPtr().get();
}
