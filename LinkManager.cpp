#include "LinkManager.h"

#include <QDebug>


LinkManager::LinkManager(QObject* parent) : QObject(parent)
{
    workerThread_ = std::make_unique<QThread>();
    workerObject_ = std::make_unique<LinkManagerWorker>(&list_, &model_, this);

    QObject::connect(workerThread_.get(), &QThread::started, workerObject_.get(), &LinkManagerWorker::onExpiredTimer);
    QObject::connect(workerObject_.get(), &LinkManagerWorker::dataUpdated, this, &LinkManager::stateChanged);

    workerObject_->moveToThread(workerThread_.get());
    workerThread_->start();
}

LinkManager::~LinkManager()
{
    if (workerThread_ && workerThread_->isRunning()) {
        workerThread_->quit();
        workerThread_->wait();
        workerThread_.reset();
    }
}

QList<Link*> LinkManager::getLinkPtrList() const
{
    return list_;
}

LinkListModel* LinkManager::getModelPtr()
{
    return &model_;
}

Link* LinkManager::getLinkPtr(QUuid uuid)
{
    for (auto& itm : list_) {
        if (itm->getUuid() == uuid)
            return itm;
    }

    return nullptr;
}

void LinkManager::openSerial(QUuid uuid)
{
    if (auto linkPtr = getLinkPtr(uuid); linkPtr) {
        // TODO
        linkPtr->openAsSerial();
        emit openedEvent(true);
    }
    else
        qDebug() << "LinkManager::open: link not found";
}

void LinkManager::openUdp(QUuid uuid)
{
    if (auto linkPtr = getLinkPtr(uuid); linkPtr) {
        // TODO
        linkPtr->openAsUdp();
        emit openedEvent(true);

    }
    else
        qDebug() << "LinkManager::open: link not found";
}

void LinkManager::openTcp(QUuid uuid)
{
    if (auto linkPtr = getLinkPtr(uuid); linkPtr) {
        // TODO
        //linkPtr->openAsTcp();
        emit openedEvent(true);
    }
    else
        qDebug() << "LinkManager::open: link not found";
}

void LinkManager::close(QUuid uuid)
{
    if (auto linkPtr = getLinkPtr(uuid); linkPtr) {
        // TODO
        qDebug() << "trying to closing link...";

        linkPtr->close();
        emit openedEvent(false);
    }
    else
        qDebug() << "LinkManager::open: link not found";
}
