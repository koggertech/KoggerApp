#pragma once

#include <QAbstractListModel>

#include "link.h"
#include "link_defs.h"


class LinkListModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum class Roles {
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
        {{ static_cast<int>(LinkListModel::Roles::Uuid) },              {"Uuid"}},
        {{ static_cast<int>(LinkListModel::Roles::ConnectionStatus) },  {"ConnectionStatus"}},
        {{ static_cast<int>(LinkListModel::Roles::ReceivesData) },      {"ReceivesData"}},
        {{ static_cast<int>(LinkListModel::Roles::ControlType) },       {"ControlType"}},
        {{ static_cast<int>(LinkListModel::Roles::PortName) },          {"PortName"}},
        {{ static_cast<int>(LinkListModel::Roles::Baudrate) },          {"Baudrate"}},
        {{ static_cast<int>(LinkListModel::Roles::Parity) },            {"Parity"}},
        {{ static_cast<int>(LinkListModel::Roles::LinkType) },          {"LinkType"}},
        {{ static_cast<int>(LinkListModel::Roles::Address) },           {"Address"}},
        {{ static_cast<int>(LinkListModel::Roles::SourcePort) },        {"SourcePort"}},
        {{ static_cast<int>(LinkListModel::Roles::DestinationPort) },   {"DestinationPort"}},
        {{ static_cast<int>(LinkListModel::Roles::IsPinned) },          {"IsPinned"}},
        {{ static_cast<int>(LinkListModel::Roles::IsHided) },           {"IsHided"}},
        {{ static_cast<int>(LinkListModel::Roles::IsNotAvailable) },    {"IsNotAvailable"}},
        {{ static_cast<int>(LinkListModel::Roles::AutoSpeedSelection) },{"AutoSpeedSelection"}},
        {{ static_cast<int>(LinkListModel::Roles::IsUpgradingState) },  {"IsUpgradingState"}}
    };
    QHash<int, QVector<QVariant>> vectors_; // first - roleName, second - vec of vals
    QHash<QUuid, int> index_; // first - uuid, second - row
    int size_;

signals:
    void appendModifyEvent(QUuid uuid, bool connectionStatus, bool receivesData, ControlType controlType, const QString& portName,
                        int baudrate, bool parity, LinkType linkType, const QString& address, int sourcePort, int destinationPort,
                        bool isPinned, bool isHided, bool isNotAvailable, bool autoSpeedSelection, bool isUpgradingSate);
    void removeEvent(QUuid uuid);
};
