#include "polygongroupparamscontroller.h"
#include <polygongroup.h>

PolygonGroupParamsController::PolygonGroupParamsController(std::shared_ptr<ActiveObjectProvider> activeObjectProvider,
                                                           QObject *parent)
    : QObject(parent)
    , mActiveObjectProvider(activeObjectProvider)
{}

PolygonGroupParamsController::~PolygonGroupParamsController()
{

}

void PolygonGroupParamsController::addPolygon() const
{
    auto sceneObject = mActiveObjectProvider->activeObject();

    auto polygonGroup = qobject_cast <PolygonGroup*> (sceneObject.get());

    if(!polygonGroup)
        return;

    polygonGroup->addPolygon(
                    std::make_shared <PolygonObject>()
                );
}

void PolygonGroupParamsController::removePolygon(int index) const
{
    auto sceneObject = mActiveObjectProvider->activeObject();

    if(!sceneObject)
        return;

    auto polygonGroup = qobject_cast <PolygonGroup*> (sceneObject.get());

    if(!polygonGroup)
        return;

    polygonGroup->removePolygon(index);
}

void PolygonGroupParamsController::addPoint(int polygonIndex) const
{
    auto sceneObject = mActiveObjectProvider->activeObject();

    if(!sceneObject)
        return;

    auto polygonGroup = qobject_cast <PolygonGroup*> (sceneObject.get());

    if(!polygonGroup)
        return;

    auto polygon = polygonGroup->at(polygonIndex);

    if(polygon)
        polygon->append({0.0f,0.0f,0.0f});
}

void PolygonGroupParamsController::removePoint(int polygonIndex, int pointIndex) const
{
    auto sceneObject = mActiveObjectProvider->activeObject();

    if(!sceneObject)
        return;

    auto polygonGroup = qobject_cast <PolygonGroup*> (sceneObject.get());

    if(!polygonGroup)
        return;

    auto polygon = polygonGroup->at(polygonIndex);

    if(polygon)
        polygon->remove(polygonIndex);
}

void PolygonGroupParamsController::updatePointCoord(int polygonIndex, int pointIndex, const QVector3D &point) const
{
    auto sceneObject = mActiveObjectProvider->activeObject();

    if(!sceneObject)
        return;

    auto polygonGroup = qobject_cast <PolygonGroup*> (sceneObject.get());

    if(!polygonGroup)
        return;

    auto polygon = polygonGroup->at(polygonIndex);

    if(polygon)
        polygon->replacePoint(pointIndex, point);
}
