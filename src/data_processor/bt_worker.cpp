#include "bt_worker.h"

#include "data_processor.h"


BtWorker::BtWorker(DataProcessor* ownerDp,
                   Dataset* dataset,
                   QObject* parent)
    : QObject(parent),
      dp_(ownerDp),
      dataset_(dataset),
      bottom_(ownerDp)
{
    bottom_.setDatasetPtr(dataset_);
}

BtWorker::~BtWorker() = default;

void BtWorker::setDatasetPtr(Dataset* ds)
{
    dataset_ = ds;
    bottom_.setDatasetPtr(ds);
}

void BtWorker::setBottomTrackZeroDepth(bool state)
{
    bottom_.setZeroDepth(state);
}

void BtWorker::clearBottomTrack()
{
    bottom_.clear();
}

void BtWorker::bottomTrackProcessing(const DatasetChannel& ch1,
                                     const DatasetChannel& ch2,
                                     const BottomTrackParam& p,
                                     bool manual,
                                     bool redrawAll)
{
    emit bottomTrackStarted();

    bottom_.bottomTrackProcessing(ch1, ch2, p, manual, redrawAll);

    emit bottomTrackFinished();
}
