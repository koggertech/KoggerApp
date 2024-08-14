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
        return;
    }

    auto generateImage  = [](int width, int height) -> QImage
    {
         Q_UNUSED(width);
         Q_UNUSED(height);
         QString imagePath = "C:/Users/salty/Desktop/textures/7.jpg";
         QImage image;
         if (!image.load(imagePath)) {
             qDebug() << "failed to load image: " << imagePath;
         }
         return image;

/*
        QImage texture(width, height, QImage::Format_RGB32);
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                int r = rand() % 256;
                int g = rand() % 256;
                int b = rand() % 256;
                texture.setPixel(x, y, qRgb(r, g, b));
            }
        }

        return texture;
*/
    };

    m_graphicsSceneView->setTextureImage(generateImage(10000,10000));
    m_graphicsSceneView->getMosaicViewPtr()->updateData();
}

MosaicView *MosaicViewControlMenuController::getMosaicViewPtr() const
{
    if(!m_graphicsSceneView)
        return nullptr;

    return m_graphicsSceneView->getMosaicViewPtr().get();
}
