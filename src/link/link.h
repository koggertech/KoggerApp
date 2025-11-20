#pragma once

#include <QObject>
#include <QIODevice>
#include <QByteArray>
#include <QUuid>
#include <QString>
#include <QHostAddress>
#include <QUdpSocket>
#include <QTcpSocket>
#include <QPointer>
#if defined(Q_OS_ANDROID)
#include "qserialport.h"
#include "qserialportinfo.h"
#else
#include <QSerialPort>
#include <QSerialPortInfo>
#endif
#include <QTimer>

#include "link_defs.h"
#include "proto_binnary.h"

using namespace Parsers;

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
    void setRequestToSend(bool rts);
    void setDataTerminalReady(bool dtr);
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
    void setAutoSpeedSelection(bool autoSpeedSelection);
    void setIsUpgradingState(bool state);
    void setAutoConnOnce(bool state);
    QUuid       getUuid() const;
    bool        getConnectionStatus() const;
    bool        getIsRecievesData() const;
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
    bool        getAutoSpeedSelection() const;
    bool        getIsUpgradingState() const;
    bool        getAutoConnOnce() const;
    void        setAttribute(LinkAttribute attribute); // for link type (sonar, motor, etc.)
    LinkAttribute attribute() { return attribute_; }

public slots:
    bool writeFrame(Parsers::FrameParser frame);
    bool write(QByteArray data);
    void onStartUpgradingFirmware();
    void onUpgradingFirmwareDone();

signals:
    void readyParse(Link* link);
    void connectionStatusChanged(QUuid uuid);
    void frameReady(QUuid uuid, Link* link, Parsers::FrameParser frame);
    void opened(QUuid uuid, Link* linkPtr);
    void closed(QUuid uuid, Link* link);
    void baudrateChanged(QUuid uuid);
    void isReceivesDataChanged(QUuid uuid);
    void sendDoRequestAll(QUuid uuid);
    void upgradingFirmwareStateChanged(QUuid uuid);
    void dataReady(QByteArray data);

private slots:
    void onCheckedTimerEnd();

private:
    /*methods*/
    void setDev(QIODevice* dev);
    void deleteDev();
    void toParser(const QByteArray data);
    void resetLastSearchIndx();

    /*data*/
    QPointer<QIODevice> ioDevice_;
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
    LinkAttribute attribute_;
    bool autoSpeedSelection_;
    std::unique_ptr<QTimer> checkTimer_;
    int timeoutCnt_;
    uint32_t lastTotalCnt_;
    bool isReceivesData_;
    QList<uint32_t> baudrateSearchList_;
    int lastSearchIndx_;
    bool onUpgradingFirmware_;
    int localGhostIgnoreCount_;
    int requestCnt_;
    bool autoConnOnce_;

private slots:
    void readyRead();
    void aboutToClose();
    void handleSerialError(QSerialPort::SerialPortError error);
};
