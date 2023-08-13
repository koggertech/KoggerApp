#include "sceneobjectslistmodel.h"

SceneObjectsListModel::SceneObjectsListModel(QObject *parent)
    : QAbstractListModel{parent}
{
    mRoleNames[NameRole] = "name";
    mRoleNames[IdRole] = "id";
    mRoleNames[TypeRole] = "type";
}

int SceneObjectsListModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)

    return mData.count();
}

QVariant SceneObjectsListModel::data(const QModelIndex &index, int role) const
{
    int row = index.row();

    if (row < 0 || row >= mData.count())
        return QVariant();

    const auto object = mData.at(row);

    switch(role){
        case NameRole: return object->name();
        case TypeRole: return object->type();
        case IdRole: return object->id();
    }

    return QVariant();
}

QList<std::shared_ptr<VertexObject> > SceneObjectsListModel::data() const
{
    QList <std::shared_ptr <VertexObject>> objects;

    auto it = mData.begin();

    while (it != mData.end()){
        objects.append(*it);

        it++;
    }

    return objects;
}

void SceneObjectsListModel::insert(int index, const std::shared_ptr<VertexObject> object)
{
    if (index < 0 || index > mData.count())
        return;

    Q_EMIT beginInsertRows(QModelIndex(), index, index);

    mData.insert(index, object);

    Q_EMIT endInsertRows();

    Q_EMIT countChanged(mData.count());
}

void SceneObjectsListModel::append(const std::shared_ptr<VertexObject> object)
{
    insert(count(), object);
}

void SceneObjectsListModel::remove(int index)
{
    if (index < 0 || index >= mData.count())
        return;

    Q_EMIT beginRemoveRows(QModelIndex(), index, index);

    mData.removeAt(index);

    Q_EMIT endRemoveRows();

    Q_EMIT countChanged(mData.count());
}

void SceneObjectsListModel::clear()
{
    emit beginResetModel();

    mData.clear();

    emit endResetModel();
}

QStringList SceneObjectsListModel::names(QString filter) const
{
    QStringList nameList;

    for (const auto& object : mData){
        if (filter == object->type()){
            nameList.append(object->name());
        }
    }

    return nameList;
}

int SceneObjectsListModel::objectIndex(QString id) const
{
    for(int i = 0; i < mData.size(); i++){
        if (mData.at(i)->id() == id)
            return i;
    }

    return -1;
}

std::shared_ptr<VertexObject> SceneObjectsListModel::get(QString id) const
{
    for(const auto& object : mData){
        if(object->id() == id)
            return object;
    }

    return nullptr;
}

std::shared_ptr<VertexObject> SceneObjectsListModel::get(int index) const
{
    if (index < 0 || index >= mData.count())
        return nullptr;

    return mData.at(index);
}

void SceneObjectsListModel::replace(int index, std::shared_ptr<VertexObject> object)
{
    if (index < 0 || index >= mData.count())
        return;

    Q_EMIT beginRemoveRows(QModelIndex(), index, index);

    mData.removeAt(index);

    Q_EMIT endRemoveRows();

    Q_EMIT countChanged(mData.count());

    Q_EMIT beginInsertRows(QModelIndex(), index, index);

    mData.insert(index,object);

    Q_EMIT endInsertRows();

}

QList <std::shared_ptr<VertexObject> > SceneObjectsListModel::dataByType(QString type) const
{
    QList <std::shared_ptr <VertexObject>> data;

    auto it = mData.begin();

    while (it != mData.end()){
        if((*it)->type() == type)
            data.append(*it);

        it++;
    }

    return data;
}

int SceneObjectsListModel::count() const
{
    return rowCount(QModelIndex());
}

QHash<int, QByteArray> SceneObjectsListModel::roleNames() const
{
    return mRoleNames;
}
