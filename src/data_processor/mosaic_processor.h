#pragma once


class Dataset;
class DataProcessor;
class MosaicProcessor
{
public:
    explicit MosaicProcessor(DataProcessor* parent);
    ~MosaicProcessor();

    void clear();
    void setDatasetPtr(Dataset* datasetPtr);

private:
    DataProcessor* dataProcessor_;
    Dataset* datasetPtr_ = nullptr;
};
