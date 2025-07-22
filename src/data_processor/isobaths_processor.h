#pragma once

#include <QVector>
#include <QVector3D>
#include <QDateTime>
#include "dataset.h"


class DataProcessor;
class IsobathsProcessor
{
public:
    explicit IsobathsProcessor(DataProcessor* parent);
    ~IsobathsProcessor();

    void setDatasetPtr(Dataset* datasetPtr);

signals:

public slots:
    void clear();

    void onBottomTrackAdded(const QVector<int>& indxs);

private:
    DataProcessor* dataProcessor_;
    Dataset* datasetPtr_ = nullptr;
};
