#include "LinkManager.h"

#include <QDebug>


LinkManager::LinkManager()
{
    workerThread_ = std::make_unique<QThread>();
    workerObject_ = std::make_unique<LinkManagerWorker>(&hash_, &model_, this);

    QObject::connect(workerThread_.get(), &QThread::started, workerObject_.get(), &LinkManagerWorker::onExpiredTimer);
    QObject::connect(workerObject_.get(), &LinkManagerWorker::dataUpdated, this, &LinkManager::stateChanged);

    workerObject_->moveToThread(workerThread_.get());
    workerThread_->start();
}

QHash<QUuid, Link> LinkManager::getHash() const
{
    return hash_;    
}

LinkListModel* LinkManager::getModelPtr()
{
    return &model_;
}


