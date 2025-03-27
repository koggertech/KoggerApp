#pragma once

#include <memory>
#include <QObject>
#include <QList>
#include <QThread>
#include <QPair>
#include <QUuid>
#include "Link.h"
#include "LinkManager.h"
#include "LinkListModel.h"


class LinkManagerWrapper : public QObject // wrapper for LinkManager in main thread
{
    Q_OBJECT
public:
    Q_PROPERTY(LinkListModel* linkListModel READ getModelPtr NOTIFY modelChanged)

    /*methods*/
    LinkManagerWrapper(QObject* parent);
    ~LinkManagerWrapper();

    LinkListModel* getModelPtr();
    LinkManager* getWorker();
    void closeOpenedLinks();
    void openClosedLinks();

public slots:

// #ifdef MOTOR
//     void openAsUdp(QUuid uuid, QString address, int sourcePort, int destinationPort, bool isMotorDevice = false);
//     void openAsSerial(QUuid uuid, bool isMotorDevice = false);
// #else
//     void openAsSerial(QUuid uuid);
//     void createAsUdp(QString address, int sourcePort, int destinationPort);
// #endif

    void openAsSerial(QUuid uuid, int attribute = 0);
    void createAsUdp(QString address, int sourcePort, int destinationPort);
    void openAsUdp(QUuid uuid, QString address, int sourcePort, int destinationPort, int attribute = 0);
    void createAsTcp(QString address, int sourcePort, int destinationPort);
    void openAsTcp(QUuid uuid, QString address, int sourcePort, int destinationPort, int attribute = 0);
    void closeLink(QUuid uuid);
    void closeFLink(QUuid uuid);
    void deleteLink(QUuid uuid);
    void updateBaudrate(QUuid uuid, int baudrate);
    void appendModifyModelData(QUuid uuid, bool connectionStatus, ControlType controlType, QString portName, int baudrate, bool parity,
                         LinkType linkType, QString address, int sourcePort, int destinationPort, bool isPinned, bool isHided, bool isNotAvailable);
    void deleteModelData(QUuid uuid);

signals:
    void modelChanged(); // Q_PROPERTY in .h

// #ifdef MOTOR
//     void sendOpenAsSerial(QUuid uuid, bool isMotorDevice = false);
//     void sendOpenAsUdp(QUuid uuid, QString address, int sourcePort, int destinationPort, bool isMotorDevice = false);
// #else
//     void sendOpenAsSerial(QUuid uuid);
//     void sendOpenAsUdp(QUuid uuid, QString address, int sourcePort, int destinationPort);
// #endif

    void sendOpenAsSerial(QUuid uuid, int attribute = 0);
    void sendCreateAsUdp(QString address, int sourcePort, int destinationPort);
    void sendOpenAsUdp(QUuid uuid, QString address, int sourcePort, int destinationPort, int attribute = 0);
    void sendCreateAsTcp(QString address, int sourcePort, int destinationPort);
    void sendOpenAsTcp(QUuid uuid, QString address, int sourcePort, int destinationPort, int attribute = 0);
    void sendCloseLink(QUuid uuid);
    void sendFCloseLink(QUuid uuid);
    void sendDeleteLink(QUuid uuid);
    void sendUpdateBaudrate(QUuid uuid, int baudrate);
    void sendUpdateAddress(QUuid uuid, QString address);
    void sendUpdateSourcePort(QUuid uuid, int sourcePort);
    void sendUpdateDestinationPort(QUuid uuid, int destinationPort);
    void sendUpdatePinnedState(QUuid uuid, bool state);
    void sendUpdateControlType(QUuid uuid, int controlType);
    void sendStopTimer();
    void sendOpenFLinks();
    void sendCreateAndOpenAsUdpProxy(QString address, int sourcePort, int destinationPort);
    void sendCloseUdpProxy();

private:
    /*data*/
    std::unique_ptr<QThread> workerThread_;
    std::unique_ptr<LinkManager> workerObject_;
    LinkListModel model_;
    QList<QPair<QUuid, LinkType>> forceClosedLinks_;
};
