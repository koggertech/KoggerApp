#include "activeobjectprovider.h"

#include <bottomtrack.h>

ActiveObjectProvider::ActiveObjectProvider(QObject *parent)
    : QObject{parent}
{}

ActiveObjectProvider::ActiveObjectProvider(std::shared_ptr<VertexObject> object, QObject *parent)
: QObject{parent}
, mp_object(object)
{}

void ActiveObjectProvider::setObject(std::shared_ptr <VertexObject> object)
{
    if(mp_object == object)
        return;

    mp_object = object;

    auto bt = dynamic_cast <BottomTrack*>(mp_object.get());

    if(bt){
        auto filter = bt->filter();

        if (filter){
            qDebug() << "Filter type: " << filter->type();
        }
    }


    Q_EMIT activeObjectChanged(mp_object.get());
}

VertexObject *ActiveObjectProvider::rawActiveObject() const
{
    return mp_object.get();
}

std::shared_ptr<VertexObject> ActiveObjectProvider::activeObject() const
{
    return mp_object;
}
