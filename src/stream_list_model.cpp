#include "stream_list_model.h"


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

void StreamListModel::doAppend(int id, uint32_t size, uint32_t doneSize, const QString& time, int recordState, int uploadState) {

    if(!_index.contains(id)) {
        const int line = rowCount();
        beginInsertRows(QModelIndex(), line, line);

        _vectors[StreamListModel::Visibility].append(true);
        _vectors[StreamListModel::ID].append(id);
        _vectors[StreamListModel::Size].append(size);
        _vectors[StreamListModel::DoneSize].append(doneSize);
        _vectors[StreamListModel::Time].append(time);
        _vectors[StreamListModel::RecordState].append(recordState);
        _vectors[StreamListModel::UploadingState].append(uploadState);

        _index[id] = line;
        _size++;
        endInsertRows();
    } else {
        int line = _index[id];
        _vectors[StreamListModel::Visibility][line] = (true);
        _vectors[StreamListModel::ID][line] = (id);
        _vectors[StreamListModel::Size][line] = (size);
        _vectors[StreamListModel::DoneSize][line] = (doneSize);
        _vectors[StreamListModel::Time][line] = (time);
        _vectors[StreamListModel::RecordState][line] = (recordState);
        _vectors[StreamListModel::UploadingState][line] = (uploadState);
        dataChanged(index(line, 0), index(line, 0));
    }
}
