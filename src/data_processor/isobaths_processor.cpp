#include "isobaths_processor.h"

#include <QDebug>
#include <QThread>
#include "data_processor.h"


IsobathsProcessor::IsobathsProcessor(DataProcessor* parent) :
    dataProcessor_(parent)
{
}

IsobathsProcessor::~IsobathsProcessor()
{
}

void IsobathsProcessor::clear()
{

}

void IsobathsProcessor::onBottomTrackAdded(const QVector<int> &indxs)
{
    // qDebug() << "IsobathsProcessor::onBottomTrackAdded";
    // qDebug() << indxs;
}

void IsobathsProcessor::setDatasetPtr(Dataset *datasetPtr)
{
    datasetPtr_ = datasetPtr;
}
