#include "connection.h"
#include "QUrl"
#include <QDir>
#include <QDateTime>

#include <core.h>
extern Core core;

Connection::Connection():
    m_serial(new QSerialPort(this)),
    m_file(new QFile(this)),
    _socket(new QUdpSocket(this)),
    _timerReconnection(new QTimer())
{
    connect(m_serial, &QSerialPort::aboutToClose, this, &Connection::closing);
    connect(m_serial, &QSerialPort::errorOccurred, this, &Connection::handleSerialError);
    connect(m_serial, &QSerialPort::readyRead, this, &Connection::readyReadSerial);

    connect(_socket, &QAbstractSocket::aboutToClose, this, &Connection::closing);
    connect(_socket, &QAbstractSocket::readyRead, this, &Connection::readyReadSerial);

    connect(m_file, &QFile::aboutToClose, this, &Connection::closing);

    connect(_timerReconnection, &QTimer::timeout, this, &Connection::reOpenSerial);
}

QList<QSerialPortInfo> Connection::availableSerial() {
    return  QSerialPortInfo::availablePorts();
}

bool Connection::reOpenSerial() {
    close();

    m_serial->open(QIODevice::ReadWrite);
    bool is_open = m_serial->isOpen();

    if(is_open) {
        m_type = ConnectionSerial;
        emit openedEvent(true);
        core.consoleInfo("Connection: serial is open");
    } else {
        core.consoleInfo("Connection: serial isn't open");
    }

    return is_open;
}

bool Connection::openSerial(int32_t baudrate, bool parity) {
    if(parity) {
        m_serial->setParity(QSerialPort::EvenParity);
    } else {
        m_serial->setParity(QSerialPort::NoParity);
    }

    m_serial->setBaudRate(baudrate);

    return reOpenSerial();
}

bool Connection::openSerial(const QString &name, int32_t baudrate, bool parity){
    _timerReconnection->stop();
    m_serial->setPortName(name);
    return openSerial(baudrate, parity);
}

bool Connection::openFile(const QString &name) {
    close();

    QUrl url(name);
    m_file->setFileName(url.toLocalFile());

    bool is_open = false;
    is_open = m_file->open(QIODevice::ReadOnly);

    if(is_open == false) {
        return false;
    }

    m_type = ConnectionFile;

    emit openedEvent(false);

    QByteArray data = m_file->readAll();
    m_file->close();
    emit receiveData(data);
    data.clear();

    return true;
}

bool Connection::openIP(const QString &address, const int port, bool is_tcp) {
    close();
//    m_socket->connectToHost("192.168.4.1", 23, QIODevice::ReadWrite);

    _socket->bind(QHostAddress::Any, port); // , QAbstractSocket::ReuseAddressHint | QAbstractSocket::ShareAddress
//    m_socket->setSocketOption(QAbstractSocket::SendBufferSizeSocketOption,    256 * 1024);
//    m_socket->setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption, 512 * 1024);
    _socket->connectToHost(address, port, QIODevice::ReadWrite);

    if (_socket->waitForConnected(1000)) {
        qInfo("Socket Connected!");
        m_type = ConnectionIP;
     } else {
        qInfo("Socket NOT Connected!");
    }
    bool is_open = isOpen();

    if(is_open) {
        emit openedEvent(true);
        core.consoleInfo("Connection: socket is open");
    } else {
        core.consoleInfo("Connection: socket isn't open");
    }

    return false;
}

bool Connection::setBaudrate(int32_t baudrate) {
//    m_serial->flush();
    m_serial->waitForBytesWritten(500);
    return m_serial->setBaudRate(baudrate);
}

int Connection::baudrate() {
    return m_serial->baudRate();
}

bool Connection::isOpen() {
    bool is_open = false;

    switch (m_type) {
    case ConnectionSerial:
        is_open = m_serial->isOpen();
        break;
    case ConnectionFile:
        is_open = m_file->isOpen();
        break;
    case ConnectionIP:
        is_open = _socket->state() != QAbstractSocket::UnconnectedState;
        break;
    default:
        break;
    }

    return is_open;
}

bool Connection::isParity() {
    if(m_type == ConnectionSerial) {
        return m_serial->parity() != QSerialPort::NoParity;
    }

    return false;
}

void Connection::setParity(bool parity) {
    if(ConnectionSerial == m_type)  {
        if(parity) {
            m_serial->setParity(QSerialPort::EvenParity);
        } else {
            m_serial->setParity(QSerialPort::NoParity);
        }
    }
}

void Connection::setDTR(bool val) {
    if(ConnectionSerial == m_type)  {
        m_serial->setDataTerminalReady(val);
    }
}

void Connection::setRTS(bool val) {
    if(ConnectionSerial == m_type)  {
        m_serial->setRequestToSend(val);
    }
}

bool Connection::close(bool is_user) {
    switch (m_type) {
    case ConnectionSerial:
        if(m_serial->isOpen()) {
//            setRTS(false);
            m_serial->close();
            m_serial->close();
            _timerReconnection->stop();
            core.consoleInfo("Connection: Port closed");
        }
        break;

    case ConnectionFile:
        if(m_file->isOpen()) {
            m_file->close();
        }
        break;

    case ConnectionIP:
        if(_socket->isOpen()) {
            _socket->disconnectFromHost();
            _socket->close();
        }
        break;
    default:
        break;
    }

    return true;
}

void Connection::sendData(const QByteArray &data){
    switch (m_type) {
    case ConnectionSerial:
        m_serial->write(data);
        loggingStream(data);
        break;
    case ConnectionFile:
        break;
    case ConnectionIP:
        _socket->write(data);
        loggingStream(data);
    default:
        break;
    }
}

void Connection::closing() {
//    if(_userClosed == false && m_type == ConnectionSerial) {
//        core.consoleInfo("Connection: started try connection");
//        _timerReconnection->start();
//    }

    m_type = ConnectionNone;
    emit closedEvent(false);
}

void Connection::handleSerialError(QSerialPort::SerialPortError error) {
    if (error == QSerialPort::ResourceError) {

        close(false);
//        core.consoleInfo("Connection: started try connection");
//        _timerReconnection->start(500);
    }
}

void Connection::readyReadSerial() {
    QByteArray data;

    switch (m_type) {
    case ConnectionSerial:
        data = m_serial->readAll();
        break;

    case ConnectionIP:
//        data = _socket->readAll();

            while (_socket->hasPendingDatagrams())
            {
                QByteArray datagram;
                datagram.resize(_socket->pendingDatagramSize());
                QHostAddress sender;
                quint16 senderPort;
                // If the other end is reset then it will still report data available,
                // but will fail on the readDatagram call
                qint64 slen = _socket->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);
                if (slen == -1) {
                    break;
                }
                data.append(datagram);
            }



        break;
    default:
        break;
    }

    loggingStream(data);
    emit receiveData(data);
}
