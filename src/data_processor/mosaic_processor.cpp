#include "mosaic_processor.h"

#include <QDebug>
#include "data_processor.h"
#include "dataset.h"


MosaicProcessor::MosaicProcessor(DataProcessor* parent) :
    dataProcessor_(parent),
    datasetPtr_(nullptr)
{
}

MosaicProcessor::~MosaicProcessor()
{
}

void MosaicProcessor::clear()
{

}

void MosaicProcessor::setDatasetPtr(Dataset *datasetPtr)
{
    datasetPtr_ = datasetPtr;
}
