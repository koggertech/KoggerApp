#pragma once

#include <QObject>
#include <QIODevice>
#include <QByteArray>
#include <QUuid>
#include <QString>
#include <QHostAddress>
#include <QUdpSocket>
#include <QTcpSocket>
#if defined(Q_OS_ANDROID)
#include "qtandroidserialport/src/qserialport.h"
#include "qtandroidserialport/src/qserialportinfo.h"
#else
#include <QSerialPort>
#include <QSerialPortInfo>
#endif
#include "ProtoBinnary.h"

using namespace Parsers;


typedef enum {
    LinkNone,
    LinkSerial,
    LinkIPUDP, // also is proxy
    LinkIPTCP,
} LinkType;

typedef enum {
    LinkAttributeNone,
    LinkAttributeMotor = 2
} LinkAttribute;

typedef enum {
    kManual = 0,
    kAuto,
    kAutoOnce
} ControlType;

class Link : public QObject
{
    Q_OBJECT

public:
    Link();
    void createAsSerial(const QString& portName, int baudrate, bool parity);
    void openAsSerial();
    void createAsUdp(const QString& address, int sourcePort, int destinationPort);
    void updateUdpParameters(const QString& address, int sourcePort, int destinationPort);
    void openAsUdp();
    void createAsTcp(const QString& address, int sourcePort, int destinationPort);
    void updateTcpParameters(const QString& address, int sourcePort, int destinationPort);
    void openAsTcp();
    bool isOpen() const;
    void close();
    bool parse();
    FrameParser* frameParser();
    QIODevice* device();
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
    void setIsProxy(bool isProxy);
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
    bool        getIsProxy() const;
    bool        getIsForceStopped() const;

// #ifdef MOTOR
    void        setAttribute(int attribute) { attribute_ = attribute; }
    bool        getIsMotorDevice() { return attribute_ == LinkAttributeMotor; }
// #endif


public slots:
    bool writeFrame(FrameParser frame);
    bool write(QByteArray data);

signals:
    void readyParse(Link* link);
    void connectionStatusChanged(QUuid uuid);
    void frameReady(QUuid uuid, Link* link, FrameParser frame);
    void opened(QUuid uuid, Link* linkPtr);
    void closed(QUuid uuid, Link* link);

#ifdef MOTOR
    void dataReady(QByteArray data);
#else
    void dataReady();
#endif

private:
    /*methods*/
    void setDev(QIODevice* dev);
    void deleteDev();
    void toParser(const QByteArray data);

    /*data*/
    QIODevice* ioDevice_;
    FrameParser frame_;
    QByteArray context_;
    QByteArray buffer_;
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
    bool isProxy_;
    bool isForcedStopped_;


    int attribute_ = 0;
// #ifdef MOTOR
//     bool isMotorDevice_ = false;
// #endif

private slots:
    void readyRead();
    void aboutToClose();
    void handleSerialError(QSerialPort::SerialPortError error);
};
