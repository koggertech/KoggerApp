#pragma once

#include <QAbstractListModel>

#include "link.h"


class LinkListModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Roles {
        Uuid,
        ConnectionStatus,
        ReceivesData,
        ControlType,
        PortName,
        Baudrate,
        Parity,
        LinkType,
        Address,
        SourcePort,
        DestinationPort,
        IsPinned,
        IsHided,
        IsNotAvailable,
        AutoSpeedSelection,
        IsUpgradingState
    };

    /*methods*/
    explicit LinkListModel(QObject* parent = nullptr);

    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    Q_INVOKABLE int rowCount(const QModelIndex& parent = QModelIndex()) const override;

    int getSize() const;
    void clear();

    QList<QPair<QUuid, ::LinkType>> getOpenedUuids() const;
    QHash<QUuid, QString> getLinkNames() const;

private:
    Q_DISABLE_COPY(LinkListModel)

    /*methods*/
    void doAppendModify(QUuid uuid, bool connectionStatus, bool receivesData, ::ControlType controlType, const QString& portName,
                        int baudrate, bool parity, ::LinkType linkType, const QString& address, int sourcePort, int destinationPort,
                        bool isPinned, bool isHided, bool isNotAvailable, bool autoSpeedSelection, bool isUpgradingState);
    void doRemove(QUuid uuid);

    /*data*/
    QHash<int, QByteArray> roleNames_ {
        {{LinkListModel::Uuid},              {"Uuid"}},
        {{LinkListModel::ConnectionStatus},  {"ConnectionStatus"}},
        {{LinkListModel::ReceivesData},      {"ReceivesData"}},
        {{LinkListModel::ControlType},       {"ControlType"}},
        {{LinkListModel::PortName},          {"PortName"}},
        {{LinkListModel::Baudrate},          {"Baudrate"}},
        {{LinkListModel::Parity},            {"Parity"}},
        {{LinkListModel::LinkType},          {"LinkType"}},
        {{LinkListModel::Address},           {"Address"}},
        {{LinkListModel::SourcePort},        {"SourcePort"}},
        {{LinkListModel::DestinationPort},   {"DestinationPort"}},
        {{LinkListModel::IsPinned},          {"IsPinned"}},
        {{LinkListModel::IsHided},           {"IsHided"}},
        {{LinkListModel::IsNotAvailable},    {"IsNotAvailable"}},
        {{LinkListModel::AutoSpeedSelection},{"AutoSpeedSelection"}},
        {{LinkListModel::IsUpgradingState},  {"IsUpgradingState"}}
    };
    QHash<int, QVector<QVariant>> vectors_; // first - roleName, second - vec of vals
    QHash<QUuid, int> index_; // first - uuid, second - row
    int size_;

signals:
    void appendModifyEvent(QUuid uuid, bool connectionStatus, bool receivesData, ::ControlType controlType, const QString& portName,
                        int baudrate, bool parity, ::LinkType linkType, const QString& address, int sourcePort, int destinationPort,
                        bool isPinned, bool isHided, bool isNotAvailable, bool autoSpeedSelection, bool isUpgradingSate);
    void removeEvent(QUuid uuid);
};
