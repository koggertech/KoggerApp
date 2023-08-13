#include "pointsetparamscontroller.h"

PointSetParamsController::PointSetParamsController(std::shared_ptr<ActiveObjectProvider> activeObjectProvider,
                                                   std::shared_ptr<SceneObjectsListModel> sceneObjectsListModel,
                                                   QObject *parent)
: QObject(parent)
, mActiveObjectProvider(activeObjectProvider)
, mSceneObjectsListModel(sceneObjectsListModel)
{

}

void PointSetParamsController::changePointSetVisibility(bool visible)
{
    auto pointSet = takePointSet();

    if(!pointSet)
        return;

    pointSet->setVisible(visible);
}

void PointSetParamsController::addPoint(QVector3D point)
{
    auto pointSet = takePointSet();

    if(!pointSet)
        return;

    pointSet->append(point);
}

void PointSetParamsController::removePoint(int index)
{
    auto pointSet = takePointSet();

    if(!pointSet)
        return;

    pointSet->removePoint(index);
}

void PointSetParamsController::changePointCoord(int index, QVector3D coord)
{
    auto pointSet = takePointSet();

    if(!pointSet)
        return;

    pointSet->changePointCoord(index, coord);
}

PointSet *PointSetParamsController::takePointSet()
{
    auto object = mActiveObjectProvider->activeObject();

    if(!object)
        return nullptr;

    auto pointSet = dynamic_cast <PointSet*>(object.get());

    return pointSet;
}
