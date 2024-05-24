#include "Link.h"

#include <QUdpSocket>
#include <QTcpSocket>
#if defined(Q_OS_ANDROID)
#include "qtandroidserialport/src/qserialport.h"
#include "qtandroidserialport/src/qserialportinfo.h"
#else
#include <QSerialPort>
#include <QSerialPortInfo>
#endif


Link::Link() :
    uuid_(QUuid::createUuid()),
    controlType_(ControlType::kManual),
    linkType_(LinkType::LinkNone),
    portName_(""),
    baudrate_(0),
    parity_(false),
    address_(""),
    sourcePort_(0),
    destinationPort_(0),
    isPinned_(false),
    isHided_(false),
    isNotAvailable_(false)
{
    _frame.resetComplete();
}

Link::Link(QString uuidStr, ControlType controlType, LinkType linkType, QString portName,
           int baudrate, bool parity, QString address, int sourcePort, int destinationPort,
           bool isPinned, bool isHided, bool isNotAvailable) :
    uuid_(QUuid(uuidStr)),
    controlType_(controlType),
    linkType_(linkType),
    portName_(portName),
    baudrate_(baudrate),
    parity_(parity),
    address_(address),
    sourcePort_(sourcePort),
    destinationPort_(destinationPort),
    isPinned_(isPinned),
    isHided_(isHided),
    isNotAvailable_(isNotAvailable)
{

}

void Link::createAsSerial(const QString &portName, int baudrate, bool parity)
{
    qDebug() << "Link::createAsSerial, uuid:" << getUuid().toString();

    linkType_ = LinkType::LinkSerial;
    portName_ = portName;
    parity_ = parity;
    baudrate_ = baudrate;
}

void Link::openAsSerial()
{
    qDebug() << "Link::openAsSerial, uuid:" << getUuid().toString();

    QSerialPort *serialPort = new QSerialPort(this);

    serialPort->setPortName(portName_);
    parity_ ? serialPort->setParity(QSerialPort::EvenParity) : serialPort->setParity(QSerialPort::NoParity);
    serialPort->setBaudRate(baudrate_);

    serialPort->setReadBufferSize(8 * 1024 * 1024);
    serialPort->open(QIODevice::ReadWrite);

    if (serialPort->isOpen()) {
        setDev(serialPort);
        emit connectionStatusChanged(uuid_);
        emit opened(uuid_, this);

        qDebug() << "link opened as serial: " << getUuid();
    }
    else {
        delete serialPort;
        emit connectionStatusChanged(uuid_);
        qDebug() << "link not opened as serial: " << getUuid();
    }

}

void Link::createAsUdp(const QString &address, int sourcePort, int destinationPort)
{
    qDebug() << "Link::createAsUdp, uuid:" << getUuid().toString();

    linkType_ = LinkType::LinkIPUDP;
    address_ = address;
    sourcePort_ = sourcePort;
    destinationPort_ = destinationPort;
}

void Link::updateUdpParameters(const QString& address, int sourcePort, int destinationPort)
{
    qDebug() << "Link::updateUdpParameters, uuid:" << getUuid().toString();

    address_ = address;
    sourcePort_ = sourcePort;
    destinationPort_ = destinationPort;
}

void Link::openAsUdp()
{
    qDebug() << "Link::openAsUdp, uuid:" << getUuid().toString();

    QUdpSocket *socketUdp = new QUdpSocket(this);

    bool isBinded = socketUdp->bind(QHostAddress::AnyIPv4, sourcePort_); // , QAbstractSocket::ReuseAddressHint | QAbstractSocket::ShareAddress

    if (!isBinded) {
        delete socketUdp;
        return;
    }

    hostAddress_.setAddress(address_);

    if (isBinded) {
        socketUdp->open(QIODevice::ReadWrite);
        setDev(socketUdp);

        emit connectionStatusChanged(uuid_);
        emit opened(uuid_, this);

        qDebug() << "link opened as udp: " << getUuid();
    }
    else {
        delete socketUdp;

        emit connectionStatusChanged(uuid_);
        qDebug() << "link not opened as udp: " << getUuid();
    }
}

void Link::createAsTcp(const QString &address, int sourcePort, int destinationPort)
{
    qDebug() << "Link::createAsTcp, uuid:" << getUuid().toString();

    linkType_ = LinkType::LinkIPTCP;
    address_ = address;
    sourcePort_ = sourcePort;
    destinationPort_ = destinationPort;
}

void Link::updateTcpParameters(const QString& address, int sourcePort, int destinationPort)
{
    qDebug() << "Link::updateTcpParameters, uuid:" << getUuid().toString();

    address_ = address;
    sourcePort_ = sourcePort;
    destinationPort_ = destinationPort;
}

void Link::openAsTcp()
{
    qDebug() << "Link::openAsTcp, uuid:" << getUuid().toString();

    // TODO
}

void Link::openAsUDP(const QString &address, const int port_in,  const int port_out) {
    updateUdpParameters(address, port_in, port_out);
    openAsUdp();
}

bool Link::isOpen() const {
    bool retVal{ false };

    if (!ioDevice_)
        return retVal;

    switch (linkType_) {
    case LinkType::LinkNone: {
        retVal = false;
        break;
    }
    case LinkType::LinkSerial: {
        retVal = ioDevice_->isOpen();
        break;
    }
    case LinkType::LinkIPUDP: {
        if (auto ptr = static_cast<QUdpSocket*>(ioDevice_); ptr)
            retVal = ptr->state() != QAbstractSocket::UnconnectedState;
        break;
    }
    case LinkType::LinkIPTCP: {
        if (auto ptr = static_cast<QTcpSocket*>(ioDevice_); ptr)
            retVal = ptr->state() != QAbstractSocket::UnconnectedState;
        break;
    }
    default:
        break;
    }

    return retVal;
}

void Link::close() {
    deleteDev();
}

bool Link::parse()
{
    if (_frame.availContext() == 0) {
        if (_buffer.size() > 0) {
            _context = QByteArray::fromRawData(_buffer.constData(), _buffer.size());
            _buffer.resize(0);
            _frame.setContext((uint8_t*)_context.data(), _context.size());
        }
    }

    _frame.process();
    return _frame.isComplete() || _frame.availContext();
}

void Link::setUuid(QUuid uuid)
{
    uuid_ = uuid;
}

void Link::setConnectionStatus(bool connectionStatus)
{
    if (!connectionStatus)
        close();
    else {
        switch (linkType_) {
        case LinkType::LinkNone: { break; }
        case LinkType::LinkSerial: { openAsSerial(); break; }
        case LinkType::LinkIPUDP: { openAsUdp(); break; }
        case LinkType::LinkIPTCP: { openAsTcp(); break; }
        default: { break; }
        }
    }
}

void Link::setControlType(ControlType controlType)
{
    controlType_ = controlType;
}

void Link::setPortName(const QString &portName)
{
    portName_ = portName;
}

void Link::setBaudrate(int baudrate)
{
    qDebug() << "Link::setBaudrate";

    baudrate_ = baudrate;

    if (linkType_ == LinkType::LinkSerial) {
        if (auto currDev = static_cast<QSerialPort*>(ioDevice_); currDev) {
            currDev->setBaudRate(baudrate_);
            qDebug() << "casted & setted";
        }
    }
}

void Link::setParity(bool parity)
{
    parity_ = parity;
}

void Link::setLinkType(LinkType linkType)
{
    linkType_ = linkType;
}

void Link::setAddress(const QString &address)
{
    address_ = address;
    //hostAddress_.setAddress(address_);
    // TODO: rebind?
}

void Link::setSourcePort(int sourcePort)
{
    sourcePort_ = sourcePort;
    // TODO: rebind?
}

void Link::setDestinationPort(int destinationPort)
{
    destinationPort_ = destinationPort;
    // TODO: rebind?
}

void Link::setIsPinned(bool state)
{
    isPinned_ = state;
}

void Link::setIsHided(bool isHided)
{
    isHided_ = isHided;
}

void Link::setIsNotAvailable(bool isNotAvailable)
{
    qDebug() << "Link::setNotAvailable: " << isNotAvailable;
    isNotAvailable_ = isNotAvailable;
}

QUuid Link::getUuid() const
{
    return uuid_;
}

bool Link::getConnectionStatus() const
{
    if(ioDevice_ != nullptr && ioDevice_->isOpen()) {
        return true;
    }
    return false;
}

ControlType Link::getControlType() const
{
    return controlType_;
}

QString Link::getPortName() const
{
    return portName_;
}

int Link::getBaudrate() const
{
    return baudrate_;
}

bool Link::getParity() const
{
    return parity_;
}

LinkType Link::getLinkType() const
{
    return linkType_;
}

QString Link::getAddress() const
{
    return address_;
}

int Link::getSourcePort() const
{
    return sourcePort_;
}

int Link::getDestinationPort() const
{
    return destinationPort_;
}

bool Link::getIsPinned() const
{
    return isPinned_;
}

bool Link::getIsHided() const
{
    return isHided_;
}

bool Link::getIsNotAvailable() const
{
    return isNotAvailable_;
}

bool Link::writeFrame(FrameParser frame) {
    return frame.isComplete() && write(QByteArray((const char*)frame.frame(), frame.frameLen()));
}

bool Link::write(QByteArray data) {
    QIODevice *dev = device();
    if(dev != nullptr && isOpen()) {
        if(linkType_ == LinkType::LinkIPUDP) {
            (static_cast<QUdpSocket*>(ioDevice_))->writeDatagram(data, hostAddress_, destinationPort_);
        } else {
            ioDevice_->write(data);
        }
        return true;
    }
    return false;
}

void Link::setDev(QIODevice *dev) {
    deleteDev();
    if(dev != nullptr) {
        ioDevice_ = dev;
        connect(dev, &QAbstractSocket::readyRead, this, &Link::readyRead);
        connect(dev, &QAbstractSocket::aboutToClose, this, &Link::aboutToClose);
    }
}

void Link::deleteDev()
{
    if (ioDevice_ != nullptr) {
        if (ioDevice_->isOpen()) {
            ioDevice_->close();
            emit connectionStatusChanged(uuid_);  // ???
            //emit closed(uuid_, this)
        }

        ioDevice_->disconnect(this);
        this->disconnect(ioDevice_);
        //setLinkType(LinkNone);
        delete ioDevice_;
        ioDevice_ = nullptr;
    }

    qDebug() << "link deleted dev: " << getUuid();

}

void Link::toParser(const QByteArray data) {
    if(data.size() <= 0) {
        return;
    }

    _frame.setContext((uint8_t*)data.data(), data.size());

    while (_frame.availContext() > 0) {
        _frame.process();
        if(_frame.isComplete()) {
            emit frameReady(uuid_, this, _frame);
        }
    }
}

void Link::readyRead() {
    QIODevice *dev = device();
    if(dev != nullptr) {
        if(linkType_ == LinkType::LinkIPUDP) {
            QUdpSocket* socs_upd = (QUdpSocket*)dev;
            while (socs_upd->hasPendingDatagrams())
            {
                QByteArray datagram;
                datagram.resize(socs_upd->pendingDatagramSize());
                QHostAddress sender;
                quint16 senderPort;

                qint64 slen = socs_upd->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);
                if (slen == -1) {
                    break;
                }

                toParser(datagram);
            }
        } else {
            toParser(dev->readAll());
        }


    }
}

void Link::aboutToClose() {
    QIODevice *dev = device();
    if (dev != nullptr) {
        //emit changeState(); //
        emit connectionStatusChanged(uuid_);
        emit closed(uuid_, this);
        qDebug() << "link aboutToClose dev: " << getUuid();
    }
}
