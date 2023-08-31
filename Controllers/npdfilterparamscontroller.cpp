#include "npdfilterparamscontroller.h"

#include <bottomtrack.h>
#include <nearestpointfilter.h>

NPDFilterParamsController::NPDFilterParamsController(std::shared_ptr<ActiveObjectProvider> activeObjectProvider,
                                                     std::shared_ptr<BottomTrackProvider> bottomTrackProvider,
                                                     QObject *parent)
    : QObject(parent)
    , mActiveObjectProvider(activeObjectProvider)
    , mBottomTrackProvider(bottomTrackProvider)
{

}

NPDFilterParamsController::~NPDFilterParamsController()
{

}

void NPDFilterParamsController::setDistance(float distance)
{
    auto object = mActiveObjectProvider->activeObject();

    auto bottomTrack = qobject_cast <BottomTrack*>(object.get());

    if (!bottomTrack)
        return;

    auto filter = qobject_cast <NearestPointFilter*>(bottomTrack->filter());

    if(filter)
        filter->setDistance(distance);

    bottomTrack->clearData();
    bottomTrack->setData(mBottomTrackProvider->getBottomTrack());
}
