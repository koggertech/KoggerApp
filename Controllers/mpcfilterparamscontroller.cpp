#include "mpcfilterparamscontroller.h"

#include <bottomtrack.h>
#include <maxpointsfilter.h>

MPCFilterParamsController::MPCFilterParamsController(std::shared_ptr <ActiveObjectProvider> activeObjectProvider,
                                                     std::shared_ptr <BottomTrackProvider> bottomTrackProvider,
                                                     QObject *parent)
    : QObject(parent)
    , mActiveObjectProvider(activeObjectProvider)
    , mBottomTrackProvider(bottomTrackProvider)
{}

MPCFilterParamsController::~MPCFilterParamsController()
{

}

void MPCFilterParamsController::setMaxPointsCount(int count)
{
    auto object = mActiveObjectProvider->activeObject();

    auto bottomTrack = qobject_cast <BottomTrack*>(object.get());

    if (!bottomTrack)
        return;

    auto filter = qobject_cast <MaxPointsFilter*>(bottomTrack->filter());

    if(filter)
        filter->setMaxPointsCount(count);

    bottomTrack->clearData();
    bottomTrack->setData(mBottomTrackProvider->getBottomTrack());
}
