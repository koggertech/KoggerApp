#include "pointset.h"

PointSet::PointSet(QObject *parent)
    : DisplayedObject(GL_POINTS, parent)
    , mPointListModel(std::make_shared <PointListModel>())
{

}

void PointSet::setData(const QVector<QVector3D> &data)
{
    VertexObject::setData(data);

    mPointListModel->clear();

    for(const auto& vertice : mData)
        mPointListModel->append(vertice);
}

void PointSet::append(const QVector3D &vertex)
{
    VertexObject::append(vertex);

    mPointListModel->clear();

    for(const auto& vertice : mData)
        mPointListModel->append(vertice);
}

void PointSet::append(const QVector<QVector3D> &other)
{
    VertexObject::append(other);

    mPointListModel->clear();

    for(const auto& vertice : mData)
        mPointListModel->append(vertice);
}

SceneObject::SceneObjectType PointSet::type() const
{
    return SceneObjectType::PointSet;
}

void PointSet::changePointCoord(int index, QVector3D coord)
{
    if(index < 0 || index >= mData.size())
        return;

    mData.replace(index, coord);

    mPointListModel->changePoint(index, coord);

    //Q_EMIT dataChanged();

    Q_EMIT pointChanged(index);
}

QVariantList PointSet::points() const
{
    QVariantList list;

    for(const auto& point : mData){
        list.append(QVariant(point));
    }

    return list;
}

PointListModel *PointSet::pointListModel() const
{
    return mPointListModel.get();
}

void PointSet::removePoint(int index)
{
    if(index < 0 || index >= mData.size())
        return;

    mData.remove(index);

    Q_EMIT dataChanged();
}
