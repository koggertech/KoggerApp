#include "LinkManagerWrapper.h"

#include <QDebug>


LinkManagerWrapper::LinkManagerWrapper(QObject* parent) : QObject(parent)
{
    workerThread_ = std::make_unique<QThread>(this);
    workerObject_ = std::make_unique<LinkManager>(this);

    QObject::connect(workerThread_.get(), &QThread::started,                              workerObject_.get(), &LinkManager::importPinnedLinksFromXML);
    QObject::connect(this,                &LinkManagerWrapper::sendOpenAsSerial,          workerObject_.get(), &LinkManager::openAsSerial);
    QObject::connect(this,                &LinkManagerWrapper::sendCreateAsUdp,           workerObject_.get(), &LinkManager::createAsUdp);
    QObject::connect(this,                &LinkManagerWrapper::sendOpenAsUdp,             workerObject_.get(), &LinkManager::openAsUdp);
    QObject::connect(this,                &LinkManagerWrapper::sendCreateAsTcp,           workerObject_.get(), &LinkManager::createAsTcp);
    QObject::connect(this,                &LinkManagerWrapper::sendOpenAsTcp,             workerObject_.get(), &LinkManager::openAsTcp);
    QObject::connect(this,                &LinkManagerWrapper::sendCloseLink,             workerObject_.get(), &LinkManager::closeLink);
    QObject::connect(this,                &LinkManagerWrapper::sendDeleteLink,            workerObject_.get(), &LinkManager::deleteLink);
    QObject::connect(this,                &LinkManagerWrapper::sendUpdateBaudrate,        workerObject_.get(), &LinkManager::updateBaudrate);
    QObject::connect(this,                &LinkManagerWrapper::sendUpdateAddress,         workerObject_.get(), &LinkManager::updateAddress);
    QObject::connect(this,                &LinkManagerWrapper::sendUpdateSourcePort,      workerObject_.get(), &LinkManager::updateSourcePort);
    QObject::connect(this,                &LinkManagerWrapper::sendUpdateDestinationPort, workerObject_.get(), &LinkManager::updateDestinationPort);
    QObject::connect(this,                &LinkManagerWrapper::sendUpdatePinnedState,     workerObject_.get(), &LinkManager::updatePinnedState);
    QObject::connect(workerObject_.get(), &LinkManager::appendModifyModel,                this,                &LinkManagerWrapper::appendModifyModelData);
    QObject::connect(workerObject_.get(), &LinkManager::deleteModel,                      this,                &LinkManagerWrapper::deleteModelData);

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

LinkManager* LinkManagerWrapper::getWorker()
{
    return workerObject_.get();
}

void LinkManagerWrapper::openAsSerial(QUuid uuid)
{
    emit sendOpenAsSerial(uuid);
}

void LinkManagerWrapper::createAsUdp(QString address, int sourcePort, int destinationPort)
{
    emit sendCreateAsUdp(address, sourcePort, destinationPort);
}

void LinkManagerWrapper::openAsUdp(QUuid uuid, QString address, int sourcePort, int destinationPort)
{
    emit sendOpenAsUdp(uuid, address, sourcePort, destinationPort);
}

void LinkManagerWrapper::createAsTcp(QString address, int sourcePort, int destinationPort)
{
    emit sendCreateAsTcp(address, sourcePort, destinationPort);
}

void LinkManagerWrapper::openAsTcp(QUuid uuid, QString address, int sourcePort, int destinationPort)
{
    emit sendOpenAsTcp(uuid, address, sourcePort, destinationPort);
}

void LinkManagerWrapper::closeLink(QUuid uuid)
{
    emit sendCloseLink(uuid);
}

void LinkManagerWrapper::deleteLink(QUuid uuid)
{
    emit sendDeleteLink(uuid);
}

void LinkManagerWrapper::updateBaudrate(QUuid uuid, int baudrate)
{
    emit sendUpdateBaudrate(uuid, baudrate);
}

void LinkManagerWrapper::appendModifyModelData(QUuid uuid, bool connectionStatus, ControlType controlType, QString portName, int baudrate, bool parity,
                                  LinkType linkType, QString address, int sourcePort, int destinationPort, bool isPinned, bool isHided, bool isNotAvailable)
{
    emit model_.appendModifyEvent(uuid, connectionStatus, controlType, portName, baudrate, parity,
                                  linkType, address, sourcePort, destinationPort, isPinned, isHided, isNotAvailable);
}

void LinkManagerWrapper::deleteModelData(QUuid uuid)
{
    emit model_.removeEvent(uuid);
}
