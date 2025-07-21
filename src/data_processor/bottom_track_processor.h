#pragma once

#include <QVector>
#include <QVector3D>
#include <QDateTime>
#include "dataset.h"


class DataProcessor;
class BottomTrackProcessor
{
public:
    explicit BottomTrackProcessor(DataProcessor* parent);
    ~BottomTrackProcessor();

    void setDatasetPtr(Dataset* datasetPtr);

signals:

public slots:
    void clear();

    void bottomTrackProcessing(const ChannelId& channel1, const ChannelId& channel2, const BottomTrackParam& bottomTrackParam_); // external calling not realtime

private:
    DataProcessor* dataProcessor_;
    Dataset* datasetPtr_ = nullptr;
};
