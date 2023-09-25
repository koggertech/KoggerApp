#include "sceneobject.h"

SceneObject::SceneObject(QObject *parent)
: QObject(parent)
{}

SceneObject::SceneObject(QString name,
                         int primitiveType,
                         QObject *parent)
: QObject(parent)
, m_name(name)
, m_primitiveType(primitiveType)
{}

SceneObject::~SceneObject()
{}

void SceneObject::setName(QString name)
{
    if(m_name != name)
        m_name = name;
}

void SceneObject::setScene(GraphicsScene3d* scene)
{
    if(mp_scene == scene)
        return;

    mp_scene = scene;
}

GraphicsScene3d* SceneObject::scene() const
{
    return mp_scene;
}

QString SceneObject::id() const
{
    return m_uuid.toString(QUuid::WithoutBraces);
}

QString SceneObject::name() const
{
    return m_name;
}

QVector<QVector3D> SceneObject::data() const
{
    return m_data;
}

const QVector<QVector3D> &SceneObject::cdata() const
{
    return m_data;
}

AbstractEntityDataFilter *SceneObject::filter() const
{
    return m_filter.get();
}

void SceneObject::setData(const QVector<QVector3D> &data)
{
    if(m_filter){
        QVector <QVector3D> filteredData;
        m_filter->apply(data, filteredData);
        m_data = filteredData;
        return;
    }

    m_data = data;

    createBoundingBox();
}

void SceneObject::setFilter(std::shared_ptr<AbstractEntityDataFilter> filter)
{
    if(m_filter == filter)
        return;

    m_filter = filter;
}

void SceneObject::clearData()
{
    m_data.clear();
}

void SceneObject::setPrimitiveType(int primitiveType)
{
    if(m_primitiveType != primitiveType)
        m_primitiveType = primitiveType;
}

void SceneObject::append(const QVector3D &vertex)
{
    m_data.append(vertex);

    Q_EMIT dataChanged();

    createBoundingBox();

    Q_EMIT boundsChanged();
}

void SceneObject::append(const QVector<QVector3D> &other)
{
    m_data.append(other);

    Q_EMIT dataChanged();

    createBoundingBox();

    Q_EMIT boundsChanged();
}

int SceneObject::primitiveType() const
{
    return m_primitiveType;
}

Cube SceneObject::boundingBox() const
{
    return m_boundingBox;
}

void SceneObject::qmlDeclare()
{
    qmlRegisterUncreatableType <SceneObject> ("SceneObject", 1, 0, "SceneObject", "");
    qRegisterMetaType<SceneObject::SceneObjectType>("SceneObjectType");
    qRegisterMetaType<AbstractEntityDataFilter::FilterType>("FilterType");
}

void SceneObject::createBoundingBox()
{
    if (m_data.isEmpty()){
        m_boundingBox = Cube(0.0f, 0.0f, 0.0f,0.0f,0.0f,0.0f);
        return;
    }

    float z_max = m_data.first().z();
    float z_min = z_max;

    float x_max = m_data.first().x();
    float x_min = x_max;

    float y_max = m_data.first().y();
    float y_min = y_max;

    for (const auto& v: m_data){
        z_min = std::min(z_min, v.z());
        z_max = std::max(z_max, v.z());

        x_min = std::min(x_min, v.x());
        x_max = std::max(x_max, v.x());

        y_min = std::min(y_min, v.y());
        y_max = std::max(y_max, v.y());
    }

    m_boundingBox = Cube(x_min, x_max, y_min, y_max, z_min, z_max);
}

SceneObject::SceneObjectType SceneObject::type() const
{
    return SceneObjectType::Unknown;
}
