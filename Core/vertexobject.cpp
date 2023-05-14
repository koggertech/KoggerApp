#include "vertexobject.h"

VertexObject::VertexObject(const int type)
: mPrimitiveType(type)
{

}

VertexObject::VertexObject(const int type, const QVector<QVector3D> &data)
: mPrimitiveType(type)
, mData(data)
{  
}

VertexObject::~VertexObject()
{

}

void VertexObject::setPrimitiveType(const int type)
{
    mPrimitiveType = type;
}

void VertexObject::setData(const QVector<QVector3D> &data)
{
    mData = data;

    if (data.isEmpty())
        return;

    mMaximumZ = data.first().z();
    mMinimumZ = mMaximumZ;

    for (const auto& v: data){
        mMinimumZ = std::min(mMinimumZ, v.z());
        mMaximumZ = std::max(mMaximumZ, v.z());
    }


}

void VertexObject::append(const QVector3D &vertex)
{
    mData.append(vertex);
}

void VertexObject::append(const QVector<QVector3D> &vertexVector)
{
    mData.append(vertexVector);

    //updateGrid();
}

int VertexObject::primitiveType() const
{
    return mPrimitiveType;
}

QVector<QVector3D> VertexObject::data() const
{
    return mData;
}

const QVector<QVector3D> &VertexObject::cdata() const
{
    return mData;
}

float VertexObject::maximumZ() const
{
    return mMaximumZ;
}

float VertexObject::minimumZ() const
{
    return mMinimumZ;
}

