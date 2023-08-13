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

void NPDFilterParamsController::setDistance(float distance)
{
    auto object = mActiveObjectProvider->activeObject();

    auto bottomTrack = dynamic_cast <BottomTrack*>(object.get());

    if (!bottomTrack)
        return;

    auto filter = dynamic_cast <NearestPointFilter*>(bottomTrack->filter());

    if(filter)
        filter->setDistance(distance);

    bottomTrack->clearData();
    bottomTrack->setData(mBottomTrackProvider->getBottomTrack());
}
