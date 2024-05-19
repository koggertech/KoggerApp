#include "LinkManager.h"

#include <QDebug>


LinkManager::LinkManager(QObject *parent) :
    QObject(parent)
{
    timer_ = std::make_unique<QTimer>(this);
    timer_->setInterval(timerInterval_);

    QObject::connect(timer_.get(), &QTimer::timeout, this, &LinkManager::onExpiredTimer);
}

LinkManager::~LinkManager()
{
    timer_->stop();
}

QList<QSerialPortInfo> LinkManager::getCurrentSerialList() const
{
    return QSerialPortInfo::availablePorts();
}

Link* LinkManager::createSerialPort(const QSerialPortInfo &serialInfo) const
{
    Link* newLinkPtr = nullptr;

    if (serialInfo.isNull())
        return newLinkPtr;

    newLinkPtr = new Link();
    newLinkPtr->createAsSerial(serialInfo.portName(), 921600, false);

    QObject::connect(newLinkPtr, &Link::connectionStatusChanged, this, &LinkManager::onLinkConnectionStatusChanged);
    QObject::connect(newLinkPtr, &Link::frameReady, this, &LinkManager::frameReady);
    QObject::connect(newLinkPtr, &Link::closed, this, &LinkManager::linkClosed);

    // connect(this, &LinkManagerWorker::frameInput, newLink, &Link::writeFrame);

    return newLinkPtr;
}

void LinkManager::addNewLinks(const QList<QSerialPortInfo> &currSerialList)
{
    for (const auto& itmI : currSerialList) {
        bool isBeen{ false };

        for (auto& itmJ : list_) {
            if (itmJ->getLinkType() != LinkType::LinkSerial)
                continue;

            if (itmI.portName() == itmJ->getPortName()) {
                isBeen = true;
                break;
            }
        }

        if (!isBeen) {
            auto link = createSerialPort(itmI);
            qDebug() << "link created: " << link->getUuid();

            // list
            list_.append(link);
            // model
            emit appendModifyModel(link->getUuid(),
                                   link->getConnectionStatus(),
                                   link->getControlType(),
                                   link->getPortName(),
                                   link->getBaudrate(),
                                   link->getParity(),
                                   link->getLinkType(),
                                   link->getAddress(),
                                   link->getSourcePort(),
                                   link->getDestinationPort(),
                                   link->isPinned(),
                                   link->isHided(),
                                   link->isNotAvailable());
        }
    }
}

void LinkManager::deleteMissingLinks(const QList<QSerialPortInfo> &currSerialList)
{
    for (int i = 0; i < list_.size(); ++i) {
        Link* link = list_.at(i);

        if (link->getLinkType() != LinkType::LinkSerial)
            continue;

        bool isBeen{ false };
        for (const auto& itm : currSerialList) {
            if (itm.portName() == link->getPortName()) {
                isBeen = true;
                break;
            }
        }

        if (!isBeen) {
            emit linkDeleted(link);

            // model
            emit deleteModel(link->getUuid());
            // list
            link->disconnect();
            this->disconnect(link);

            if (link->isOpen())
                link->close();
            qDebug() << "link deleted: " << link->getUuid();
            delete link;
            list_.removeAt(i);
            --i;
        }
    }
}

void LinkManager::update()
{
    auto currSerialList{ getCurrentSerialList() };

    addNewLinks(currSerialList);
    deleteMissingLinks(currSerialList);
}

Link *LinkManager::getLinkPtr(QUuid uuid)
{
    Link* retVal{ nullptr };

    for (auto& itm : list_) {
        if (itm->getUuid() == uuid) {
            retVal = itm;
            break;
        }
    }

    return retVal;
}

void LinkManager::onLinkConnectionStatusChanged(QUuid uuid)
{
    if (const auto linkPtr = getLinkPtr(uuid); linkPtr) {
        emit appendModifyModel(linkPtr->getUuid(),
                               linkPtr->getConnectionStatus(),
                               linkPtr->getControlType(),
                               linkPtr->getPortName(),
                               linkPtr->getBaudrate(),
                               linkPtr->getParity(),
                               linkPtr->getLinkType(),
                               linkPtr->getAddress(),
                               linkPtr->getSourcePort(),
                               linkPtr->getDestinationPort(),
                               linkPtr->isPinned(),
                               linkPtr->isHided(),
                               linkPtr->isNotAvailable());
    }
}

void LinkManager::onExpiredTimer()
{
    update();
    timer_->start();
}

void LinkManager::openAsSerial(QUuid uuid)
{
    timer_->stop();

    if (const auto linkPtr = getLinkPtr(uuid); linkPtr)
        linkPtr->openAsSerial();

    timer_->start();
}

void LinkManager::openAsUdp(QUuid uuid, QString address, int sourcePort, int destinationPort)
{
    timer_->stop();

    if (const auto linkPtr = getLinkPtr(uuid); linkPtr) {
        linkPtr->updateUdpParameters(address, sourcePort, destinationPort);
        linkPtr->openAsUdp();
    }

    timer_->start();
}

void LinkManager::openAsTcp(QUuid uuid, QString address, int sourcePort, int destinationPort)
{
    timer_->stop();

    if (const auto linkPtr = getLinkPtr(uuid); linkPtr) {
        linkPtr->updateTcpParameters(address, sourcePort, destinationPort);
        linkPtr->openAsTcp();
    }

    timer_->start();
}

void LinkManager::closeLink(QUuid uuid)
{
    timer_->stop();

    if (const auto linkPtr = getLinkPtr(uuid); linkPtr)
        linkPtr->close();

    timer_->start();
}

void LinkManager::deleteLink(QUuid uuid)
{
    timer_->stop();

    if (const auto linkPtr = getLinkPtr(uuid); linkPtr) {
        emit linkDeleted(linkPtr);

        emit deleteModel(linkPtr->getUuid());
        linkPtr->disconnect();
        this->disconnect(linkPtr);

        if (linkPtr->isOpen())
            linkPtr->close();

        qDebug() << "link deleted: " << linkPtr->getUuid();

        delete linkPtr;
    }

    timer_->start();
}

void LinkManager::frameInput(Link *link, FrameParser frame)
{
    // TODO
    Q_UNUSED(link);
    Q_UNUSED(frame);
}

void LinkManager::createAsUdp(QString address, int sourcePort, int destinationPort)
{
    Link* newLinkPtr = new Link();

    newLinkPtr->createAsUdp(address, sourcePort, destinationPort);

    QObject::connect(newLinkPtr, &Link::connectionStatusChanged, this, &LinkManager::onLinkConnectionStatusChanged);
    QObject::connect(newLinkPtr, &Link::frameReady, this, &LinkManager::frameReady);
    QObject::connect(newLinkPtr, &Link::closed, this, &LinkManager::linkClosed);

    list_.append(newLinkPtr);

    emit appendModifyModel(newLinkPtr->getUuid(),
                           newLinkPtr->getConnectionStatus(),
                           newLinkPtr->getControlType(),
                           newLinkPtr->getPortName(),
                           newLinkPtr->getBaudrate(),
                           newLinkPtr->getParity(),
                           newLinkPtr->getLinkType(),
                           newLinkPtr->getAddress(),
                           newLinkPtr->getSourcePort(),
                           newLinkPtr->getDestinationPort(),
                           newLinkPtr->isPinned(),
                           newLinkPtr->isHided(),
                           newLinkPtr->isNotAvailable());
}

void LinkManager::createAsTcp(QString address, int sourcePort, int destinationPort)
{
    Link* newLinkPtr = new Link();

    newLinkPtr->createAsTcp(address, sourcePort, destinationPort);

    QObject::connect(newLinkPtr, &Link::connectionStatusChanged, this, &LinkManager::onLinkConnectionStatusChanged);
    QObject::connect(newLinkPtr, &Link::frameReady, this, &LinkManager::frameReady);
    QObject::connect(newLinkPtr, &Link::closed, this, &LinkManager::linkClosed);

    list_.append(newLinkPtr);

    emit appendModifyModel(newLinkPtr->getUuid(),
                           newLinkPtr->getConnectionStatus(),
                           newLinkPtr->getControlType(),
                           newLinkPtr->getPortName(),
                           newLinkPtr->getBaudrate(),
                           newLinkPtr->getParity(),
                           newLinkPtr->getLinkType(),
                           newLinkPtr->getAddress(),
                           newLinkPtr->getSourcePort(),
                           newLinkPtr->getDestinationPort(),
                           newLinkPtr->isPinned(),
                           newLinkPtr->isHided(),
                           newLinkPtr->isNotAvailable());
}
