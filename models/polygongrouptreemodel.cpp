#include "polygongrouptreemodel.h"

PolygonGroupTreeModel::PolygonGroupTreeModel(QObject *parent)
    : QAbstractItemModel{parent}
    , mRootItem(new PolygonObject)
{
    //auto item1 = new PolygonObject(mRootItem);
    //auto item2 = new PolygonObject(mRootItem);
    //auto item3 = new PolygonObject(mRootItem);
}

PolygonGroupTreeModel::~PolygonGroupTreeModel()
{
}

int PolygonGroupTreeModel::rowCount(const QModelIndex &parent) const
{
    const auto parentItem = getItem(parent);

    return parentItem == mRootItem ? mData.count() : parentItem->pointCount();
}

int PolygonGroupTreeModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return 1;
}

QVariant PolygonGroupTreeModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid())
        return {};

    if(role != Qt::DisplayRole)
        return {};

    auto item = getItem(index);

    return item->name();
}

QModelIndex PolygonGroupTreeModel::index(int row, int column, const QModelIndex &parent) const
{
    if (parent.isValid() && parent.column() != 0)
        return {};

    auto parentItem = getItem(parent);

    if(!parentItem)
        return {};

    //auto childItem = parentItem == mRootItem ? mData.at(row) :

    //auto childItem = parentItem->children().at(row);

    //if(childItem)
    //    return createIndex(row, column, childItem);

    return {};
}

QModelIndex PolygonGroupTreeModel::parent(const QModelIndex &index) const
{
    if(!index.isValid())
        return {};

    auto childItem = getItem(index);
    auto parentItem = childItem ? childItem->parent() : nullptr;

    //if(!parentItem || parentItem == static_cast <QObject*>(mRootItem))
    //    return {};

    return createIndex(parentItem->parent()->children().indexOf(parentItem), 0, parentItem);
}

bool PolygonGroupTreeModel::insertRows(int position, int rows, const QModelIndex &parent)
{
    //auto parentItem = getItem(parent);

    //if(!parentItem)
    //    return false;

    //// If parent equals root item, we must create a polygon item
    //// else - creating point item and insert in polygon root item

    //Q_EMIT beginInsertRows(parent, position, position + rows - 1);

    //if(parentItem == mRootItem)
    //    new PolygonObject(parentItem);
    //else{
    //    parentItem->append()
    //    new PointObject(parentItem);
    //}

    //Q_EMIT endInsertRows();

    return true;
}

bool PolygonGroupTreeModel::removeRows(int position, int rows, const QModelIndex &parent)
{
    auto parentItem = getItem(parent);

    if(!parentItem)
        return false;

    Q_EMIT beginRemoveRows(parent, position, position + rows - 1);
    auto item = parentItem->children().at(position);

    delete item;

    Q_EMIT endRemoveRows();
}

std::shared_ptr <PolygonObject> PolygonGroupTreeModel::getItem(const QModelIndex &index) const
{
    if(index.isValid()){
        auto item = mData.at(index.row());

        if(item)
            return item;
    }

    return mRootItem;
}
