#include "LinkListModel.h"


LinkListModel::LinkListModel(QObject* parent)
    : QAbstractListModel(parent) {
    connect(this, &LinkListModel::appendEvent, this, &LinkListModel::doAppend);
}


QVariant LinkListModel::data(const QModelIndex &index, int role) const{
    const int indexRow = index.row();
    QVector<QVariant> vectorRole = _vectors[role];
    if (indexRow < 0 || vectorRole.size() <= indexRow) {
        return {"No data"};
    }
    return _vectors[role][indexRow];
}

QHash<int, QByteArray> LinkListModel::roleNames() const {
    return _roleNames;
}

void LinkListModel::doAppend(bool connectionStatus, ::ControlType controlType, const QString& portName, int baudrate, bool parity,
                             ::LinkType linkType, const QString& address, int sourcePort, int destinationPort, bool isPinned, bool isHided, bool isNotAvailable)
{
    const int line = rowCount();
    beginInsertRows(QModelIndex(), line, line);
    _vectors[LinkListModel::ConnectionStatus].append(connectionStatus);
    _vectors[LinkListModel::ControlType].append(controlType);
    _vectors[LinkListModel::PortName].append("portNamesdcsdcs");
    _vectors[LinkListModel::Baudrate].append(baudrate);
    _vectors[LinkListModel::Parity].append(parity);
    _vectors[LinkListModel::LinkType].append(linkType);
    _vectors[LinkListModel::Address].append(address);
    _vectors[LinkListModel::SourcePort].append(sourcePort);
    _vectors[LinkListModel::DestinationPort].append(destinationPort);
    _vectors[LinkListModel::isPinned].append(isPinned);
    _vectors[LinkListModel::isHided].append(isHided);
    _vectors[LinkListModel::isNotAvailable].append(isNotAvailable);
    endInsertRows();
}
