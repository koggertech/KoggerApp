#include "connection.h"
#include "QMetaEnum"
#include "QUrl"

#include <core.h>
extern Core core;

Connection::Connection():
    m_serial(new QSerialPort(this)),
    m_file(new QFile(this))
{
    connect(m_serial, &QSerialPort::aboutToClose, this, &Connection::closing);
    connect(m_serial, &QSerialPort::errorOccurred, this, &Connection::handleSerialError);
    connect(m_serial, &QSerialPort::readyRead, this, &Connection::readyReadSerial);

    connect(m_file, &QFile::aboutToClose, this, &Connection::closing);
}

QList<QSerialPortInfo> Connection::availableSerial() {
    return  QSerialPortInfo::availablePorts();
}

bool Connection::openSerial(bool parity) {
    if(parity) {
        m_serial->setParity(QSerialPort::EvenParity);
    } else {
        m_serial->setParity(QSerialPort::NoParity);
    }

    m_serial->open(QIODevice::ReadWrite);
    m_type = ConnectionSerial;

    bool is_open = isOpen();

    if(is_open) {
        emit openedEvent(true);
        core.consoleInfo("Connection: serial is open");
    } else {
        core.consoleInfo("Connection: serial isn't open");
    }

    return is_open;
}

bool Connection::openSerial(const QString &name, int32_t baudrate, bool parity){
    close();

    m_serial->setPortName(name);
    m_serial->setBaudRate(baudrate);

    if(openSerial(parity) == false) {
        return false;
    }


    return true;
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

    QByteArray data =  m_file->readAll();
    emit receiveData(data);
    return true;
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

bool Connection::close() {
    switch (m_type) {
    case ConnectionSerial:
        if(m_serial->isOpen()) {
            setRTS(false);
            m_serial->close();
            m_serial->close();
            core.consoleInfo("Connection: serial is close");
        }
        break;

    case ConnectionFile:
        if(m_file->isOpen()) {
            m_file->close();
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
//        core.consoleInfo("send data");
        m_serial->write(data);
        break;
    case ConnectionFile:
        break;
    default:
        break;
    }
}

void Connection::closing() {
    m_type = ConnectionNone;
    emit closedEvent(false);
}

void Connection::handleSerialError(QSerialPort::SerialPortError error) {
    if (error == QSerialPort::ResourceError) {
        close();
    }
}

void Connection::readyReadSerial() {
    const QByteArray data = m_serial->readAll();
    emit receiveData(data);
}
