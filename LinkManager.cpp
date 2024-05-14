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

Link* LinkManager::getLinkPtr(QUuid uuid)
{
    auto it = hash_.find(uuid);
    if (it != hash_.end()) {
        return &(it.value());
    }
    return nullptr;
}

void LinkManager::open(QUuid uuid)
{
    if (auto linkPtr = getLinkPtr(uuid); linkPtr) {
        // TODO
        linkPtr->openAsSerial();
    }
    else
        qDebug() << "LinkManager::open: link not found";
}

void LinkManager::close(QUuid uuid)
{

}
