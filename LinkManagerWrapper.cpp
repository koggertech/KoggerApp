#include "LinkManagerWrapper.h"

#include <QDebug>


LinkManagerWrapper::LinkManagerWrapper(QObject* parent) : QObject(parent)
{
    workerThread_ = std::make_unique<QThread>(this);
    workerObject_ = std::make_unique<LinkManager>(nullptr);

    workerObject_->moveToThread(workerThread_.get());

    QObject::connect(workerThread_.get(), &QThread::started,                              workerObject_.get(), &LinkManager::importPinnedLinksFromXML,     Qt::QueuedConnection);
    QObject::connect(this,                &LinkManagerWrapper::sendStopTimer,             workerObject_.get(), &LinkManager::stopTimer,                    Qt::QueuedConnection);
    QObject::connect(workerObject_.get(), &LinkManager::appendModifyModel,                this,                &LinkManagerWrapper::appendModifyModelData, Qt::QueuedConnection);
    QObject::connect(workerObject_.get(), &LinkManager::deleteModel,                      this,                &LinkManagerWrapper::deleteModelData,       Qt::QueuedConnection);
    QObject::connect(this,                &LinkManagerWrapper::sendOpenAsSerial,          workerObject_.get(), &LinkManager::openAsSerial,                 Qt::QueuedConnection);
    QObject::connect(this,                &LinkManagerWrapper::sendCreateAsUdp,           workerObject_.get(), &LinkManager::createAsUdp,                  Qt::QueuedConnection);
    QObject::connect(this,                &LinkManagerWrapper::sendOpenAsUdp,             workerObject_.get(), &LinkManager::openAsUdp,                    Qt::QueuedConnection);
    QObject::connect(this,                &LinkManagerWrapper::sendCreateAsTcp,           workerObject_.get(), &LinkManager::createAsTcp,                  Qt::QueuedConnection);
    QObject::connect(this,                &LinkManagerWrapper::sendOpenAsTcp,             workerObject_.get(), &LinkManager::openAsTcp,                    Qt::QueuedConnection);
    QObject::connect(this,                &LinkManagerWrapper::sendCloseLink,             workerObject_.get(), &LinkManager::closeLink,                    Qt::QueuedConnection);
    QObject::connect(this,                &LinkManagerWrapper::sendDeleteLink,            workerObject_.get(), &LinkManager::deleteLink,                   Qt::QueuedConnection);
    QObject::connect(this,                &LinkManagerWrapper::sendUpdateBaudrate,        workerObject_.get(), &LinkManager::updateBaudrate,               Qt::QueuedConnection);
    QObject::connect(this,                &LinkManagerWrapper::sendUpdateAddress,         workerObject_.get(), &LinkManager::updateAddress,                Qt::QueuedConnection);
    QObject::connect(this,                &LinkManagerWrapper::sendUpdateSourcePort,      workerObject_.get(), &LinkManager::updateSourcePort,             Qt::QueuedConnection);
    QObject::connect(this,                &LinkManagerWrapper::sendUpdateDestinationPort, workerObject_.get(), &LinkManager::updateDestinationPort,        Qt::QueuedConnection);
    QObject::connect(this,                &LinkManagerWrapper::sendUpdatePinnedState,     workerObject_.get(), &LinkManager::updatePinnedState,            Qt::QueuedConnection);

    QObject::connect(this,                &LinkManagerWrapper::sendUpdateControlType,     this,                 [this](QUuid uuid, int controlType) {
        QMetaObject::invokeMethod(workerObject_.get(), [this, uuid, controlType]() {
                switch (controlType) {
                    case 0: workerObject_->updateControlType(uuid, ControlType::kManual);   break;
                    case 1: workerObject_->updateControlType(uuid, ControlType::kAuto);     break;
                    case 2: workerObject_->updateControlType(uuid, ControlType::kAutoOnce); break;
                    default: break;
                }
            }, Qt::QueuedConnection);
    });

    workerThread_->start();
}

LinkManagerWrapper::~LinkManagerWrapper()
{
    if (workerThread_ && workerThread_->isRunning()) {
        workerThread_->quit();
        workerThread_->wait();
    }

    workerThread_->deleteLater();
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
