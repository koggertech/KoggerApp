#include "mpcfilterparamscontroller.h"

#include <bottomtrack.h>
#include <maxpointsfilter.h>

MPCFilterParamsController::MPCFilterParamsController(std::shared_ptr <ActiveObjectProvider> activeObjectProvider,
                                                     std::shared_ptr <BottomTrackProvider> bottomTrackProvider,
                                                     QObject *parent)
    : QObject(parent)
    , mActiveObjectProvider(activeObjectProvider)
    , mBottomTrackProvider(bottomTrackProvider)
{

}

void MPCFilterParamsController::setMaxPointsCount(int count)
{
    auto object = mActiveObjectProvider->activeObject();

    auto bottomTrack = dynamic_cast <BottomTrack*>(object.get());

    if (!bottomTrack)
        return;

    auto filter = dynamic_cast <MaxPointsFilter*>(bottomTrack->filter());

    if(filter)
        filter->setMaxPointsCount(count);

    bottomTrack->clearData();
    bottomTrack->setData(mBottomTrackProvider->getBottomTrack());
}
