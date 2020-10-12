#ifndef CONNECTION_H
#define CONNECTION_H

#include <QObject>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QFile>
#include <QDataStream>
#include <console.h>

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
    bool openSerial(const QString &name, int32_t baudrate);
    bool openFile(const QString &name);

    bool isOpen();
    bool close();
    void sendData(const QByteArray &data);

signals:
    void closedEvent(bool duplex);
    void openedEvent(bool duplex);
    void receiveData(const QByteArray &data);

private:
    QSerialPort *m_serial = nullptr;
    QFile *m_file = nullptr;
    ConnectionType m_type = ConnectionNone;


private slots:
    void closing();
    void handleSerialError(QSerialPort::SerialPortError error);
    void readyReadSerial();

};

#endif // CONNECTION_H
