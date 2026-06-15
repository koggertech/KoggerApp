#pragma once

#include <QObject>

#include "bottom_track_processor.h"
#include "dataset_defs.h"


class Dataset;
class DataProcessor;

class BtWorker : public QObject
{
    Q_OBJECT

public:
    explicit BtWorker(DataProcessor* ownerDp,
                      Dataset* dataset,
                      QObject* parent = nullptr);
    ~BtWorker() override;

public slots:
    // settings
    void setDatasetPtr(Dataset* ds);
    void setBottomTrackZeroDepth(bool state);
    void clearBottomTrack();

    // task
    void bottomTrackProcessing(const DatasetChannel& ch1,
                               const DatasetChannel& ch2,
                               const BottomTrackParam& p,
                               bool manual,
                               bool redrawAll);

signals:
    void bottomTrackStarted();
    void bottomTrackFinished();

private:
    DataProcessor*       dp_;
    Dataset*             dataset_;
    BottomTrackProcessor bottom_;
};
