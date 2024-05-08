#pragma once

#include <QAbstractListModel>

#include "Link.h"


class LinkListModel : public QAbstractListModel
{
    Q_OBJECT
public:
    LinkListModel(QObject* parent = nullptr);

    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE int rowCount(const QModelIndex& parent = QModelIndex()) const override {
        Q_UNUSED(parent)
        return _size;
    }


    enum Roles {
        ConnectionStatus,
        ControlType,
        PortName,
        Baudrate,
        Parity,
        LinkType,
        Address,
        SourcePort,
        DestinationPort,
        isPinned,
        isHided,
        isNotAvailable
    };

    void clear() {
        beginResetModel();
        _vectors.clear();
        _size = 0;
        endResetModel();
    }

    int size() {
        return _size;
    }

signals:
    void appendEvent(bool connectionStatus, ::ControlType controlType, const QString& portName, int baudrate, bool parity,
                     ::LinkType linkType, const QString& address, int sourcePort, int destinationPort, bool isPinned, bool isHided, bool isNotAvailable);

private:
    Q_DISABLE_COPY(LinkListModel)

    int _size = 0;
    int _categories = 0;

    QVector<int> _roles;

    QHash<int, QByteArray> _roleNames {
        {{LinkListModel::ConnectionStatus}, {"connectionStatus"}},
        {{LinkListModel::ControlType}, {"controlType"}},
        {{LinkListModel::PortName}, {"portName"}},
        {{LinkListModel::Baudrate}, {"baudrate"}},
        {{LinkListModel::Parity}, {"parity"}},
        {{LinkListModel::LinkType}, {"linkType"}},
        {{LinkListModel::Address}, {"address"}},
        {{LinkListModel::SourcePort}, {"sourcePort"}},
        {{LinkListModel::DestinationPort}, {"destinationPort"}},
        {{LinkListModel::isPinned}, {"isPinned"}},
        {{LinkListModel::isHided}, {"isHided"}},
        {{LinkListModel::isNotAvailable}, {"isNotAvailable"}}
    };

    QHash<int, QVector<QVariant>> _vectors;
    QHash<int, int> _index;


    void doAppend(bool connectionStatus, ::ControlType controlType, const QString& portName, int baudrate, bool parity,
                  ::LinkType linkType, const QString& address, int sourcePort, int destinationPort, bool isPinned, bool isHided, bool isNotAvailable);
};
