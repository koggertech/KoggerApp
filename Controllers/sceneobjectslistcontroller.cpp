#include "sceneobjectslistcontroller.h"

#include <bottomtrack.h>
#include <surface.h>
#include <pointset.h>

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

void SceneObjectsListController::addObject(QString name, QString type)
{
    std::shared_ptr <VertexObject> object;

    if(type == "Bottom track"){
        object = std::make_shared <BottomTrack>();
        object->setData(
                    mpBottomTrackProvider->getBottomTrack()
                );
    }else if (type == "Surface"){
        object = std::make_shared <Surface>();
    }else if(type == "Point set"){
        object = std::make_shared <PointSet>();
    }else{
        return;
    }

    object->setName(name);
    object->setType(type);


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