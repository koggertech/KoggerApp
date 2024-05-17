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
    baudrate_(0),
    parity_(false),
    linkType_(LinkType::LinkNone),
    sourcePort_(0),
    destinationPort_(0),
    isPinned_(false),
    isHided_(false),
    isNotAvailable_(false)
{
    _frame.resetComplete();
}

void Link::createAsSerial(const QString &portName, int baudrate, bool parity)
{
    linkType_ = LinkType::LinkSerial;
    portName_ = portName;
    parity_ = parity;
    baudrate_ = baudrate;
}

void Link::openAsSerial()
{
    QSerialPort *serialPort = new QSerialPort(this);

    serialPort->setPortName(portName_);
    parity_ ? serialPort->setParity(QSerialPort::EvenParity) : serialPort->setParity(QSerialPort::NoParity);
    serialPort->setBaudRate(baudrate_);

    serialPort->setReadBufferSize(8 * 1024 * 1024);
    serialPort->open(QIODevice::ReadWrite);

    if (serialPort->isOpen()) {
        setDev(serialPort);
        emit connectionStatusChanged(uuid_);
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
    linkType_ = LinkType::LinkIPUDP;
    address_ = address;
    sourcePort_ = sourcePort;
    destinationPort_ = destinationPort;
}

void Link::openAsUdp()
{
    QUdpSocket *socketUdp = new QUdpSocket(this);

    bool isBinded = socketUdp->bind(QHostAddress::AnyIPv4, sourcePort_, QAbstractSocket::ReuseAddressHint | QAbstractSocket::ShareAddress);

    if (!isBinded) {
        delete socketUdp;
        return;
    }

    hostAddress_.setAddress(address_);
    if (destinationPort_ > 0)
        socketUdp->connectToHost(hostAddress_, destinationPort_, QIODevice::ReadWrite);

    bool isOpen = socketUdp->state() != QAbstractSocket::UnconnectedState;

    if (isOpen) {
        setDev(socketUdp);

        emit connectionStatusChanged(uuid_);
        qDebug() << "link opened as udp: " << getUuid();
    }
    else {
        delete socketUdp;

        emit connectionStatusChanged(uuid_);
        qDebug() << "link not opened as udp: " << getUuid();
    }
}

void Link::openAsUDP(const QString &address, const int port_in,  const int port_out) {
    QUdpSocket* socket = new QUdpSocket();

    socket->setSocketOption(QAbstractSocket::SendBufferSizeSocketOption,     4*1024*1024);
    socket->setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption, 4*1024*1024);
    bool is_bind = socket->bind(QHostAddress::Any, port_in); // , QAbstractSocket::ReuseAddressHint | QAbstractSocket::ShareAddress
    if(port_out > 0) {
        socket->connectToHost(address, port_out, QIODevice::ReadWrite);
    }

    if (is_bind) {
        setDev(socket);
        setType(LinkIPUDP);
    } else {
        delete socket;
    }
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

bool Link::isPinned() const
{
    return isPinned_;
}

bool Link::isHided() const
{
    return isHided_;
}

bool Link::isNotAvailable() const
{
    return isNotAvailable_;
}

bool Link::writeFrame(FrameParser frame) {
    return frame.isComplete() && write(QByteArray((const char*)frame.frame(), frame.frameLen()));
}

bool Link::write(QByteArray data) {
    QIODevice *dev = device();
    if(dev != nullptr && dev->isOpen()) {
        ioDevice_->write(data);
        return true;
    }
    return false;
}

void Link::setDev(QIODevice *dev) {
    deleteDev();
    if(dev != nullptr && dev->isOpen()) {
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
        }

        ioDevice_->disconnect(this);
        this->disconnect(ioDevice_);
        setType(LinkNone);
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
            emit frameReady(this, _frame);
        }
    }
}

void Link::readyRead() {
    QIODevice *dev = device();
    if(dev != nullptr) {
        toParser(dev->readAll());
    }
}

void Link::aboutToClose() {
    QIODevice *dev = device();
    if (dev != nullptr) {
        //emit changeState(); //
        emit connectionStatusChanged(uuid_);
        emit closed(this);
        qDebug() << "link aboutToClose dev: " << getUuid();
    }
}
