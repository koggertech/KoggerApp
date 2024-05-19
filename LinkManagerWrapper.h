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
    std::shared_ptr<LinkManager> getWorker();

private:
    /*data*/
    std::unique_ptr<QThread> workerThread_;
    std::shared_ptr<LinkManager> workerObject_;
    LinkListModel model_;

signals:
    void modelChanged(); //

    void sendOpenAsSerial(QUuid uuid);

    void sendCreateAsUdp(QString address, int sourcePort, int destinationPort);
    void sendOpenAsUdp(QUuid uuid);
    void sendCreateAsTcp(QString address, int sourcePort, int destinationPort);
    void sendOpenAsTcp(QUuid uuid);

    void sendClose(QUuid uuid);

public slots:

    void openAsSerial(QUuid uuid);

    void createAsUdp(QString address, int sourcePort, int destinationPort);
    void openAsUdp(QUuid uuid);

    void createAsTcp(QString address, int sourcePort, int destinationPort);
    void openAsTcp(QUuid uuid);

    void close(QUuid uuid);

    // for model
    void appendModifyModelData(QUuid uuid, bool connectionStatus, ControlType controlType, QString portName, int baudrate, bool parity,
                         LinkType linkType, QString address, int sourcePort, int destinationPort, bool isPinned, bool isHided, bool isNotAvailable);
    void deleteModelData(QUuid uuid);
};
