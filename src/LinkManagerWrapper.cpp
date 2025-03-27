#include "LinkManagerWrapper.h"

#include <QDebug>


LinkManagerWrapper::LinkManagerWrapper(QObject* parent) : QObject(parent)
{
    workerThread_ = std::make_unique<QThread>(this);
    workerObject_ = std::make_unique<LinkManager>(nullptr);

    auto connectionType = Qt::AutoConnection;
    QObject::connect(workerThread_.get(), &QThread::started,                                workerObject_.get(), &LinkManager::createAndStartTimer,          connectionType);
    QObject::connect(this,                &LinkManagerWrapper::sendStopTimer,               workerObject_.get(), &LinkManager::stopTimer,                    connectionType);
    QObject::connect(workerObject_.get(), &LinkManager::appendModifyModel,                  this,                &LinkManagerWrapper::appendModifyModelData, connectionType);
    QObject::connect(workerObject_.get(), &LinkManager::deleteModel,                        this,                &LinkManagerWrapper::deleteModelData,       connectionType);
    QObject::connect(this,                &LinkManagerWrapper::sendOpenAsSerial,            workerObject_.get(), &LinkManager::openAsSerial,                 connectionType);
    QObject::connect(this,                &LinkManagerWrapper::sendCreateAsUdp,             workerObject_.get(), &LinkManager::createAsUdp,                  connectionType);
    QObject::connect(this,                &LinkManagerWrapper::sendOpenAsUdp,               workerObject_.get(), &LinkManager::openAsUdp,                    connectionType);
    QObject::connect(this,                &LinkManagerWrapper::sendCreateAsTcp,             workerObject_.get(), &LinkManager::createAsTcp,                  connectionType);
    QObject::connect(this,                &LinkManagerWrapper::sendOpenAsTcp,               workerObject_.get(), &LinkManager::openAsTcp,                    connectionType);
    QObject::connect(this,                &LinkManagerWrapper::sendCloseLink,               workerObject_.get(), &LinkManager::closeLink,                    connectionType);
    QObject::connect(this,                &LinkManagerWrapper::sendFCloseLink,              workerObject_.get(), &LinkManager::closeFLink,                   connectionType);
    QObject::connect(this,                &LinkManagerWrapper::sendOpenFLinks,              workerObject_.get(), &LinkManager::openFLinks,                   connectionType);
    QObject::connect(this,                &LinkManagerWrapper::sendDeleteLink,              workerObject_.get(), &LinkManager::deleteLink,                   connectionType);
    QObject::connect(this,                &LinkManagerWrapper::sendUpdateBaudrate,          workerObject_.get(), &LinkManager::updateBaudrate,               connectionType);
    QObject::connect(this,                &LinkManagerWrapper::sendUpdateAddress,           workerObject_.get(), &LinkManager::updateAddress,                connectionType);
    QObject::connect(this,                &LinkManagerWrapper::sendUpdateSourcePort,        workerObject_.get(), &LinkManager::updateSourcePort,             connectionType);
    QObject::connect(this,                &LinkManagerWrapper::sendUpdateDestinationPort,   workerObject_.get(), &LinkManager::updateDestinationPort,        connectionType);
    QObject::connect(this,                &LinkManagerWrapper::sendUpdatePinnedState,       workerObject_.get(), &LinkManager::updatePinnedState,            connectionType);
    QObject::connect(this,                &LinkManagerWrapper::sendCreateAndOpenAsUdpProxy, workerObject_.get(), &LinkManager::createAndOpenAsUdpProxy,      connectionType);
    QObject::connect(this,                &LinkManagerWrapper::sendCloseUdpProxy,           workerObject_.get(), &LinkManager::closeUdpProxy,                connectionType);
    QObject::connect(this,                &LinkManagerWrapper::sendUpdateControlType,       this,                [this](QUuid uuid, int controlType) {
        QMetaObject::invokeMethod(workerObject_.get(), [this, uuid, controlType]() {
                switch (controlType) {
                    case 0: workerObject_->updateControlType(uuid, ControlType::kManual);   break;
                    case 1: workerObject_->updateControlType(uuid, ControlType::kAuto);     break;
                    case 2: workerObject_->updateControlType(uuid, ControlType::kAutoOnce); break;
                    default: break;
                }
        }, Qt::QueuedConnection);
    }, connectionType);

    workerObject_->moveToThread(workerThread_.get());
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

void LinkManagerWrapper::closeOpenedLinks()
{
    for (auto& itm : model_.getOpenedUuids()) {
        emit sendFCloseLink(itm.first);
    }
}

void LinkManagerWrapper::openClosedLinks()
{
    emit sendOpenFLinks();
}

void LinkManagerWrapper::openAsSerial(QUuid uuid, int attribute)
{
    emit sendOpenAsSerial(uuid, attribute);
}

// #ifdef MOTOR
// void LinkManagerWrapper::openAsSerial(QUuid uuid, int attribute)
// {
//     emit sendOpenAsSerial(uuid, isMotorDevice);
// }
// #else
// void LinkManagerWrapper::openAsSerial(QUuid uuid)
// {
//     emit sendOpenAsSerial(uuid);
// }
// #endif

void LinkManagerWrapper::createAsUdp(QString address, int sourcePort, int destinationPort)
{
    emit sendCreateAsUdp(address, sourcePort, destinationPort);
}

void LinkManagerWrapper::openAsUdp(QUuid uuid, QString address, int sourcePort, int destinationPort, int attribute)
{
    emit sendOpenAsUdp(uuid, address, sourcePort, destinationPort, attribute);
}

void LinkManagerWrapper::createAsTcp(QString address, int sourcePort, int destinationPort)
{
    emit sendCreateAsTcp(address, sourcePort, destinationPort);
}

void LinkManagerWrapper::openAsTcp(QUuid uuid, QString address, int sourcePort, int destinationPort, int attribute)
{
    emit sendOpenAsTcp(uuid, address, sourcePort, destinationPort, attribute);
}

void LinkManagerWrapper::closeLink(QUuid uuid)
{
    emit sendCloseLink(uuid);
}
void LinkManagerWrapper::closeFLink(QUuid uuid)
{
    emit sendFCloseLink(uuid);
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
