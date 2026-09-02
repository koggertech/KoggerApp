#include "console_list_model.h"

#include <QStringList>

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
        return {};
    }

    const auto it = _vectors.constFind(role);
    if (it == _vectors.cend() || it.value().size() <= indexRow) {
        return {};
    }

    return it.value().at(indexRow);
}

QHash<int, QByteArray> ConsoleListModel::roleNames() const {
    return _roleNames;
}

void ConsoleListModel::doAppend(const QString& time, int category, const QString& data)
{
    trimHeadIfNeeded();

    const int line = rowCount();
    beginInsertRows(QModelIndex(), line, line);

    _vectors[ConsoleListModel::Time].append(time);
    _vectors[ConsoleListModel::Category].append(category);
    _vectors[ConsoleListModel::Payload].append(data);
    ++_size;

    endInsertRows();
}

QString ConsoleListModel::rowText(int row) const
{
    if (row < 0 || row >= _size) {
        return {};
    }

    const auto timeIt = _vectors.constFind(ConsoleListModel::Time);
    const auto payloadIt = _vectors.constFind(ConsoleListModel::Payload);
    if (timeIt == _vectors.cend() || payloadIt == _vectors.cend()) {
        return {};
    }
    if (timeIt.value().size() <= row || payloadIt.value().size() <= row) {
        return {};
    }

    return timeIt.value().at(row).toString() + QStringLiteral("  ") + payloadIt.value().at(row).toString();
}

QString ConsoleListModel::rangeText(int from, int to) const
{
    if (_size == 0) {
        return {};
    }

    const int first = qBound(0, qMin(from, to), _size - 1);
    const int last = qBound(0, qMax(from, to), _size - 1);

    QStringList lines;
    lines.reserve(last - first + 1);
    for (int row = first; row <= last; ++row) {
        lines.append(rowText(row));
    }

    return lines.join(QLatin1Char('\n'));
}

void ConsoleListModel::setMaxRows(int rows)
{
    rows = qBound(kMinRows, rows, kMaxRows);
    if (rows == _maxRows) {
        return;
    }
    _maxRows = rows;

    removeHead(_size - _maxRows);
}

void ConsoleListModel::trimHeadIfNeeded(int incomingCount)
{
    const int overflow = (_size + incomingCount) - _maxRows;
    if (overflow <= 0) {
        return;
    }

    const int batch = qBound(1, _maxRows / 10, kTrimBatch);
    removeHead(qMin(_size, qMax(overflow, batch)));
}

void ConsoleListModel::removeHead(int removeCount)
{
    if (removeCount <= 0) {
        return;
    }

    beginRemoveRows(QModelIndex(), 0, removeCount - 1);

    for (auto it = _vectors.begin(); it != _vectors.end(); ++it) {
        it.value().remove(0, removeCount);
    }
    _size -= removeCount;

    endRemoveRows();

    emit rowsTrimmed(removeCount);
}
