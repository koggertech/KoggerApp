#include "vertexobject.h"

VertexObject::VertexObject(QObject* parent)
    : QObject(parent)
    , mPrimitiveType(GL_TRIANGLES)
{
    mUuid = QUuid::createUuid();
}

VertexObject::VertexObject(const int type, QObject* parent)
    : QObject(parent)
    , mPrimitiveType(type)
{
    mUuid = QUuid::createUuid();
}

VertexObject::VertexObject(const int type, const QVector<QVector3D> &data, QObject* parent)
    : QObject(parent)
    , mPrimitiveType(type)
    , mData(data)
{  
    mUuid = QUuid::createUuid();
}

VertexObject::~VertexObject()
{

}

QString VertexObject::id() const
{
    return mUuid.toString();
}

QString VertexObject::name() const
{
    return mName;
}

QString VertexObject::type() const
{
    return mType;
}

void VertexObject::setPrimitiveType(int primitiveType)
{
    if (mPrimitiveType == primitiveType)
        return;

    mPrimitiveType = primitiveType;

    Q_EMIT primitiveTypeChanged(primitiveType);
}

void VertexObject::setData(const QVector<QVector3D> &data)
{
    mData = data;

    Q_EMIT dataChanged();

    createBounds();

    Q_EMIT boundsChanged();
}

void VertexObject::setName(QString name)
{
    if(mName == name)
        return;

    mName = name;

    Q_EMIT nameChanged(mName);
}

void VertexObject::setType(QString type)
{
    if (mType == type)
        return;

    mType = type;

    Q_EMIT typeChanged(mType);
}

void VertexObject::append(const QVector3D &vertex)
{
    mData.append(vertex);

    Q_EMIT dataChanged();

    createBounds();

    Q_EMIT boundsChanged();
}

void VertexObject::append(const QVector<QVector3D> &vertexVector)
{
    mData.append(vertexVector);

    Q_EMIT dataChanged();

    createBounds();

    Q_EMIT boundsChanged();
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

Cube VertexObject::bounds() const
{
    return mBounds;
}

void VertexObject::clearData()
{
    if(mData.isEmpty())
        return;

    mData.clear();

    Q_EMIT dataChanged();
}

void VertexObject::createBounds()
{
    if (mData.isEmpty()){
        mBounds = Cube(0.0f, 0.0f, 0.0f,0.0f,0.0f,0.0f);
        return;
    }

    float z_max = mData.first().z();
    float z_min = z_max;

    float x_max = mData.first().x();
    float x_min = x_max;

    float y_max = mData.first().y();
    float y_min = y_max;

    for (const auto& v: mData){
        z_min = std::min(z_min, v.z());
        z_max = std::max(z_max, v.z());

        x_min = std::min(x_min, v.x());
        x_max = std::max(x_max, v.x());

        y_min = std::min(y_min, v.y());
        y_max = std::max(y_max, v.y());
    }

    mBounds = Cube(x_min, x_max, y_min, y_max, z_min, z_max);

    Q_EMIT boundsChanged();
}
