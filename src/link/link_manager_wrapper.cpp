#include "link_manager_wrapper.h"

#include <QDebug>


LinkManagerWrapper::LinkManagerWrapper(QObject* parent) : QObject(parent)
{
    workerThread_ = std::make_unique<QThread>(this);
    workerObject_ = std::make_unique<LinkManager>(nullptr);

    auto connectionType = Qt::AutoConnection;
    QObject::connect(workerThread_.get(), &QThread::started,                                workerObject_.get(), &LinkManager::createAndStartTimer,          connectionType);
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
    QObject::connect(this,                &LinkManagerWrapper::sendSetRequestToSend,        workerObject_.get(), &LinkManager::setRequestToSend,             connectionType);
    QObject::connect(this,                &LinkManagerWrapper::sendSetDataTerminalReady,    workerObject_.get(), &LinkManager::setDataTerminalReady,         connectionType);
    QObject::connect(this,                &LinkManagerWrapper::sendSetPatity,               workerObject_.get(), &LinkManager::setParity,                    connectionType);
    QObject::connect(this,                &LinkManagerWrapper::sendSetAttribut,             workerObject_.get(), &LinkManager::setAttribute,                 connectionType);
    QObject::connect(this,                &LinkManagerWrapper::sendUpdateAddress,           workerObject_.get(), &LinkManager::updateAddress,                connectionType);
    QObject::connect(this,                &LinkManagerWrapper::sendUpdateSourcePort,        workerObject_.get(), &LinkManager::updateSourcePort,             connectionType);
    QObject::connect(this,                &LinkManagerWrapper::sendUpdateDestinationPort,   workerObject_.get(), &LinkManager::updateDestinationPort,        connectionType);
    QObject::connect(this,                &LinkManagerWrapper::sendUpdatePinnedState,       workerObject_.get(), &LinkManager::updatePinnedState,            connectionType);
    QObject::connect(this,                &LinkManagerWrapper::sendCreateAndOpenAsUdpProxy, workerObject_.get(), &LinkManager::createAndOpenAsUdpProxy,      connectionType);
    QObject::connect(this,                &LinkManagerWrapper::sendCloseUdpProxy,           workerObject_.get(), &LinkManager::closeUdpProxy,                connectionType);
    QObject::connect(this,                &LinkManagerWrapper::sendAutoSpeedSelection,      workerObject_.get(), &LinkManager::updateAutoSpeedSelection,     connectionType);
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
    workerThread_->setObjectName("LinkThread");
}

LinkManagerWrapper::~LinkManagerWrapper()
{
    shutdownWorkerThread();
}

LinkListModel* LinkManagerWrapper::getModelPtr()
{
    return &model_;
}

LinkManager* LinkManagerWrapper::getWorker()
{
    return workerObject_.get();
}

void LinkManagerWrapper::startWorkerThread()
{
    if (workerThread_ && !workerThread_->isRunning()) {
        workerThread_->start();
    }
}

void LinkManagerWrapper::shutdownWorkerThread()
{
    if (!workerThread_) {
        workerObject_.reset();
        return;
    }

    if (workerObject_) {
        if (workerThread_->isRunning()) {
            LinkManager* managerToDelete = workerObject_.release();
            QMetaObject::invokeMethod(
                managerToDelete,
                [managerToDelete]() {
                    managerToDelete->shutdown();
                    delete managerToDelete;
                },
                Qt::BlockingQueuedConnection);
        } else {
            workerObject_->shutdown();
            workerObject_.reset();
        }
    }

    if (workerThread_->isRunning()) {
        workerThread_->quit();
        workerThread_->wait();
    }

    workerThread_.reset();
}

void LinkManagerWrapper::closeOpenedLinks()
{
    for (auto& itm : model_.getOpenedUuids()) {
        emit sendFCloseLink(itm.first);
    }
}

QHash<QUuid, QString> LinkManagerWrapper::getLinkNames() const
{
    return model_.getLinkNames();
}

void LinkManagerWrapper::openClosedLinks()
{
    emit sendOpenFLinks();
}

bool LinkManagerWrapper::reloadPinnedLinksFromXmlData(const QByteArray& xmlData,
                                                       bool allowSerialLinks,
                                                       int* skippedSerialLinks,
                                                       QString* error)
{
    if (!workerObject_) {
        if (error) {
            *error = QStringLiteral("Link worker is not available");
        }
        return false;
    }

    if (workerThread_ && !workerThread_->isRunning()) {
        workerThread_->start();
    }

    if (QThread::currentThread() == workerObject_->thread()) {
        return workerObject_->reloadPinnedLinksFromXmlData(xmlData, allowSerialLinks, skippedSerialLinks, error);
    }

    bool ok = false;
    int localSkippedSerialLinks = 0;
    QString localError;
    QMetaObject::invokeMethod(workerObject_.get(), [this, &ok, &localSkippedSerialLinks, &localError, xmlData, allowSerialLinks]() {
        ok = workerObject_->reloadPinnedLinksFromXmlData(xmlData,
                                                         allowSerialLinks,
                                                         &localSkippedSerialLinks,
                                                         &localError);
    }, Qt::BlockingQueuedConnection);

    if (skippedSerialLinks) {
        *skippedSerialLinks = localSkippedSerialLinks;
    }
    if (error) {
        *error = localError;
    }
    return ok;
}

QVariant LinkManagerWrapper::baudrateModel() const
{
    QVariantList list;
    for (const uint32_t rate : baudrates) {
        list.append(QVariant::fromValue(rate));
    }
    return list;
}

void LinkManagerWrapper::openAsSerial(QUuid uuid, LinkAttribute attribute)
{
    emit sendOpenAsSerial(uuid, attribute);
}

void LinkManagerWrapper::createAsUdp(QString address, int sourcePort, int destinationPort)
{
    emit sendCreateAsUdp(address, sourcePort, destinationPort);
}

void LinkManagerWrapper::openAsUdp(QUuid uuid, QString address, int sourcePort, int destinationPort, LinkAttribute attribute)
{
    emit sendOpenAsUdp(uuid, address, sourcePort, destinationPort, attribute);
}

void LinkManagerWrapper::createAsTcp(QString address, int sourcePort, int destinationPort)
{
    emit sendCreateAsTcp(address, sourcePort, destinationPort);
}

void LinkManagerWrapper::openAsTcp(QUuid uuid, QString address, int sourcePort, int destinationPort, LinkAttribute attribute)
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

void LinkManagerWrapper::setRequestToSend(QUuid uuid, bool rts) {
    emit sendSetRequestToSend(uuid, rts);
}

void LinkManagerWrapper::setDataTerminalReady(QUuid uuid, bool dtr) {
    emit sendSetDataTerminalReady(uuid, dtr);
}

void LinkManagerWrapper::setParity(QUuid uuid, bool parity) {
    emit sendSetPatity(uuid, parity);
}

void LinkManagerWrapper::setAttribute(QUuid uuid, LinkAttribute attribute) {
    emit sendSetAttribut(uuid, attribute);
}

void LinkManagerWrapper::appendModifyModelData(QUuid uuid, bool connectionStatus, bool receivesData, ControlType controlType, QString portName,
                                               int baudrate, bool parity, LinkType linkType, QString address, int sourcePort, int destinationPort,
                                               bool isPinned, bool isHided, bool isNotAvailable, bool autoSpeedSelection, bool isUpgradingState)
{
    emit model_.appendModifyEvent(uuid, connectionStatus, receivesData, controlType, portName, baudrate, parity,
                                  linkType, address, sourcePort, destinationPort, isPinned, isHided, isNotAvailable, autoSpeedSelection, isUpgradingState);
}

void LinkManagerWrapper::deleteModelData(QUuid uuid)
{
    emit model_.removeEvent(uuid);
}
