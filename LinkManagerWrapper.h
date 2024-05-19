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
    void sendOpenAsUdp(QUuid uuid);
    void sendOpenAsTcp(QUuid uuid);

    void sendClose(QUuid uuid);

public slots:
    void openAsSerial(QUuid uuid);
    void openAsUdp(QUuid uuid);
    void openAsTcp(QUuid uuid);

    void close(QUuid uuid);

    void appendModifyModelData(QUuid uuid, bool connectionStatus, ControlType controlType, QString portName, int baudrate, bool parity,
                         LinkType linkType, QString address, int sourcePort, int destinationPort, bool isPinned, bool isHided, bool isNotAvailable);
    void deleteModelData(QUuid uuid);
};
