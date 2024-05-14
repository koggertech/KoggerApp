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
    controlType_(ControlType::kManual),
    baudrate_(0),
    parity_(false),
    linkType_(LinkType::LinkNone),
    srcPort_(0),
    dstPort_(0),
    isPinned_(false),
    isHided_(false),
    isNotAvailable_(false)
{ }

Link::Link(const Link& other)
    : QObject(other.parent()),
    _mutex(),
    _frame(other._frame),
    _dev(other._dev),
    _context(other._context),
    _buffer(other._buffer),
    _type(other._type),
    controlType_(other.controlType_),
    portName_(other.portName_),
    baudrate_(other.baudrate_),
    parity_(other.parity_),
    linkType_(other.linkType_),
    address_(other.address_),
    srcPort_(other.srcPort_),
    dstPort_(other.dstPort_),
    isPinned_(other.isPinned_),
    isHided_(other.isHided_),
    isNotAvailable_(other.isNotAvailable_) {
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
    QSerialPort* dev = new QSerialPort();
    dev->setPortName(portName_);

    if (parity_) {
        dev->setParity(QSerialPort::NoParity);
    }

    dev->setBaudRate(baudrate_);

    setDev(dev);
    setType(LinkSerial);
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

bool Link::isOpen() {
    QIODevice *dev = device();
    if(dev != nullptr && dev->isOpen()) {
        return true;
    }
    return false;
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

bool Link::getConnectionStatus() const
{
    if(_dev != nullptr && _dev->isOpen()) {
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
    return srcPort_;
}

int Link::getDestinationPort() const
{
    return dstPort_;
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

Link &Link::operator=(const Link &other)
{
    this->_buffer = other._buffer;
    this->_context = other._context;
    this->_dev = other._dev;
    this->_frame = other._frame;
    this->_type = other._type;
    this->portName_ = other.portName_;

    return *this;
}

bool Link::operator==(const Link &other) const
{
    if (this->_type == other._type &&
        this->_dev == other._dev)
        return true;
    else
        return false;
}

bool Link::writeFrame(FrameParser *frame) {
    return frame->isComplete() && write(QByteArray((const char*)frame->frame(), frame->frameLen()));
}

bool Link::write(QByteArray data) {
    QIODevice *dev = device();
    if(dev != nullptr && dev->isOpen()) {
        _dev->write(data);
        return true;
    }
    return false;
}

void Link::setDev(QIODevice *dev) {
    deleteDev();
    if(dev != nullptr && dev->isOpen()) {
        _dev = dev;
        connect(dev, &QAbstractSocket::readyRead, this, &Link::readyRead);
        connect(dev, &QAbstractSocket::aboutToClose, this, &Link::aboutToClose);
    }
}

void Link::deleteDev() {
    if(_dev != nullptr) {
        if(_dev->isOpen()) {
            _dev->close();
        }
        _dev->disconnect(this);
        this->disconnect(_dev);
        setType(LinkNone);
        delete _dev;
        _dev = nullptr;
    }
}

void Link::toContext(const QByteArray data) {
//    _mutex.lock();
    if(_buffer.size() == 0) {
        _buffer.append(data);
        emit readyParse(this);
    }
//    _mutex.unlock();
}
void Link::readyRead() {
    QIODevice *dev = device();
    if(dev != nullptr) {
        toContext(dev->readAll());
    }
}

void Link::aboutToClose() {
    QIODevice *dev = device();
    if(dev != nullptr) {
        emit changeState();
    }
}
