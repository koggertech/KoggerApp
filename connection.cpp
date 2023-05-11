#include "connection.h"
#include "QUrl"
#include <QDir>
#include <QDateTime>

#include <core.h>
extern Core core;

Connection::Connection():
    m_serial(new QSerialPort(this)),
    m_file(new QFile(this)),
    _socketUDP(new QUdpSocket(this)),
    _socketTCP(new QTcpSocket(this)),
    _timerReconnection(new QTimer())
{
    m_serial->moveToThread(&workerThread);
    connect(&workerThread, &QThread::finished, m_serial, &QObject::deleteLater);

    connect(m_serial, &QSerialPort::aboutToClose, this, &Connection::closing);
#if defined(Q_OS_ANDROID)
//    connect(m_serial, &QSerialPort::error, this, &Connection::handleSerialError);
#else
    connect(m_serial, &QSerialPort::errorOccurred, this, &Connection::handleSerialError);
#endif

    connect(m_serial, &QSerialPort::readyRead, this, &Connection::readyReadSerial, Qt::QueuedConnection);

    workerThread.start();

    connect(_socketUDP, &QAbstractSocket::aboutToClose, this, &Connection::closing);
    connect(_socketUDP, &QAbstractSocket::readyRead, this, &Connection::readyReadSerial);

    connect(_socketTCP, &QAbstractSocket::aboutToClose, this, &Connection::closing);
    connect(_socketTCP, &QAbstractSocket::readyRead, this, &Connection::readyReadSerial);

    connect(m_file, &QFile::aboutToClose, this, &Connection::closing);

    connect(_timerReconnection, &QTimer::timeout, this, &Connection::reOpenSerial);
}

QList<QSerialPortInfo> Connection::availableSerial() {
    return  QSerialPortInfo::availablePorts();
}

bool Connection::reOpenSerial() {
    close();

    m_serial->setReadBufferSize(8*1024 * 1024);

    m_serial->open(QIODevice::ReadWrite);
    bool is_open = m_serial->isOpen();

    if(is_open) {
        setType(ConnectionSerial);
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
    static QByteArray data;
    close();

    core.consoleInfo(QString("File path source: %1").arg(name));

    QUrl url(name);
    if(url.isLocalFile()) {
        m_file->setFileName(url.toLocalFile());
    } else {
        m_file->setFileName(url.toString());
    }

    core.consoleInfo(QString("File path: %1").arg(m_file->fileName()));

    bool is_open = false;
    is_open = m_file->open(QIODevice::ReadOnly);

    if(is_open == false)
    {
        data.clear();
        return false;
    }

    setType(ConnectionFile);

    emit openedEvent(false);

    while(true) {
        data.append(m_file->read(1024*1024));
        if(data.size() == 0) { break; }
        emit receiveData(data);
        data.clear();
    }

    data.clear();

    m_file->close();


    return true;
}

bool Connection::openIP(const QString &address, const int port, bool is_tcp) {
    close();
//    m_socket->connectToHost("192.168.4.1", 23, QIODevice::ReadWrite);

    if(is_tcp) {
        _socketTCP->setSocketOption(QAbstractSocket::SendBufferSizeSocketOption,     128 * 1024);
        _socketTCP->setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption, 256 * 1024);
        _socketTCP->connectToHost(address, port, QIODevice::ReadWrite);

        if (_socketTCP->waitForConnected(1000)) {
            qInfo("TCP socket is connected!");
            setType(ConnectionTCP);
         } else {
            qInfo("TCP socket is not connected!");
        }
    } else {
        _socketUDP->setSocketOption(QAbstractSocket::SendBufferSizeSocketOption,     128 * 1024);
        _socketUDP->setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption, 256 * 1024);
        _socketUDP->bind(QHostAddress::Any, port); // , QAbstractSocket::ReuseAddressHint | QAbstractSocket::ShareAddress
        _socketUDP->connectToHost(address, port, QIODevice::ReadWrite);

        if (_socketUDP->waitForConnected(1000)) {
            qInfo("UDP socket is connected!");
            setType(ConnectionUDP);
         } else {
            qInfo("UDP socket is not connected!");
        }
    }

    bool is_open = isOpen();

    if(is_open) {
        emit openedEvent(true);
        core.consoleInfo("Connection: socket is open");
    } else {
        core.consoleInfo("Connection: socket isn't open");
    }

    return true;
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
    case ConnectionUDP:
        is_open = _socketUDP->state() != QAbstractSocket::UnconnectedState;
        break;
    case ConnectionTCP:
        is_open = _socketTCP->state() != QAbstractSocket::UnconnectedState;
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

    case ConnectionUDP:
        if(_socketUDP->isOpen()) {
            _socketUDP->disconnectFromHost();
            _socketUDP->close();
        }
        break;

    case ConnectionTCP:
        if(_socketTCP->isOpen()) {
            _socketTCP->disconnectFromHost();
            _socketTCP->close();
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
        break;
    case ConnectionFile:
        break;
    case ConnectionUDP:
        _socketUDP->write(data);
        break;
    case ConnectionTCP:
        _socketTCP->write(data);
        break;
    default:
        break;
    }

    if(m_type != ConnectionFile) {
        loggingStream(data);
    }
}

void Connection::closing() {
//    if(_userClosed == false && m_type == ConnectionSerial) {
//        core.consoleInfo("Connection: started try connection");
//        _timerReconnection->start();
//    }

    setType(ConnectionNone);
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
    data.clear();

    switch (m_type) {
    case ConnectionSerial:
        data = m_serial->readAll();
        break;

    case ConnectionUDP:
        while (_socketUDP->hasPendingDatagrams())
        {
            QByteArray datagram;
            datagram.resize(_socketUDP->pendingDatagramSize());
            QHostAddress sender;
            quint16 senderPort;

            qint64 slen = _socketUDP->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);
            if (slen == -1) {
                break;
            }
            data.append(datagram);
        }
        break;
    case ConnectionTCP:
        data.append(_socketTCP->readAll());
        break;
    default:
        break;
    }

    if(data.size() > 0) {
        loggingStream(data);
        emit receiveData(data);
    }
}
