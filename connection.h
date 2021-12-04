#ifndef CONNECTION_H
#define CONNECTION_H

#include <QObject>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QFile>
#include <QDataStream>
#include <console.h>
#include <QAbstractSocket>
#include <QUdpSocket>
#include <QTcpSocket>
#include <QHostAddress>
#include <QTimer>
#include <QThread>


class Connection : public QObject
{
    Q_OBJECT
public:
    Connection();

    typedef enum {
        ConnectionNone,
        ConnectionSerial,
        ConnectionFile,
        ConnectionIP
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

signals:
    void closedEvent(bool duplex);
    void openedEvent(bool duplex);
    void receiveData(const QByteArray &data);
    void loggingStream(const QByteArray &data);

private:
    QSerialPort *m_serial = nullptr;
    QFile *m_file = nullptr;
    QUdpSocket *_socket = nullptr;
    QTimer* _timerReconnection = nullptr;
    ConnectionType m_type = ConnectionNone;

    bool m_isLogWrite = false;

    QThread workerThread;

private slots:
    void closing();
    void handleSerialError(QSerialPort::SerialPortError error);
    void readyReadSerial();

};

#endif // CONNECTION_H
