#include "surfaceparamscontroller.h"
#include <bottomtrack.h>
#include <surfaceprocessor.h>

SurfaceParamsController::SurfaceParamsController(std::shared_ptr<ActiveObjectProvider> activeObjectProvider,
                                                 std::shared_ptr<SceneObjectsListModel> sceneObjectsListModel,
                                                 QObject *parent)
    : QObject(parent)
    , mActiveObjectProvider(activeObjectProvider)
    , mSceneObjectsListModel(sceneObjectsListModel)
{}

SurfaceParamsController::~SurfaceParamsController()
{

}

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
    auto objects = mSceneObjectsListModel->dataByType(SceneObject::SceneObjectType::BottomTrack);

    if(bottomTrackObjectIndex < 0 ||
       bottomTrackObjectIndex >= objects.count())
    {
        return;
    }

    auto bottomTrackSceneObject = objects.at(bottomTrackObjectIndex);

    auto bottomTrack = qobject_cast <BottomTrack*>(bottomTrackSceneObject.get());

    if (!bottomTrack)
        return;

    auto surfaceSceneObject = mActiveObjectProvider->activeObject();

    auto surface = qobject_cast <Surface*>(surfaceSceneObject.get());

    if (!surface)
        return;

    SurfaceProcessor surfaceProcessor;

    surfaceProcessor.process(bottomTrack->cdata(),
                             surface,
                             interpolateWithGrid,
                             gridCellSize);

    surface->setBottomTrackId(bottomTrack->id());
}

Surface *SurfaceParamsController::takeSurface()
{
    auto object = mActiveObjectProvider->activeObject();

    if(!object)
        return nullptr;

    auto surface = qobject_cast <Surface*>(object.get());

    return surface;
}
