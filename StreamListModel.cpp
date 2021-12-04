#include "StreamListModel.h"


StreamListModel::StreamListModel(QObject* parent)
    : QAbstractListModel(parent) {
    connect(this, &StreamListModel::appendEvent, this, &StreamListModel::doAppend);
}


QVariant StreamListModel::data(const QModelIndex &index, int role) const{
    const int indexRow = index.row();
    QVector<QVariant> vectorRole = _vectors[role];
    if (indexRow < 0 || vectorRole.size() <= indexRow) {
        return {"No data"};
    }
    return _vectors[role][indexRow];
}

QHash<int, QByteArray> StreamListModel::roleNames() const {
    return _roleNames;
}

void StreamListModel::doAppend(int id, uint32_t size, const QString& time, int recordState, int uploadState) {
    const int line = rowCount();
    beginInsertRows(QModelIndex(), line, line);

    _vectors[StreamListModel::Visibility].append(true);
    _vectors[StreamListModel::ID].append(id);
    _vectors[StreamListModel::Size].append(size);
    _vectors[StreamListModel::Time].append(time);
    _vectors[StreamListModel::RecordState].append(recordState);
    _vectors[StreamListModel::UploadingState].append(uploadState);

    _size++;
    endInsertRows();
}
