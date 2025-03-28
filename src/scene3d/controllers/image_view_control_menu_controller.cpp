#include "image_view_control_menu_controller.h"
#include "scene3d_view.h"
#include "image_view.h"



ImageViewControlMenuController::ImageViewControlMenuController(QObject *parent) :
    QmlComponentController(parent),
    m_graphicsSceneView(nullptr)
{ }

void ImageViewControlMenuController::setGraphicsSceneView(GraphicsScene3dView *sceneView)
{
    m_graphicsSceneView = sceneView;
}

void ImageViewControlMenuController::onVisibilityChanged(bool state)
{
    if (m_graphicsSceneView) {
        m_graphicsSceneView->getImageViewPtr()->setVisible(state);
    }
}

void ImageViewControlMenuController::onUseFilterChanged(bool state)
{
    if (m_graphicsSceneView) {
        m_graphicsSceneView->getImageViewPtr()->setUseLinearFilter(state);
    }
}

void ImageViewControlMenuController::onUpdateClicked(const QString& imagePath, double lat_lt, double lon_lt, double lat_rb, double lon_rb, float z)
{
    if (m_graphicsSceneView) {

        // QVector3D lt {static_cast<float>(x1), static_cast<float>(y1), static_cast<float>(z)};
        // QVector3D rb {static_cast<float>(x2), static_cast<float>(y2), static_cast<float>(z)};


        Dataset * dataset = m_graphicsSceneView->dataset();
        if(dataset) {
            LLARef ref = dataset->getLlaRef();
            Position pos_lt;
            pos_lt.lla = LLA(lat_lt, lon_lt);

            Position pos_rb;
            pos_rb.lla = LLA(lat_rb, lon_rb);

            if(ref.isInit) {
                pos_rb.LLA2NED(&ref);
                pos_lt.LLA2NED(&ref);

                QVector3D lt(pos_lt.ned.n, pos_lt.ned.e, z);
                QVector3D rb(pos_rb.ned.n, pos_rb.ned.e, z);

                m_graphicsSceneView->getImageViewPtr()->updateTexture(imagePath, lt, rb);
            }
        }
    }
}

ImageView* ImageViewControlMenuController::getImageViewPtr() const
{
    if (m_graphicsSceneView) {
        return m_graphicsSceneView->getImageViewPtr().get();
    }
    return nullptr;
}

void ImageViewControlMenuController::findComponent()
{
    m_component = m_engine->findChild<QObject*>("imageViewControlMenu");
}
