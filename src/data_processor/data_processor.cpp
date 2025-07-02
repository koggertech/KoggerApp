#include "data_processor.h"

#include <QDebug>
#include <QThread>


DataProcessor::DataProcessor(QObject *parent) :
    QObject(parent)
{
    //qDebug() << "DataProcessor ctr" << QThread::currentThreadId();
}

DataProcessor::~DataProcessor()
{
    //qDebug() << "DataProcessor dtr" << QThread::currentThreadId();
}

void DataProcessor::init()
{
    qDebug() << "DataProcessor init" << QThread::currentThreadId();
}

void DataProcessor::doAction()
{
    qDebug() << "DataProcessor doAction" << QThread::currentThreadId();

    emit finished();
}
