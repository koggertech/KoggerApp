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

    void createAsSerial(const QString &portName, int baudrate, bool parity);
    void openAsSerial();

    void createAsUdp(const QString &address, int sourcePort, int destinationPort);
    void openAsUdp();

    void createAsTcp(const QString &address, int sourcePort, int destinationPort);
    void openAsTcp();

    void openAsUDP(const QString &address, const int port_in,  const int port_out); //

    bool isOpen() const;
    void close();
    bool parse();

    FrameParser* frameParser() { return &_frame; }
    QIODevice* device() { return ioDevice_; }


    /*multi link*/
    QUuid getUuid() const;
    bool getConnectionStatus() const;
    ControlType getControlType() const;
    /*Serial*/
    QString getPortName() const;
    int getBaudrate() const;
    bool getParity() const;
    /*UDP/TCP*/
    LinkType getLinkType() const;
    QString getAddress() const;
    int getSourcePort() const;
    int getDestinationPort() const;
    /*other*/
    bool isPinned() const;
    bool isHided() const;
    bool isNotAvailable() const;
    /**/


public slots:
    bool writeFrame(FrameParser frame);
    bool write(QByteArray data);

private slots:

    void readyRead();
    void aboutToClose();

private:
    QMutex _mutex;
    FrameParser _frame;

    QIODevice* ioDevice_ = nullptr;
    QHostAddress hostAddress_;

    QByteArray _context;
    QByteArray _buffer;

    LinkType type_ = LinkNone;


    /*multi link*/
    QUuid uuid_;

    ControlType controlType_;
    /*Serial*/
    QString portName_;
    int baudrate_;
    bool parity_;
    /*UDP/TCP*/
    LinkType linkType_;
    QString address_;
    int sourcePort_;
    int destinationPort_;
    /*others*/
    bool isPinned_;
    bool isHided_;
    bool isNotAvailable_;
    /**/

    void setType(LinkType type) { type_ = type; }
    void setDev(QIODevice* dev);
    void deleteDev();

    void toParser(const QByteArray data);

signals:
    void readyParse(Link* link);
    // void changeState();
    void connectionStatusChanged(QUuid uuid);
    void closed(Link* link);
    void frameReady(Link* link, FrameParser frame);
    void dataReady();
};


#endif // LINK_H
