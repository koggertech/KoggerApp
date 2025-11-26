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

QHash<QUuid, QString> LinkListModel::getLinkNames() const
{
    QHash<QUuid, QString> retVal;

    for (auto it = index_.cbegin(); it != index_.cend(); ++it) {
        int line = it.value();

        if (vectors_[static_cast<int>(LinkListModel::Roles::ConnectionStatus)][line].toBool()) {
            auto linkType = vectors_[static_cast<int>(LinkListModel::Roles::LinkType)][line].toUInt();

            if (linkType == 1) { // uart
                retVal[it.key()] = vectors_[static_cast<int>(LinkListModel::Roles::PortName)][line].toString();
            }
            if (linkType == 2 || linkType == 3) {
                QString type = (linkType == 2) ? "UDP" : "TCP";
                QString address = vectors_[static_cast<int>(LinkListModel::Roles::Address)][line].toString();
                retVal[it.key()] = type + "(" + address + ")";
            }
        }
    }

    return retVal;
}

QList<QPair<QUuid, LinkType>> LinkListModel::getOpenedUuids() const
{
    QList<QPair<QUuid, ::LinkType>> retVal;

    for (auto it = index_.cbegin(); it != index_.cend(); ++it) {
        int line = it.value();
        if (vectors_[static_cast<int>(LinkListModel::Roles::ConnectionStatus)][line].toBool()) {
            retVal.append(qMakePair(it.key(), static_cast<::LinkType>(vectors_[static_cast<int>(LinkListModel::Roles::LinkType)][line].toInt())));
        }
    }

    return retVal;
}

int LinkListModel::getSize() const
{
    return size_;
}

void LinkListModel::doAppendModify(QUuid uuid, bool connectionStatus, bool receivesData, ControlType controlType, const QString& portName,
                                   int baudrate, bool parity, LinkType linkType, const QString& address, int sourcePort, int destinationPort,
                                   bool isPinned, bool isHided, bool isNotAvailable, bool autoSpeedSelection, bool isUpgradingState)
{
    if (isHided)
        return;

    if (!index_.contains(uuid)) {
        const int line = rowCount();
        beginInsertRows(QModelIndex(), line, line);

        index_[uuid] = line;

        vectors_[static_cast<int>(LinkListModel::Roles::Uuid)].append(uuid);
        vectors_[static_cast<int>(LinkListModel::Roles::ConnectionStatus)].append(connectionStatus);
        vectors_[static_cast<int>(LinkListModel::Roles::ReceivesData)].append(receivesData);
        vectors_[static_cast<int>(LinkListModel::Roles::ControlType)].append(static_cast<int>(controlType));
        vectors_[static_cast<int>(LinkListModel::Roles::PortName)].append(portName);
        vectors_[static_cast<int>(LinkListModel::Roles::Baudrate)].append(baudrate);
        vectors_[static_cast<int>(LinkListModel::Roles::Parity)].append(parity);
        vectors_[static_cast<int>(LinkListModel::Roles::LinkType)].append(static_cast<int>(linkType));
        vectors_[static_cast<int>(LinkListModel::Roles::Address)].append(address);
        vectors_[static_cast<int>(LinkListModel::Roles::SourcePort)].append(sourcePort);
        vectors_[static_cast<int>(LinkListModel::Roles::DestinationPort)].append(destinationPort);
        vectors_[static_cast<int>(LinkListModel::Roles::IsPinned)].append(isPinned);
        vectors_[static_cast<int>(LinkListModel::Roles::IsHided)].append(isHided);
        vectors_[static_cast<int>(LinkListModel::Roles::IsNotAvailable)].append(isNotAvailable);
        vectors_[static_cast<int>(LinkListModel::Roles::AutoSpeedSelection)].append(autoSpeedSelection);
        vectors_[static_cast<int>(LinkListModel::Roles::IsUpgradingState)].append(isUpgradingState);

        ++size_;
        endInsertRows();
    }
    else {
        int line = index_[uuid];

        vectors_[static_cast<int>(LinkListModel::Roles::Uuid)][line] = uuid;
        vectors_[static_cast<int>(LinkListModel::Roles::ConnectionStatus)][line] = connectionStatus;
        vectors_[static_cast<int>(LinkListModel::Roles::ReceivesData)][line] = receivesData;
        vectors_[static_cast<int>(LinkListModel::Roles::ControlType)][line] = static_cast<int>(controlType);
        vectors_[static_cast<int>(LinkListModel::Roles::PortName)][line] = portName;
        vectors_[static_cast<int>(LinkListModel::Roles::Baudrate)][line] = baudrate;
        vectors_[static_cast<int>(LinkListModel::Roles::Parity)][line] = parity;
        vectors_[static_cast<int>(LinkListModel::Roles::LinkType)][line] = static_cast<int>(linkType);
        vectors_[static_cast<int>(LinkListModel::Roles::Address)][line] = address;
        vectors_[static_cast<int>(LinkListModel::Roles::SourcePort)][line] = sourcePort;
        vectors_[static_cast<int>(LinkListModel::Roles::DestinationPort)][line] = destinationPort;
        vectors_[static_cast<int>(LinkListModel::Roles::IsPinned)][line] = isPinned;
        vectors_[static_cast<int>(LinkListModel::Roles::IsHided)][line] = isHided;
        vectors_[static_cast<int>(LinkListModel::Roles::IsNotAvailable)][line] = isNotAvailable;
        vectors_[static_cast<int>(LinkListModel::Roles::AutoSpeedSelection)][line] = autoSpeedSelection;
        vectors_[static_cast<int>(LinkListModel::Roles::IsUpgradingState)][line] = isUpgradingState;

        emit dataChanged(index(line, 0), index(line, 0));
    }
}

void LinkListModel::doRemove(QUuid uuid)
{
    if (index_.contains(uuid)) {
        int line = index_[uuid];

        beginRemoveRows(QModelIndex(), line, line);

        vectors_[static_cast<int>(LinkListModel::Roles::Uuid)].erase(               vectors_[static_cast<int>(LinkListModel::Roles::Uuid)].begin() + line);
        vectors_[static_cast<int>(LinkListModel::Roles::ConnectionStatus)].erase(   vectors_[static_cast<int>(LinkListModel::Roles::ConnectionStatus)].begin() + line);
        vectors_[static_cast<int>(LinkListModel::Roles::ReceivesData)].erase(       vectors_[static_cast<int>(LinkListModel::Roles::ReceivesData)].begin() + line);
        vectors_[static_cast<int>(LinkListModel::Roles::ControlType)].erase(        vectors_[static_cast<int>(LinkListModel::Roles::ControlType)].begin() + line);
        vectors_[static_cast<int>(LinkListModel::Roles::PortName)].erase(           vectors_[static_cast<int>(LinkListModel::Roles::PortName)].begin() + line);
        vectors_[static_cast<int>(LinkListModel::Roles::Baudrate)].erase(           vectors_[static_cast<int>(LinkListModel::Roles::Baudrate)].begin() + line);
        vectors_[static_cast<int>(LinkListModel::Roles::Parity)].erase(             vectors_[static_cast<int>(LinkListModel::Roles::Parity)].begin() + line);
        vectors_[static_cast<int>(LinkListModel::Roles::LinkType)].erase(           vectors_[static_cast<int>(LinkListModel::Roles::LinkType)].begin() + line);
        vectors_[static_cast<int>(LinkListModel::Roles::Address)].erase(            vectors_[static_cast<int>(LinkListModel::Roles::Address)].begin() + line);
        vectors_[static_cast<int>(LinkListModel::Roles::SourcePort)].erase(         vectors_[static_cast<int>(LinkListModel::Roles::SourcePort)].begin() + line);
        vectors_[static_cast<int>(LinkListModel::Roles::DestinationPort)].erase(    vectors_[static_cast<int>(LinkListModel::Roles::DestinationPort)].begin() + line);
        vectors_[static_cast<int>(LinkListModel::Roles::IsPinned)].erase(           vectors_[static_cast<int>(LinkListModel::Roles::IsPinned)].begin() + line);
        vectors_[static_cast<int>(LinkListModel::Roles::IsHided)].erase(            vectors_[static_cast<int>(LinkListModel::Roles::IsHided)].begin() + line);
        vectors_[static_cast<int>(LinkListModel::Roles::IsNotAvailable)].erase(     vectors_[static_cast<int>(LinkListModel::Roles::IsNotAvailable)].begin() + line);
        vectors_[static_cast<int>(LinkListModel::Roles::AutoSpeedSelection)].erase( vectors_[static_cast<int>(LinkListModel::Roles::AutoSpeedSelection)].begin() + line);
        vectors_[static_cast<int>(LinkListModel::Roles::IsUpgradingState)].erase(   vectors_[static_cast<int>(LinkListModel::Roles::IsUpgradingState)].begin() + line);

        index_.remove(uuid);
        for (auto it = index_.begin(); it != index_.end(); ++it) {
            if (it.value() > line)
                --it.value();
        }

        --size_;

        endRemoveRows();
    }
}
