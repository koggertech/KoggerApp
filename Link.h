#ifndef LINK_H
#define LINK_H

#include <ProtoBinnary.h>

#include <QObject>
#include <QIODevice>
#include <QByteArray>
#include <QQueue>
#include <QMutex>
#include <QThread>
#include <QUuid>
#include <QUdpSocket>
#include <QTcpSocket>
#include <QHostAddress>

#if defined(Q_OS_ANDROID)
#include "qtandroidserialport/src/qserialport.h"
#include "qtandroidserialport/src/qserialportinfo.h"
#else
#include <QSerialPort>
#include <QSerialPortInfo>
#endif

using namespace Parsers;


typedef enum {
    LinkNone,
    LinkSerial,
    LinkIPUDP,
    LinkIPTCP,
} LinkType;

typedef enum {
    kManual = 0,
    kAuto,
    kAutoOnce
} ControlType;

class Link : public QObject {
    Q_OBJECT
public:
    Link();
    Link(QString uuidStr, ControlType controlType, LinkType linkType, QString portName, int baudrate, bool parity, QString address,
         int sourcePort, int destinationPort, bool isPinned, bool isHided, bool isNotAvailable);

    void createAsSerial(const QString& portName, int baudrate, bool parity);
    void openAsSerial();

    void createAsUdp(const QString& address, int sourcePort, int destinationPort);
    void updateUdpParameters(const QString& address, int sourcePort, int destinationPort);
    void openAsUdp();

    void createAsTcp(const QString& address, int sourcePort, int destinationPort);
    void updateTcpParameters(const QString& address, int sourcePort, int destinationPort);
    void openAsTcp();

    void openAsUDP(const QString &address, const int port_in,  const int port_out); //

    bool isOpen() const;
    void close();
    bool parse();

    FrameParser* frameParser() { return &_frame; }
    QIODevice* device() { return ioDevice_; }

    void setUuid(QUuid uuid);
    void setConnectionStatus(bool connectionStatus);
    void setControlType(ControlType controlType);
    void setPortName(const QString& portName);
    void setBaudrate(int baudrate);
    void setParity(bool parity);
    void setLinkType(LinkType linkType);
    void setAddress(const QString& address);
    void setSourcePort(int sourcePort);
    void setDestinationPort(int destinationPort);
    void setIsPinned(bool isPinned);
    void setIsHided(bool isHided);
    void setIsNotAvailable(bool isNotAvailable);

    void setIsForceStopped(bool isForcedStopped);

    QUuid       getUuid() const;
    bool        getConnectionStatus() const;
    ControlType getControlType() const;
    QString     getPortName() const;
    int         getBaudrate() const;
    bool        getParity() const;
    LinkType    getLinkType() const;
    QString     getAddress() const;
    int         getSourcePort() const;
    int         getDestinationPort() const;
    bool        getIsPinned() const;
    bool        getIsHided() const;
    bool        getIsNotAvailable() const;

    bool        getIsForceStopped() const;

public slots:
    bool writeFrame(FrameParser frame);
    bool write(QByteArray data);

private slots:
    void readyRead();
    void aboutToClose();

private:
    /*methods*/
    void setDev(QIODevice* dev);
    void deleteDev();
    void toParser(const QByteArray data);

    /*data*/
    QMutex _mutex;
    FrameParser _frame;
    QIODevice* ioDevice_ = nullptr;
    QByteArray _context;
    QByteArray _buffer;
    QHostAddress hostAddress_;

    QUuid uuid_;
    ControlType controlType_;
    LinkType linkType_;
    QString portName_;
    int baudrate_;
    bool parity_;
    QString address_;
    int sourcePort_;
    int destinationPort_;
    bool isPinned_;
    bool isHided_;
    bool isNotAvailable_;

    bool isForcedStopped_;

signals:
    void readyParse(Link* link);
    // void changeState();
    void connectionStatusChanged(QUuid uuid);
    void frameReady(QUuid uuid, Link* link, FrameParser frame);
    void opened(QUuid uuid, Link* linkPtr);
    void closed(QUuid uuid, Link* link);
    void dataReady();
};


#endif // LINK_H
