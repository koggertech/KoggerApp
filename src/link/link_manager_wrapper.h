#pragma once

#include <memory>
#include <QObject>
#include <QList>
#include <QThread>
#include <QPair>
#include <QUuid>
#include "link_manager.h"
#include "link_list_model.h"


class LinkManagerWrapper : public QObject // wrapper for LinkManager in main thread
{
    Q_OBJECT
public:
    Q_PROPERTY(LinkListModel* linkListModel READ getModelPtr NOTIFY modelChanged)
    Q_PROPERTY(QVariant baudrateModel READ baudrateModel CONSTANT)

    /*methods*/
    LinkManagerWrapper(QObject* parent);
    ~LinkManagerWrapper();

    LinkListModel* getModelPtr();
    LinkManager* getWorker();
    void closeOpenedLinks();
    QHash<QUuid, QString> getLinkNames() const;
    void openClosedLinks();
    QVariant baudrateModel() const;

public slots:
    void openAsSerial(QUuid uuid, LinkAttribute attribute = LinkAttribute::kLinkAttributeNone);
    void createAsUdp(QString address, int sourcePort, int destinationPort);
    void openAsUdp(QUuid uuid, QString address, int sourcePort, int destinationPort, LinkAttribute attribute = LinkAttribute::kLinkAttributeNone);
    void createAsTcp(QString address, int sourcePort, int destinationPort);
    void openAsTcp(QUuid uuid, QString address, int sourcePort, int destinationPort, LinkAttribute attribute = LinkAttribute::kLinkAttributeNone);
    void closeLink(QUuid uuid);
    void closeFLink(QUuid uuid);
    void deleteLink(QUuid uuid);
    void updateBaudrate(QUuid uuid, int baudrate);
    void setRequestToSend(QUuid uuid, bool rts);
    void setDataTerminalReady(QUuid uuid, bool dtr);
    void setParity(QUuid uuid, bool parity);
    void setAttribute(QUuid uuid, LinkAttribute attribute);
    void appendModifyModelData(QUuid uuid, bool connectionStatus, bool receivesData, ControlType controlType, QString portName, int baudrate, bool parity,
                               LinkType linkType, QString address, int sourcePort, int destinationPort, bool isPinned, bool isHided, bool isNotAvailable,
                               bool autoSpeedSelection, bool isUpgradingState);
    void deleteModelData(QUuid uuid);
    QUuid getFirstOpened() { return getWorker()->getFirstOpend(); }
    Link* getLinkPtr(QUuid uuid) { return getWorker()->getLinkPtr(uuid); }

signals:
    void modelChanged(); // Q_PROPERTY in .h
    void sendOpenAsSerial(QUuid uuid, LinkAttribute attribute = LinkAttribute::kLinkAttributeNone);
    void sendCreateAsUdp(QString address, int sourcePort, int destinationPort);
    void sendOpenAsUdp(QUuid uuid, QString address, int sourcePort, int destinationPort, LinkAttribute attribute = LinkAttribute::kLinkAttributeNone);
    void sendCreateAsTcp(QString address, int sourcePort, int destinationPort);
    void sendOpenAsTcp(QUuid uuid, QString address, int sourcePort, int destinationPort, LinkAttribute attribute = LinkAttribute::kLinkAttributeNone);
    void sendCloseLink(QUuid uuid);
    void sendFCloseLink(QUuid uuid);
    void sendDeleteLink(QUuid uuid);
    void sendUpdateBaudrate(QUuid uuid, int baudrate);
    void sendSetRequestToSend(QUuid uuid, bool rts);
    void sendSetDataTerminalReady(QUuid uuid, bool dtr);
    void sendSetPatity(QUuid uuid, bool parity);
    void sendSetAttribut(QUuid uuid, LinkAttribute attribute);
    void sendUpdateAddress(QUuid uuid, QString address);
    void sendAutoSpeedSelection(QUuid uuid, bool state);
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
