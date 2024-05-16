#include "LinkManagerWorker.h"

#include <QDebug>


LinkManagerWorker::LinkManagerWorker(QList<Link*>* hashPtr, LinkListModel* modelPtr, QObject *parent) :
    QObject(parent),
    list_(hashPtr),
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

Link* LinkManagerWorker::createSerialPort(const QSerialPortInfo &serialInfo) const
{
    if (serialInfo.isNull())
        return {};

    Link* newLink = new Link();
    newLink->createAsSerial(serialInfo.portName(), 921600, false);

    connect(newLink, &Link::connectionStatusChanged, this, &LinkManagerWorker::stateChanged);


    return newLink;
}

void LinkManagerWorker::addNewLinks(const QList<QSerialPortInfo> &currSerialList)
{
    for (auto& itmI : currSerialList) {
        bool isBeen{ false };

        for (auto& itmJ : *list_) {
            if (itmI.portName() == itmJ->getPortName()) {
                isBeen = true;
                break;
            }
        }

        if (!isBeen) {
            auto link = createSerialPort(itmI);

            mutex_.lock();
            // hash
            list_->append(link);
            // model
            emit model_->appendEvent(link->getUuid(),
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
            mutex_.unlock();

            emit dataUpdated();
        }
    }
}

void LinkManagerWorker::deleteMissingLinks(const QList<QSerialPortInfo> &currSerialList)
{
    for (int i = 0; i < list_->size(); ++i) {

        Link* link = list_->at(i);

        bool isBeen{ false };
        for (auto& itm : currSerialList) {
            if (itm.portName() == link->getPortName()) {
                isBeen = true;
                break;
            }
        }

        if (!isBeen) {
            mutex_.lock();
            // model
            emit model_->removeEvent(link->getUuid());
            // list
            if (link->isOpen())
                link->disconnect();
            delete link;
            list_->removeAt(i);
            mutex_.unlock();

            --i;

            emit dataUpdated();
        }
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

void LinkManagerWorker::stateChanged(Link *linkPtr, bool state)
{
    emit model_->appendEvent(linkPtr->getUuid(),
                             state,
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

    emit dataUpdated();
}
