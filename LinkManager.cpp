#include "LinkManager.h"

#include <QDebug>
#include <QListIterator>


LinkManager::LinkManager()
{
    // createSerialPortsByDefault();
    updateLinkModel("sdsgfsg");
}

LinkManager::~LinkManager()
{

}

QHash<QUuid, Link> LinkManager::createSerialPortsByDefault()
{
    updateLinksList();
    return linkHash_;
}

QHash<QUuid, Link> LinkManager::getLinkHash()
{
    return linkHash_;
}

LinkListModel* LinkManager::getLinkModel()
{
    return &linkModel_;
}

void LinkManager::updateLinkModel(const QString& portName)
{
    // TODO

    bool connectionStatus = false;
    ::ControlType controlType = ::ControlType::kManual;
    //QString portName_ = ";
    int baudrate = 1;
    bool parity = false;
    ::LinkType linkType = ::LinkType::LinkNone;
    QString address = "1";
    int sourcePort = 1;
    int destinationPort = 1;
    bool isPinned = false;
    bool isHided = false;
    bool isNotAvailable = false;

   linkModel_.appendEvent(connectionStatus, controlType, portName, baudrate, parity, linkType, address, sourcePort, destinationPort, isPinned, isHided, isNotAvailable);
    emit linkModelChanged();
}

QPair<QUuid, Link> LinkManager::createSerialPort(const QSerialPortInfo &serialInfo) const
{
    if (serialInfo.isNull())
        return {};

    Link newLink;
    newLink.openAsSerial(serialInfo.portName());
    QUuid uuid{ QUuid::createUuid() };

    return qMakePair(uuid, newLink);
}

QList<QSerialPortInfo> LinkManager::getSerialList() const
{
    return QSerialPortInfo::availablePorts();
}

void LinkManager::updateLinksList()
{
    auto currSerialList{ getSerialList() };
    if (addNewLinks(currSerialList) || deleteMissingLinks(currSerialList)) {
        emit linkHashChanged();
        emit linkModelChanged();
    }
}

bool LinkManager::addNewLinks(const QList<QSerialPortInfo> &currSerialList)
{
    bool retVal{ false };

    for (auto& itmI : currSerialList) {
        bool isBeen{ false };
        for (auto& itmJ : linkHash_) {
            if (itmI.portName() == itmJ.getPortName()) {
                isBeen = true;
                break;
            }
        }
        if (!isBeen) {
            if (auto link = createSerialPort(itmI); link.second.isOpen()) {
                linkHash_.insert(link.first, link.second);
                retVal = true;
            }
        }
    }

    return retVal;
}

bool LinkManager::deleteMissingLinks(const QList<QSerialPortInfo> &currSerialList)
{
    bool retVal{ false };

    QHash<QUuid, Link>::iterator it;
    for (it = linkHash_.begin(); it != linkHash_.end(); ++it) {

        bool isBeen{ false };
        for (auto& itm : currSerialList) {
            if (itm.portName() == it->getPortName()) {
                isBeen = true;
                break;
            }
        }
        if (!isBeen) {
            it->disconnect();
            it = linkHash_.erase(it);
            retVal = true;
        }
    }

    return retVal;
}

void LinkManager::addLink() {
    updateLinkModel("asascasc");
}
