#include "sceneobjectslistcontroller.h"

#include <bottomtrack.h>
#include <surface.h>
#include <polygonobject.h>
#include <polygongroup.h>
#include <pointgroup.h>

SceneObjectsListController::SceneObjectsListController(
        std::shared_ptr <ActiveObjectProvider> selectedObjectModel,
        std::shared_ptr <SceneObjectsListModel> model,
        std::shared_ptr <BottomTrackProvider> bottomTrackProvider,
        QObject *parent)
: QObject(parent)
, mpModel(model)
, mpActiveObjectProvider(selectedObjectModel)
, mpBottomTrackProvider(bottomTrackProvider)
{}

SceneObjectsListController::~SceneObjectsListController()
{

}

void SceneObjectsListController::addObject(QString name, SceneObject::SceneObjectType type)
{
    std::shared_ptr <SceneObject> object;

    if(type == SceneObject::SceneObjectType::BottomTrack){
        object = std::make_shared <BottomTrack>();

        auto bottomTrack = qobject_cast <BottomTrack*>(object.get());

        if(bottomTrack){
            bottomTrack->setData(
                            mpBottomTrackProvider->getBottomTrack()
                        );
        }
    }else if (type == SceneObject::SceneObjectType::Surface){
        object = std::make_shared <Surface>();
    }else if (type == SceneObject::SceneObjectType::PolygonGroup){
        object = std::make_shared <PolygonGroup>();
    }else if (type == SceneObject::SceneObjectType::PointGroup){
        object = std::make_shared <PointGroup>();
    }else{
        return;
    }

    object->setName(name);

    mpModel->append(object);
}

void SceneObjectsListController::removeObject(int index)
{
    mpModel->remove(index);
}

void SceneObjectsListController::setObjectName(int index, QString name)
{
    auto object = mpModel->get(index);

    if (!object)
        return;

    object->setName(name);

    mpModel->replace(index, object);
}

void SceneObjectsListController::setCurrentObject(int index)
{
    auto object = mpModel->get(index);

    mpActiveObjectProvider->setObject(object);
}
