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
    newLink.createAsSerial(serialInfo.portName());

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
                QUuid uuid = QUuid::createUuid(); // creating uuid
                bool connectionStatus = link.second.getConnectionStatus();
                ::ControlType controlType = link.second.getControlType();
                QString portName = link.second.getPortName(); // itmI .portName();
                int baudrate = link.second.getParity();
                bool parity = link.second.getParity();
                ::LinkType linkType = link.second.getLinkType();
                QString address = link.second.getAddress();
                int sourcePort = link.second.getSourcePort();
                int destinationPort = link.second.getDestinationPort();
                bool isPinned = link.second.isPinned();
                bool isHided = link.second.isHided();
                bool isNotAvailable = link.second.isNotAvailable();

                emit model_.appendEvent(uuid, connectionStatus, controlType, portName, baudrate, parity, linkType, address, sourcePort, destinationPort, isPinned, isHided, isNotAvailable);

                qDebug() << "added serial port: " << link.second.getPortName();

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

