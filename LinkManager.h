#pragma once

#include <memory>
#include <QObject>
#include <QString>
#include <QUuid>
#include <QList>
#include <QTimer>
#if defined(Q_OS_ANDROID)
#include "qtandroidserialport/src/qserialport.h"
#include "qtandroidserialport/src/qserialportinfo.h"
#else
#include <QSerialPort>
#include <QSerialPortInfo>
#endif
#include "Link.h"
#include "ProtoBinnary.h"


class LinkManager : public QObject
{
    Q_OBJECT

public:
    explicit LinkManager(QObject *parent = nullptr);

public slots:
    void onLinkConnectionStatusChanged(QUuid uuid);
    void createAndStartTimer();
    void stopTimer();
    void onExpiredTimer();

    void openAsSerial(QUuid uuid, int attribute = 0);

// #ifdef MOTOR
//     void openAsSerial(QUuid uuid, bool isMotorDevice = false);
// #else
//     void openAsSerial(QUuid uuid);
// #endif

    void openAsUdp(QUuid uuid, QString address, int sourcePort, int destinationPort, int attribute = 0);
    void openAsTcp(QUuid uuid, QString address, int sourcePort, int destinationPort, int attribute = 0);
    void closeLink(QUuid uuid);
    void closeFLink(QUuid uuid);
    void deleteLink(QUuid uuid);
    void updateBaudrate(QUuid uuid, int baudrate);
    void updateAddress(QUuid uuid, const QString& address);
    void updateSourcePort(QUuid uuid,int sourcePort);
    void updateDestinationPort(QUuid uuid,int destinationPort);
    void updatePinnedState(QUuid uuid, bool state);
    void updateControlType(QUuid uuid, ControlType controlType);
    void frameInput(Link* link, FrameParser frame);
    void createAsUdp(QString address, int sourcePort, int destinationPort);
    void createAsTcp(QString address, int sourcePort, int destinationPort);
    void importPinnedLinksFromXML();
    void openFLinks();
    void createAndOpenAsUdpProxy(QString address, int sourcePort, int destinationPort);
    void closeUdpProxy();

signals:
    void appendModifyModel(QUuid uuid, bool connectionStatus, ControlType controlType, QString portName, int baudrate, bool parity,
                           LinkType linkType, QString address, int sourcePort, int destinationPort, bool isPinned, bool isHided, bool isNotAvailable);
    void deleteModel(QUuid uuid);
    void frameReady(QUuid uuid, Link* link, FrameParser frame);
    void linkClosed(QUuid uuid, Link* link);
    void linkOpened(QUuid uuid, Link* link);
    void linkDeleted(QUuid uuid, Link* link);

private:
    /*structures*/
    class TimerController {
    public:
        explicit TimerController(QTimer* timer);
        ~TimerController();
    private:
        QTimer* timer_;
    };

    /*methods*/
    QList<QSerialPortInfo> getCurrentSerialList() const;
    Link* createSerialPort(const QSerialPortInfo& serialInfo) const;
    void addNewLinks(const QList<QSerialPortInfo> &currSerialList);
    void deleteMissingLinks(const QList<QSerialPortInfo> &currSerialList);
    void openAutoConnections();
    void update();
    Link *getLinkPtr(QUuid uuid);
    void doEmitAppendModifyModel(Link* linkPtr);
    void exportPinnedLinksToXML();
    Link* createNewLink() const;
    void printLinkDebugInfo(Link* link) const;

    /*data*/
    QList<Link*> list_;
    std::unique_ptr<QTimer> timer_;
    static const int timerInterval_ = 500; // msecs
    QUuid proxyLinkUuid_;
    bool coldStarted_;
};
