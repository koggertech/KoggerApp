#include "pointgroup.h"

PointGroup::PointGroup(QObject *parent)
    : SceneObject(parent)
    , mModel(new QStandardItemModel)
{
}

PointGroup::~PointGroup()
{

}

SceneObject::SceneObjectType PointGroup::type() const
{
    return SceneObject::SceneObjectType::PointGroup;
}

void PointGroup::addPoint(std::shared_ptr<PointObject> point)
{
    mPointList.append(point);

    auto item = new QStandardItem();
    item->setData(point->name(), Qt::DisplayRole);

    mModel->invisibleRootItem()->appendRow(item);

    Q_EMIT countChanged(mPointList.count());
}

void PointGroup::removePoint(int index)
{
    if(index < 0 && index >= mPointList.count())
        return;

    mPointList.removeAt(index);

    mModel->invisibleRootItem()->removeRow(index);

    Q_EMIT countChanged(mPointList.count());
}

std::shared_ptr<PointObject> PointGroup::at(int index) const
{
    if(index < 0 || index >= mPointList.count())
        return nullptr;

    for(int i = 0; i < mPointList.count(); i++){
        if(i == index)
            return mPointList[i];
    }

    return nullptr;
}

QObject *PointGroup::pointAt(int index) const
{
    if(index < 0 && index >= mPointList.count())
        return nullptr;

    return static_cast <QObject*>(at(index).get());
}

QStandardItemModel *PointGroup::model() const
{
    return mModel.get();
}
