#include "activeobjectprovider.h"

#include <bottomtrack.h>

ActiveObjectProvider::ActiveObjectProvider(QObject *parent)
    : QObject{parent}
{}

ActiveObjectProvider::ActiveObjectProvider(std::shared_ptr<SceneObject> object, QObject *parent)
: QObject{parent}
, mp_object(object)
{}

void ActiveObjectProvider::setObject(std::shared_ptr <SceneObject> object)
{
    if(mp_object == object)
        return;

    mp_object = object;

    Q_EMIT activeObjectChanged(mp_object.get());
}

SceneObject *ActiveObjectProvider::rawActiveObject() const
{
    return mp_object.get();
}

std::shared_ptr<SceneObject> ActiveObjectProvider::activeObject() const
{
    return mp_object;
}
