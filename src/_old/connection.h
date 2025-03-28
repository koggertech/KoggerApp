#ifndef CONNECTION_H
#define CONNECTION_H

#include <QObject>
#include <QFile>
#include <QDataStream>
#include <console.h>
#include <QAbstractSocket>
#include <QUdpSocket>
#include <QTcpSocket>
#include <QTcpServer>
#include <QHostAddress>
#include <QTimer>
#include <QThread>

#if defined(Q_OS_ANDROID)
#include "platform/android/src/qtandroidserialport/src/qserialport.h"
#include "platform/android/src/qtandroidserialport/src/qserialportinfo.h"
#else
#include <QSerialPort>
#include <QSerialPortInfo>
#endif


class Connection : public QObject
{
    Q_OBJECT
public:
    Connection();

    typedef enum {
        ConnectionNone,
        ConnectionSerial,
        ConnectionFile,
        ConnectionUDP,
        ConnectionTCP,
    } ConnectionType;

public slots:
    QList<QSerialPortInfo> availableSerial();
    bool reOpenSerial();
    bool openSerial(int32_t baudrate, bool parity = false);
    bool openSerial(const QString &name, int32_t baudrate, bool parity = false);
    bool openFile(const QString &name);
    bool openIP(const QString &address, const int port, bool is_tcp);

    bool setBaudrate(int32_t baudrate);
    int baudrate();

    bool isOpen();

    bool isParity();
    void setParity(bool parity);
    void setDTR(bool val);
    void setRTS(bool val);

    bool close(bool is_user = true);
    void sendData(const QByteArray &data);

    ConnectionType lastType() { return m_lastType; }
    QString lastFileName() { return _lastFileName; }

signals:
    void closedEvent(bool duplex);
    void openedEvent(bool duplex);
    void receiveData(const QByteArray &data);
    void loggingStream(const QByteArray &data);

private:
    QSerialPort *m_serial = nullptr;
    QFile *m_file = nullptr;
    QUdpSocket *_socketUDP = nullptr;
    QTcpSocket *_socketTCP = nullptr;
    QTimer* _timerReconnection = nullptr;
    ConnectionType m_type = ConnectionNone;
    ConnectionType m_lastType = ConnectionNone;
    QString _lastFileName = "";

    bool m_isLogWrite = false;

    uint32_t _port = 0;
    QHostAddress _addr;

    QThread workerThread;

    void setType(ConnectionType type) {
        m_type = type;
        if(m_type != ConnectionNone) {
            m_lastType = m_type;

            if(m_lastType == ConnectionFile) {
                _lastFileName = m_file->fileName();
            } else {
                _lastFileName = "";
            }
        }
    }



private slots:
    void closing();
    void handleSerialError(QSerialPort::SerialPortError error);
    void readyReadSerial();
};

#endif // CONNECTION_H
