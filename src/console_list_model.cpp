#include "console_list_model.h"

ConsoleListModel::ConsoleListModel(QObject* parent)
    : QAbstractListModel(parent)
{

}

void ConsoleListModel::init() {
    connect(this, &ConsoleListModel::appendEvent, this, &ConsoleListModel::doAppend);
}

QVariant ConsoleListModel::data(const QModelIndex &index, int role) const{
    const int indexRow = index.row();
    if (indexRow < 0 || indexRow >= _size) {
        return {"No data"};
    }

    const auto it = _vectors.constFind(role);
    if (it == _vectors.cend() || it.value().size() <= indexRow) {
        return {"No data"};
    }

    return it.value().at(indexRow);
}

QHash<int, QByteArray> ConsoleListModel::roleNames() const {
    return _roleNames;
}

void ConsoleListModel::doAppend(const QString& time, int category, const QString& data)
{
    trimHeadIfNeeded();

    bool visible = category & _categories;
    const int line = rowCount();
    beginInsertRows(QModelIndex(), line, line);

    _vectors[ConsoleListModel::Visibility].append(visible);
    _vectors[ConsoleListModel::Time].append(time);
    _vectors[ConsoleListModel::Category].append(category);
    _vectors[ConsoleListModel::Payload].append(data);

    _size++;
    endInsertRows();
}

void ConsoleListModel::trimHeadIfNeeded(int incomingCount)
{
    const int overflow = (_size + incomingCount) - kMaxRows;
    if (overflow <= 0) {
        return;
    }

    const int removeCount = qMin(_size, qMax(overflow, kTrimBatch));
    if (removeCount <= 0) {
        return;
    }

    beginRemoveRows(QModelIndex(), 0, removeCount - 1);

    _vectors[ConsoleListModel::Visibility].remove(0, removeCount);
    _vectors[ConsoleListModel::Time].remove(0, removeCount);
    _vectors[ConsoleListModel::Category].remove(0, removeCount);
    _vectors[ConsoleListModel::Payload].remove(0, removeCount);

    _size -= removeCount;

    endRemoveRows();
}
