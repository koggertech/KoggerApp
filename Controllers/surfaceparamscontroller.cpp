#include "surfaceparamscontroller.h"
#include <surfaceprocessor.h>

SurfaceParamsController::SurfaceParamsController(std::shared_ptr<ActiveObjectProvider> activeObjectProvider,
                                                 std::shared_ptr<SceneObjectsListModel> sceneObjectsListModel,
                                                 QObject *parent)
    : QObject(parent)
    , mActiveObjectProvider(activeObjectProvider)
    , mSceneObjectsListModel(sceneObjectsListModel)
{}

void SurfaceParamsController::changeSurfaceVisibility(bool visible)
{
    auto surface = takeSurface();

    if(!surface)
        return;

    surface->setVisible(visible);
}

void SurfaceParamsController::changeSurfaceContourVisibility(bool visible)
{
    auto surface = takeSurface();

    if(!surface)
        return;

    surface->contour()->setVisible(visible);
}

void SurfaceParamsController::changeSurfaceGridVisibility(bool visible)
{
    auto surface = takeSurface();

    if(!surface)
        return;

    surface->grid()->setVisible(visible);
}

void SurfaceParamsController::changeSurfaceContourColor(QColor color)
{
    auto surface = takeSurface();

    if(!surface)
        return;

    surface->contour()->setColor(color);

    qDebug() << color;
}

void SurfaceParamsController::changeSurfaceGridColor(QColor color)
{
    auto surface = takeSurface();

    if(!surface)
        return;

    surface->grid()->setColor(color);
}

void SurfaceParamsController::updateSurface(int  bottomTrackObjectIndex,
                                            bool interpolateWithGrid,
                                            int  gridCellSize)
{
    auto bottomTrackVertexObject = mSceneObjectsListModel->get(bottomTrackObjectIndex);

    if (!bottomTrackVertexObject)
        return;

    auto surfaceVertexObject = mActiveObjectProvider->activeObject();

    if (!surfaceVertexObject)
        return;

    SurfaceProcessor surfaceProcessor;

    surfaceProcessor.process(bottomTrackVertexObject->cdata(),
                             surfaceVertexObject,
                             interpolateWithGrid,
                             gridCellSize);

    auto surface = takeSurface();

    if (!surface)
        return;

    surface->setBottomTrackId(bottomTrackVertexObject->id());
}

Surface *SurfaceParamsController::takeSurface()
{
    auto object = mActiveObjectProvider->activeObject();

    if(!object)
        return nullptr;

    auto surface = dynamic_cast <Surface*>(object.get());

    return surface;
}
