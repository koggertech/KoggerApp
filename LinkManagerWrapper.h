#pragma once

#include <QHash>
#include <QList>
#include <QThread>
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

private:
    /*data*/
    std::unique_ptr<QThread> workerThread_;
    std::unique_ptr<LinkManager> workerObject_;
    LinkListModel model_;

signals:
    void modelChanged(); // Q_PROPERTY in .h
    void sendOpenAsSerial(QUuid uuid);
    void sendCreateAsUdp(QString address, int sourcePort, int destinationPort);
    void sendOpenAsUdp(QUuid uuid, QString address, int sourcePort, int destinationPort);
    void sendCreateAsTcp(QString address, int sourcePort, int destinationPort);
    void sendOpenAsTcp(QUuid uuid, QString address, int sourcePort, int destinationPort);
    void sendCloseLink(QUuid uuid);
    void sendDeleteLink(QUuid uuid);
    void sendUpdateBaudrate(QUuid uuid, int baudrate);
    void sendUpdateAddress(QUuid uuid, QString address);
    void sendUpdateSourcePort(QUuid uuid, int sourcePort);
    void sendUpdateDestinationPort(QUuid uuid, int destinationPort);
    void sendUpdatePinnedState(QUuid uuid, bool state);

public slots:
    void openAsSerial(QUuid uuid);
    void createAsUdp(QString address, int sourcePort, int destinationPort);
    void openAsUdp(QUuid uuid, QString address, int sourcePort, int destinationPort);
    void createAsTcp(QString address, int sourcePort, int destinationPort);
    void openAsTcp(QUuid uuid, QString address, int sourcePort, int destinationPort);
    void closeLink(QUuid uuid);
    void deleteLink(QUuid uuid);
    void updateBaudrate(QUuid uuid, int baudrate);
    void appendModifyModelData(QUuid uuid, bool connectionStatus, ControlType controlType, QString portName, int baudrate, bool parity,
                         LinkType linkType, QString address, int sourcePort, int destinationPort, bool isPinned, bool isHided, bool isNotAvailable);
    void deleteModelData(QUuid uuid);
};
