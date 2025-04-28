#include "link_list_model.h"


LinkListModel::LinkListModel(QObject* parent) :
    QAbstractListModel(parent),
    size_(0)
{
    connect(this, &LinkListModel::appendModifyEvent, this, &LinkListModel::doAppendModify);
    connect(this, &LinkListModel::removeEvent, this, &LinkListModel::doRemove);
}

QVariant LinkListModel::data(const QModelIndex &index, int role) const
{
    const int indexRow = index.row();
    QVector<QVariant> vectorRole = vectors_[role];

    if (indexRow < 0 || vectorRole.size() <= indexRow)
        return {"No data"};

    return vectors_[role][indexRow];
}

QHash<int, QByteArray> LinkListModel::roleNames() const
{
    return roleNames_;
}

Q_INVOKABLE int LinkListModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent)
    return size_;
}

void LinkListModel::clear()
{
    beginResetModel();
    vectors_.clear();
    index_.clear();
    size_ = 0;
    endResetModel();
}

QList<QPair<QUuid, LinkType>> LinkListModel::getOpenedUuids() const
{
    QList<QPair<QUuid, ::LinkType>> retVal;

    for (auto it = index_.cbegin(); it != index_.cend(); ++it) {
        int line = it.value();
        if (vectors_[LinkListModel::ConnectionStatus][line].toBool()) {
            retVal.append(qMakePair(it.key(), static_cast<::LinkType>(vectors_[LinkListModel::LinkType][line].toInt())));
        }
    }

    return retVal;
}

int LinkListModel::getSize() const
{
    return size_;
}

void LinkListModel::doAppendModify(QUuid uuid, bool connectionStatus, bool receivesData, ::ControlType controlType, const QString& portName,
                                   int baudrate, bool parity, ::LinkType linkType, const QString& address, int sourcePort, int destinationPort,
                                   bool isPinned, bool isHided, bool isNotAvailable, bool autoSpeedSelection, bool isUpgradingState)
{
    if (isHided)
        return;

    if (!index_.contains(uuid)) {
        const int line = rowCount();
        beginInsertRows(QModelIndex(), line, line);

        index_[uuid] = line;

        vectors_[LinkListModel::Uuid].append(uuid);
        vectors_[LinkListModel::ConnectionStatus].append(connectionStatus);
        vectors_[LinkListModel::ReceivesData].append(receivesData);
        vectors_[LinkListModel::ControlType].append(static_cast<int>(controlType));
        vectors_[LinkListModel::PortName].append(portName);
        vectors_[LinkListModel::Baudrate].append(baudrate);
        vectors_[LinkListModel::Parity].append(parity);
        vectors_[LinkListModel::LinkType].append(static_cast<int>(linkType));
        vectors_[LinkListModel::Address].append(address);
        vectors_[LinkListModel::SourcePort].append(sourcePort);
        vectors_[LinkListModel::DestinationPort].append(destinationPort);
        vectors_[LinkListModel::IsPinned].append(isPinned);
        vectors_[LinkListModel::IsHided].append(isHided);
        vectors_[LinkListModel::IsNotAvailable].append(isNotAvailable);
        vectors_[LinkListModel::AutoSpeedSelection].append(autoSpeedSelection);
        vectors_[LinkListModel::IsUpgradingState].append(isUpgradingState);

        ++size_;
        endInsertRows();
    }
    else {
        int line = index_[uuid];

        vectors_[LinkListModel::Uuid][line] = uuid;
        vectors_[LinkListModel::ConnectionStatus][line] = connectionStatus;
        vectors_[LinkListModel::ReceivesData][line] = receivesData;
        vectors_[LinkListModel::ControlType][line] = static_cast<int>(controlType);
        vectors_[LinkListModel::PortName][line] = portName;
        vectors_[LinkListModel::Baudrate][line] = baudrate;
        vectors_[LinkListModel::Parity][line] = parity;
        vectors_[LinkListModel::LinkType][line] = static_cast<int>(linkType);
        vectors_[LinkListModel::Address][line] = address;
        vectors_[LinkListModel::SourcePort][line] = sourcePort;
        vectors_[LinkListModel::DestinationPort][line] = destinationPort;
        vectors_[LinkListModel::IsPinned][line] = isPinned;
        vectors_[LinkListModel::IsHided][line] = isHided;
        vectors_[LinkListModel::IsNotAvailable][line] = isNotAvailable;
        vectors_[LinkListModel::AutoSpeedSelection][line] = autoSpeedSelection;
        vectors_[LinkListModel::IsUpgradingState][line] = isUpgradingState;

        emit dataChanged(index(line, 0), index(line, 0));
    }
}

void LinkListModel::doRemove(QUuid uuid)
{
    if (index_.contains(uuid)) {
        int line = index_[uuid];

        beginRemoveRows(QModelIndex(), line, line);

        vectors_[LinkListModel::Uuid].erase(vectors_[LinkListModel::Uuid].begin() + line);
        vectors_[LinkListModel::ConnectionStatus].erase(vectors_[LinkListModel::ConnectionStatus].begin() + line);
        vectors_[LinkListModel::ReceivesData].erase(vectors_[LinkListModel::ReceivesData].begin() + line);
        vectors_[LinkListModel::ControlType].erase(vectors_[LinkListModel::ControlType].begin() + line);
        vectors_[LinkListModel::PortName].erase(vectors_[LinkListModel::PortName].begin() + line);
        vectors_[LinkListModel::Baudrate].erase(vectors_[LinkListModel::Baudrate].begin() + line);
        vectors_[LinkListModel::Parity].erase(vectors_[LinkListModel::Parity].begin() + line);
        vectors_[LinkListModel::LinkType].erase(vectors_[LinkListModel::LinkType].begin() + line);
        vectors_[LinkListModel::Address].erase(vectors_[LinkListModel::Address].begin() + line);
        vectors_[LinkListModel::SourcePort].erase(vectors_[LinkListModel::SourcePort].begin() + line);
        vectors_[LinkListModel::DestinationPort].erase(vectors_[LinkListModel::DestinationPort].begin() + line);
        vectors_[LinkListModel::IsPinned].erase(vectors_[LinkListModel::IsPinned].begin() + line);
        vectors_[LinkListModel::IsHided].erase(vectors_[LinkListModel::IsHided].begin() + line);
        vectors_[LinkListModel::IsNotAvailable].erase(vectors_[LinkListModel::IsNotAvailable].begin() + line);
        vectors_[LinkListModel::AutoSpeedSelection].erase(vectors_[LinkListModel::AutoSpeedSelection].begin() + line);
        vectors_[LinkListModel::IsUpgradingState].erase(vectors_[LinkListModel::IsUpgradingState].begin() + line);

        index_.remove(uuid);
        for (auto it = index_.begin(); it != index_.end(); ++it) {
            if (it.value() > line)
                --it.value();
        }

        --size_;

        endRemoveRows();
    }
}
