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
