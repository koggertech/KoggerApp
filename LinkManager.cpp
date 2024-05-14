#include "LinkManager.h"

#include <QDebug>


LinkManager::LinkManager()
{

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
    newLink.createAsSerial(serialInfo.portName(), 96100, false);

    QUuid uuid{ QUuid::createUuid() };

    return qMakePair(uuid, newLink);
}

void LinkManager::update()
{
    auto currSerialList{ getSerialList() };

    addNewLinks(currSerialList);
    deleteMissingLinks(currSerialList);
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

                // model
                emit model_.appendEvent(link.first,
                                        link.second.getConnectionStatus(),
                                        link.second.getControlType(),
                                        link.second.getPortName(),
                                        link.second.getBaudrate(),
                                        link.second.getParity(),
                                        link.second.getLinkType(),
                                        link.second.getAddress(),
                                        link.second.getSourcePort(),
                                        link.second.getDestinationPort(),
                                        link.second.isPinned(),
                                        link.second.isHided(),
                                        link.second.isNotAvailable());

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

            emit stateChanged();
        }
    }
}

void LinkManager::onExpiredTimer()
{
    update();
}

