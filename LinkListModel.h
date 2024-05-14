#pragma once

#include <QAbstractListModel>

#include "Link.h"


class LinkListModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Roles {
        Uuid,
        ConnectionStatus,
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
        IsNotAvailable
    };

    /*methods*/
    explicit LinkListModel(QObject* parent = nullptr);
    ~LinkListModel();

    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    Q_INVOKABLE int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    void clear();
    int size() const;

private:
    Q_DISABLE_COPY(LinkListModel)

    /*methods*/
    void doAppend(QUuid uuid, bool connectionStatus, ::ControlType controlType, const QString& portName, int baudrate, bool parity,
                  ::LinkType linkType, const QString& address, int sourcePort, int destinationPort, bool isPinned, bool isHided, bool isNotAvailable);
    void doRemove(QUuid uuid);

    /*data*/
    QHash<int, QByteArray> roleNames_ {
        {{LinkListModel::Uuid},             {"Uuid"}},
        {{LinkListModel::ConnectionStatus}, {"ConnectionStatus"}},
        {{LinkListModel::ControlType},      {"ControlType"}},
        {{LinkListModel::PortName},         {"PortName"}},
        {{LinkListModel::Baudrate},         {"Baudrate"}},
        {{LinkListModel::Parity},           {"Parity"}},
        {{LinkListModel::LinkType},         {"LinkType"}},
        {{LinkListModel::Address},          {"Address"}},
        {{LinkListModel::SourcePort},       {"SourcePort"}},
        {{LinkListModel::DestinationPort},  {"DestinationPort"}},
        {{LinkListModel::IsPinned},         {"IsPinned"}},
        {{LinkListModel::IsHided},          {"IsHided"}},
        {{LinkListModel::IsNotAvailable},   {"IsNotAvailable"}}
    };
    QHash<int, QVector<QVariant>> vectors_; // first - roleName, second - vec of vals
    QHash<QUuid, int> index_; // first - uuid, second - row
    QVector<int> roles_;
    int categories_;
    int size_;

signals:
    void appendEvent(QUuid uuid, bool connectionStatus, ::ControlType controlType, const QString& portName, int baudrate, bool parity,
                     ::LinkType linkType, const QString& address, int sourcePort, int destinationPort, bool isPinned, bool isHided, bool isNotAvailable);
    void removeEvent(QUuid uuid);
};
