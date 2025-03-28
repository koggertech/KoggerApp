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
    QVector<QVariant> vectorRole = _vectors[role];
    if (indexRow < 0 || vectorRole.size() <= indexRow) {
        return {"No data"};
    }
    return _vectors[role][indexRow];
}

QHash<int, QByteArray> ConsoleListModel::roleNames() const {
    return _roleNames;
}

void ConsoleListModel::doAppend(const QString& time, int category, const QString& data)
{
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
