#include "LinkManagerWrapper.h"

#include <QDebug>


LinkManagerWrapper::LinkManagerWrapper(QObject* parent) : QObject(parent)
{
    workerThread_ = std::make_unique<QThread>(/**/);
    workerObject_ = std::make_unique<LinkManager>(this);

    QObject::connect(workerThread_.get(), &QThread::started, workerObject_.get(), &LinkManager::onExpiredTimer);

    // this -> thread
    QObject::connect(this, &LinkManagerWrapper::sendOpenAsSerial, workerObject_.get(), &LinkManager::openAsSerial);
    QObject::connect(this, &LinkManagerWrapper::sendOpenAsUdp, workerObject_.get(), &LinkManager::openAsUdp);
    QObject::connect(this, &LinkManagerWrapper::sendOpenAsTcp, workerObject_.get(), &LinkManager::openAsTcp);
    QObject::connect(this, &LinkManagerWrapper::sendClose, workerObject_.get(), &LinkManager::close);

    // thread -> this
    QObject::connect(workerObject_.get(), &LinkManager::appendModifyModel, this, &LinkManagerWrapper::appendModifyModelData);
    QObject::connect(workerObject_.get(), &LinkManager::deleteModel, this, &LinkManagerWrapper::deleteModelData);

    workerObject_->moveToThread(workerThread_.get());
    workerThread_->start();
}

LinkManagerWrapper::~LinkManagerWrapper()
{
    if (workerThread_ && workerThread_->isRunning()) {
        workerThread_->quit();
        workerThread_->wait();
        workerThread_.reset();
    }
}

LinkListModel* LinkManagerWrapper::getModelPtr()
{
    return &model_;
}

void LinkManagerWrapper::openAsSerial(QUuid uuid)
{
    emit sendOpenAsSerial(uuid);
}

void LinkManagerWrapper::openAsUdp(QUuid uuid)
{
    emit sendOpenAsUdp(uuid);
}

void LinkManagerWrapper::openAsTcp(QUuid uuid)
{
    emit sendOpenAsTcp(uuid);
}

void LinkManagerWrapper::close(QUuid uuid)
{
    emit sendClose(uuid);
}

void LinkManagerWrapper::appendModifyModelData(QUuid uuid, bool connectionStatus, ControlType controlType, QString portName, int baudrate, bool parity,
                                  LinkType linkType, QString address, int sourcePort, int destinationPort, bool isPinned, bool isHided, bool isNotAvailable)
{
    emit model_.appendModifyEvent(uuid,
                                  connectionStatus,
                                  controlType,
                                  portName,
                                  baudrate,
                                  parity,
                                  linkType,
                                  address,
                                  sourcePort,
                                  destinationPort,
                                  isPinned,
                                  isHided,
                                  isNotAvailable);
}

void LinkManagerWrapper::deleteModelData(QUuid uuid)
{
    emit model_.removeEvent(uuid);
}
