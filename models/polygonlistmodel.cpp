#include "polygonlistmodel.h"

PolygonListModel::PolygonListModel(QObject *parent)
    : QAbstractListModel(parent)
    , mPolygonList(std::make_shared <PolygonList>())
{
    mRoleNames[NameRole] = "name";
    mRoleNames[PointGroupRole] = "pointGroup";
}

int PolygonListModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)

    return mPolygonList->count();
}

QVariant PolygonListModel::data(const QModelIndex &index, int role) const
{
    int row = index.row();

    if (row < 0 || row >= mPolygonList->count())
        return QVariant();

    auto polygon = mPolygonList->at(row);

    switch(role){
        case NameRole:       return polygon->name();
        //case PointGroupRole: return QVariant::fromValue(polygon->pointGroup().get());
    }

    return QVariant();
}

int PolygonListModel::count() const
{
    return mPolygonList->count();
}

void PolygonListModel::insert(int index, std::shared_ptr <PolygonObject> polygon)
{
    if (index < 0 || index > mPolygonList->count())
        return;

    Q_EMIT beginInsertRows(QModelIndex(), index, index);

    mPolygonList->insert(index, polygon);

    Q_EMIT endInsertRows();

    Q_EMIT countChanged(mPolygonList->count());
}

void PolygonListModel::append(std::shared_ptr <PolygonObject> polygon)
{
    insert(count(), polygon);
}

void PolygonListModel::removeAt(int index)
{
    if (index < 0 || index >= mPolygonList->count())
        return;

    Q_EMIT beginRemoveRows(QModelIndex(), index, index);

    mPolygonList->removeAt(index);

    Q_EMIT endRemoveRows();

    Q_EMIT countChanged(mPolygonList->count());
}

PolygonListPtr PolygonListModel::polygonList() const
{
    return mPolygonList;
}

QHash<int, QByteArray> PolygonListModel::roleNames() const
{
    return mRoleNames;
}
