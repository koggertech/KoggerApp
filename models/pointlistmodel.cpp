#include "pointlistmodel.h"

PointListModel::PointListModel(QObject *parent)
    : QAbstractListModel{parent}
    , mPointList(std::make_shared<PointList>())
{
    mRoleNames[XValueRole] = "x";
    mRoleNames[YValueRole] = "y";
    mRoleNames[ZValueRole] = "z";
}

int PointListModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)

    return mPointList->count();
}

bool PointListModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    int row = index.row();

    if (row < 0 || row >= mPointList->count())
        return false;

    auto point = mPointList->at(row);

    switch(role){
        case XValueRole: point->setX(value.toFloat());
        case YValueRole: point->setY(value.toFloat());
        case ZValueRole: point->setZ(value.toFloat());
    }

    Q_EMIT dataChanged(index, index, { Qt::EditRole, Qt::DisplayRole });

    return true;
}

QVariant PointListModel::data(const QModelIndex &index, int role) const
{
    int row = index.row();

    if (row < 0 || row >= mPointList->count())
        return QVariant();

    const auto point = mPointList->at(row);

    switch(role){
        case XValueRole: return point->x();
        case YValueRole: return point->y();
        case ZValueRole: return point->z();
    }

    return QVariant();
}

void PointListModel::insert(int index, std::shared_ptr <PointObject> point)
{
    if (index < 0 || index > mPointList->count())
        return;

    Q_EMIT beginInsertRows(QModelIndex(), index, index);

    mPointList->insert(index, point);

    Q_EMIT endInsertRows();
}

void PointListModel::append(std::shared_ptr <PointObject> point)
{
    insert(mPointList->count(), point);
}

void PointListModel::clear()
{
    Q_EMIT beginResetModel();

    mPointList->clear();

    Q_EMIT endResetModel();
}

QHash <int, QByteArray> PointListModel::roleNames() const
{
    return mRoleNames;
}
