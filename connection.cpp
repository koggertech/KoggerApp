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

bool Connection::openSerial(const QString &name, int32_t baudrate){
    close();

    m_serial->setPortName(name);
    m_serial->setBaudRate(baudrate);

    if (m_serial->open(QIODevice::ReadWrite) == false) {
        qInfo("Serial connection failed to open");
        return false;
    }

    m_type = ConnectionSerial;
//    qInfo("Serial connection open");
    core.consoleInfo(QStringLiteral("Connection to ") + name + ":" + QString::number(baudrate));
    emit openedEvent(true);
    return true;
}

bool Connection::openFile(const QString &name) {
    close();

    QUrl url(name);
    m_file->setFileName(url.toLocalFile());

    bool is_open = false;
    is_open = m_file->open(QIODevice::ReadOnly);

    if(is_open == false) {
        qInfo("File connection failed to open");
        return false;
    }

    m_type = ConnectionFile;
    qInfo("File connection open");

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

bool Connection::close() {
    switch (m_type) {
    case ConnectionSerial:
        if(m_serial->isOpen()) {
            m_serial->close();
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
