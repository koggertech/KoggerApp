#include "polygonobject.h"

#include <QModelIndex>

PolygonObject::PolygonObject(QObject *parent)
    : DisplayedObject(GL_POLYGON, parent)
    , mModel(new QStandardItemModel)
{
    mModel->setHeaderData(0, Qt::Horizontal, QObject::tr("Subject"));
}

PolygonObject::~PolygonObject()
{}

SceneObject::SceneObjectType PolygonObject::type() const
{
    return SceneObject::SceneObjectType::Polygon;
}

void PolygonObject::setData(const QVector<QVector3D> &data)
{
    VertexObject::setData(data);

    mModel->clear();

    for(const auto& point : mData){
        auto item = new QStandardItem();

        item->setData(QStringLiteral("x: %1, y: %2, z: %3").arg(point.x())
                                                           .arg(point.y())
                                                           .arg(point.z()),
                       Qt::DisplayRole);

        mModel->invisibleRootItem()->appendRow(item);
    }
}

void PolygonObject::append(const QVector<QVector3D> &data)
{
    VertexObject::append(data);

    for(const auto& point : data){
        auto item = new QStandardItem();

        item->setData(QStringLiteral("x: %1, y: %2, z: %3").arg(point.x())
                                                           .arg(point.y())
                                                           .arg(point.z()),
                       Qt::DisplayRole);

        mModel->invisibleRootItem()->appendRow(item);
    }
}

void PolygonObject::append(const QVector3D &point)
{
    VertexObject::append(point);

    auto item = new QStandardItem();

    item->setData(QStringLiteral("x: %1, y: %2, z: %3").arg(point.x())
                                                       .arg(point.y())
                                                       .arg(point.z()),
                   Qt::DisplayRole);

    mModel->invisibleRootItem()->appendRow(item);
}

void PolygonObject::remove(int index)
{
    VertexObject::remove(index);

    mModel->invisibleRootItem()->removeRow(index);
}

int PolygonObject::pointCount() const
{
    return mData.count();
}

const QVector3D& PolygonObject::pointAt(int index) const
{
    return mData.at(index);
}

void PolygonObject::replacePoint(int index, const QVector3D &point)
{
    if(index < 0 || index >= mData.count())
        return;

    mData.replace(index, point);

    VertexObject::createBounds();

    auto item = mModel->invisibleRootItem()->child(index);

    item->setData(QStringLiteral("x: %1, y: %2, z: %3").arg(point.x())
                                                       .arg(point.y())
                                                       .arg(point.z()),
                   Qt::DisplayRole);
}

QStandardItemModel *PolygonObject::model() const
{
    return mModel.get();
}
