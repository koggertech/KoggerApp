#pragma once


class Dataset;
class DataProcessor;
class SurfaceProcessor
{
public:
    explicit SurfaceProcessor(DataProcessor* parent);
    ~SurfaceProcessor();

    void clear();
    void setDatasetPtr(Dataset* datasetPtr);

private:
    DataProcessor* dataProcessor_;
    Dataset* datasetPtr_;
};
