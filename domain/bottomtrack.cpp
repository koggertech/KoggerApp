#include "bottomtrack.h"

#include <constants.h>

BottomTrack::BottomTrack(QObject* parent)
    : DisplayedObject(GL_LINE_STRIP, parent)
{

}

BottomTrack::~BottomTrack()
{

}

void BottomTrack::setFilter(std::shared_ptr<AbstractBottomTrackFilter> filter)
{
    if(mpFilter == filter)
        return;

    mpFilter = filter;

    Q_EMIT filterChanged(mpFilter.get());
}

float BottomTrack::routeLength() const
{
    return 0.0f;
}

AbstractBottomTrackFilter *BottomTrack::filter() const
{
    return mpFilter.get();
}

void BottomTrack::setData(const QVector<QVector3D> &data)
{
    QVector <QVector3D> filtered;

    if (mpFilter){
        mpFilter->apply(data,filtered);
        DisplayedObject::setData(filtered);
        return;
    }

    DisplayedObject::setData(data);
}

SceneObject::SceneObjectType BottomTrack::type() const
{
    return SceneObjectType::BottomTrack;
}
