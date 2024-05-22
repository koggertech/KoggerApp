#include "FileReaderWrapper.h"

#include <QDebug>


FileReaderWrapper::FileReaderWrapper(QObject* parent) : QObject(parent)
{
    workerThread_ = std::make_unique<QThread>();
    workerObject_ = std::make_shared<FileReader>(this);



    workerObject_->moveToThread(workerThread_.get());
    workerThread_->start();
}

FileReaderWrapper::~FileReaderWrapper()
{
    if (workerThread_ && workerThread_->isRunning()) {
        workerThread_->quit();
        workerThread_->wait();
        workerThread_.reset();
    }
}
