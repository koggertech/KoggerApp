#pragma once

#include <QVector>
#include <QVector3D>
#include <QDateTime>
#include "dataset_defs.h"


class Dataset;
class DataProcessor;
class BottomTrackProcessor
{
public:
    explicit BottomTrackProcessor(DataProcessor* parent);
    ~BottomTrackProcessor();

    void clear();
    void setDatasetPtr(Dataset* datasetPtr);

    void bottomTrackProcessing(const ChannelId& channel1, const ChannelId& channel2, const BottomTrackParam& bottomTrackParam_); // external calling not realtime

private:
    DataProcessor* dataProcessor_;
    Dataset* datasetPtr_;
};
