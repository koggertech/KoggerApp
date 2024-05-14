#include "LinkManager.h"

#include <QDebug>


LinkManager::LinkManager()
{
    qDebug() << "LinkManager::LinkManager()";
}

LinkManager::~LinkManager()
{

}

QHash<QUuid, Link> LinkManager::getHash() const
{
    return hash_;
}

LinkListModel* LinkManager::getModelPtr()
{
    return &model_;
}

QList<QSerialPortInfo> LinkManager::getSerialList() const
{
    return QSerialPortInfo::availablePorts();
}

QPair<QUuid, Link> LinkManager::createSerialPort(const QSerialPortInfo &serialInfo) const
{
    if (serialInfo.isNull())
        return {};

    Link newLink;
    //newLink.openAsSerial(serialInfo.portName());
    //qDebug() << "is opened: " << newLink.isOpen();

    QUuid uuid{ QUuid::createUuid() };

    return qMakePair(uuid, newLink);
}

void LinkManager::update()
{
    qDebug() << "LinkManager::update";

    auto currSerialList{ getSerialList() };

    qDebug() << "currSerialList:";
    for (auto& itm : currSerialList) {
        qDebug() << itm.portName();
    }

    addNewLinks(currSerialList);
    //deleteMissingLinks(currSerialList);

    qDebug() << "updated hash:";
    auto res = getHash();
    for (auto& itm : res) {
        qDebug() << itm.getPortName();
    }
}

void LinkManager::addNewLinks(const QList<QSerialPortInfo> &currSerialList)
{
    for (auto& itmI : currSerialList) {
        bool isBeen{ false };
        for (auto& itmJ : hash_) {
            if (itmI.portName() == itmJ.getPortName()) {
                isBeen = true;
                break;
            }
        }
        if (!isBeen) {
                auto link = createSerialPort(itmI);
                // hash
                hash_.insert(link.first, link.second);

                // TODO model
                QUuid uuid = QUuid::createUuid();

                bool connectionStatus = false;
                ::ControlType controlType = ::ControlType::kManual;
                QString portName = itmI.portName();
                int baudrate = 1;
                bool parity = false;
                ::LinkType linkType = ::LinkType::LinkNone;
                QString address = "1";
                int sourcePort = 1;
                int destinationPort = 1;
                bool isPinned = false;
                bool isHided = false;
                bool isNotAvailable = false;

                emit model_.appendEvent(uuid, connectionStatus, controlType, portName, baudrate, parity, linkType, address, sourcePort, destinationPort, isPinned, isHided, isNotAvailable);

                qDebug() << "added serial port: " << "  " /*link.second.getPortName()*/;

                emit stateChanged();

        }
    }
}

void LinkManager::deleteMissingLinks(const QList<QSerialPortInfo> &currSerialList)
{
    QHash<QUuid, Link>::iterator it;
    for (it = hash_.begin(); it != hash_.end(); ++it) {

        bool isBeen{ false };
        for (auto& itm : currSerialList) {
            if (itm.portName() == it->getPortName()) {
                isBeen = true;
                break;
            }
        }
        if (!isBeen) {
            // model
            emit model_.removeEvent(it.key());

            // hash
            it->disconnect();
            it = hash_.erase(it);

            qDebug() << "deleted serial port: " << it.value().getPortName();

            emit stateChanged();
        }
    }
}

void LinkManager::onExpiredTimer()
{
    qDebug() << "LinkManager::onTimerExpired()";
    update();
}

