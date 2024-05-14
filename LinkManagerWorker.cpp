#include "LinkManagerWorker.h"

#include <QDebug>


LinkManagerWorker::LinkManagerWorker(QHash<QUuid, Link>* hashPtr, LinkListModel* modelPtr, QObject *parent) :
    QObject(parent),
    hash_(hashPtr),
    model_(modelPtr)
{
    timer_ = std::make_unique<QTimer>(this);
    timer_->setInterval(timerInterval_);

    QObject::connect(timer_.get(), &QTimer::timeout, this, &LinkManagerWorker::onExpiredTimer);
}

QList<QSerialPortInfo> LinkManagerWorker::getCurrentSerialList() const
{
    return QSerialPortInfo::availablePorts();
}

QPair<QUuid, Link> LinkManagerWorker::createSerialPort(const QSerialPortInfo &serialInfo) const
{
    if (serialInfo.isNull())
        return {};

    Link newLink;
    newLink.createAsSerial(serialInfo.portName(), 96100, false);

    QUuid uuid{ QUuid::createUuid() };

    return qMakePair(uuid, newLink);
}

void LinkManagerWorker::addNewLinks(const QList<QSerialPortInfo> &currSerialList)
{
    for (auto& itmI : currSerialList) {
        bool isBeen{ false };

        for (auto& itmJ : *hash_) {
            if (itmI.portName() == itmJ.getPortName()) {
                isBeen = true;
                break;
            }
        }

        if (!isBeen) {
            auto link = createSerialPort(itmI);

            mutex_.lock();
            // hash
            hash_->insert(link.first, link.second);
            // model
            emit model_->appendEvent(link.first,
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
            mutex_.unlock();

            emit dataUpdated();
        }
    }
}

void LinkManagerWorker::deleteMissingLinks(const QList<QSerialPortInfo> &currSerialList)
{
    for (auto it = hash_->begin(); it != hash_->end();) {

        bool isBeen{ false };
        for (auto& itm : currSerialList) {
            if (itm.portName() == it->getPortName()) {
                isBeen = true;
                break;
            }
        }

        if (!isBeen) {
            mutex_.lock();
            // model
            emit model_->removeEvent(it.key());
            // hash
            it = hash_->erase(it);
            mutex_.unlock();

            emit dataUpdated();
        }
        else
            ++it;
    }
}


void LinkManagerWorker::update()
{
    auto currSerialList{ getCurrentSerialList() };

    addNewLinks(currSerialList);
    deleteMissingLinks(currSerialList);
}

void LinkManagerWorker::onExpiredTimer()
{
    update();
    timer_->start();
}
