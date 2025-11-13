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

    void bottomTrackProcessing(const DatasetChannel& channel1, const DatasetChannel& channel2, const BottomTrackParam& bottomTrackParam_, bool manual, bool redrawAll); // external calling not realtime

private:
    bool canceled() const noexcept;

private:
    DataProcessor* dataProcessor_;
    Dataset* datasetPtr_;
};
