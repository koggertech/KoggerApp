#include "bottomtrackparamscontroller.h"

#include <QDebug>

#include <constants.h>
#include <bottomtrack.h>
#include <nearestpointfilter.h>
#include <maxpointsfilter.h>

BottomTrackParamsController::BottomTrackParamsController(
        std::shared_ptr<ActiveObjectProvider> selectedObjectModel,
        std::shared_ptr<BottomTrackProvider> bottomTrackProvider,
        QObject *parent)
: QObject(parent)
, mpActiveObjectProvider(selectedObjectModel)
, mpBottomTrackProvider(bottomTrackProvider)
{}

void BottomTrackParamsController::changeBottomTrackVisibility(bool visible)
{
    auto object = mpActiveObjectProvider->activeObject();

    auto bottomTrack = dynamic_cast <BottomTrack*>(object.get());

    bottomTrack->setVisible(visible);
}

void BottomTrackParamsController::changeBottomTrackFilter(QString filterType)
{
}

void BottomTrackParamsController::changeBottomTrackFilter(int filterType)
{
    auto activeObject = mpActiveObjectProvider->activeObject();

    auto bottomTrack = dynamic_cast <BottomTrack*> (activeObject.get());

    if (!bottomTrack)
        return;

    std::shared_ptr <AbstractBottomTrackFilter> filter;

    switch(filterType){
    case AbstractBottomTrackFilter::NearestPointDistance:
        filter = std::make_shared <NearestPointFilter>();
        break;
    case AbstractBottomTrackFilter::MaxPointsCount:
        filter = std::make_shared <MaxPointsFilter>();
        break;
    }

    bottomTrack->setFilter(filter);
}

void BottomTrackParamsController::changeBottomTrackFiltrationMethod(QString method)
{

}

void BottomTrackParamsController::changeNearestPointFiltrationRange(float range)
{

}

void BottomTrackParamsController::changeMaxPointsFiltrationCount(int count)
{

}
