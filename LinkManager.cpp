#include "LinkManager.h"

#include <QDebug>
#include <QListIterator>


namespace linking {

LinkManager::LinkManager()
{

}

LinkManager::~LinkManager()
{

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

QHash<QUuid, Link> LinkManager::createSerialPortsByDefault()
{
    updateLinksList();
    return linkHash_;
}


QList<QSerialPortInfo> LinkManager::getSerialList() const
{
    return QSerialPortInfo::availablePorts();
}

void LinkManager::updateLinksList()
{
    auto currSerialList{ getSerialList() };
    addNewLinks(currSerialList);
    deleteMissingLinks(currSerialList);
}

void LinkManager::addNewLinks(const QList<QSerialPortInfo> &currSerialList)
{
    for (auto& itmI : currSerialList) {
        bool isBeen{ false };
        for (auto& itmJ : linkHash_) {
            if (itmI.portName() == itmJ.getPortName()) {
                isBeen = true;
                break;
            }
        }
        if (!isBeen) {
            if (auto link = createSerialPort(itmI); link.second.isOpen())
                linkHash_.insert(link.first, link.second);
        }
    }
}

void LinkManager::deleteMissingLinks(const QList<QSerialPortInfo> &currSerialList)
{
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
        }
    }
}

} // namespace linking
