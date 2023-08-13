#include "pointlistmodel.h"

PointListModel::PointListModel(QObject *parent)
    : QAbstractListModel{parent}
{
    mRoleNames[XValueRole] = "x";
    mRoleNames[YValueRole] = "y";
    mRoleNames[ZValueRole] = "z";
}

int PointListModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)

    return mData.count();
}

bool PointListModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    int row = index.row();

    if (row < 0 || row >= mData.count())
        return false;

    auto point = mData.at(row);

    switch(role){
        case XValueRole: point.setX(value.toFloat());
        case YValueRole: point.setY(value.toFloat());
        case ZValueRole: point.setZ(value.toFloat());
    }

    Q_EMIT dataChanged(index, index, { Qt::EditRole, Qt::DisplayRole });

    return true;
}

QVariant PointListModel::data(const QModelIndex &index, int role) const
{
    int row = index.row();

    if (row < 0 || row >= mData.count())
        return QVariant();

    const auto point = mData.at(row);

    switch(role){
        case XValueRole: return point.x();
        case YValueRole: return point.y();
        case ZValueRole: return point.z();
    }

    return QVariant();
}

void PointListModel::insert(int index, const QVector3D &point)
{
    if (index < 0 || index > mData.count())
        return;

    Q_EMIT beginInsertRows(QModelIndex(), index, index);

    mData.insert(index, point);

    Q_EMIT endInsertRows();
}

void PointListModel::append(const QVector3D &point)
{
    insert(mData.count(), point);
}

void PointListModel::clear()
{
    Q_EMIT beginResetModel();

    mData.clear();

    Q_EMIT endResetModel();
}

void PointListModel::changePoint(int index, const QVector3D &point)
{
    if (index < 0 || index >= mData.count())
        return;

    mData.replace(index, point);

    Q_EMIT dataChanged(createIndex(0, 0), createIndex(mData.count(), 0), { Qt::EditRole, Qt::DisplayRole });
}

QHash <int, QByteArray> PointListModel::roleNames() const
{
    return mRoleNames;
}
