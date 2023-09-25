#include "polygongroup.h"
#include <polygonobject.h>

PolygonGroup::PolygonGroup(QObject *parent)
: SceneGraphicsObject(parent)
{}

PolygonGroup::~PolygonGroup()
{}

void PolygonGroup::draw(QOpenGLFunctions *ctx, const QMatrix4x4 &mvp, const QMap<QString, std::shared_ptr<QOpenGLShaderProgram> > &shaderProgramMap) const
{
    if(!m_isVisible)
        return;

    for(const auto& polygon : m_polygonList)
        polygon->draw(ctx, mvp, shaderProgramMap);
}

SceneObject::SceneObjectType PolygonGroup::type() const
{
    return SceneObject::SceneObjectType::PolygonGroup;
}

std::shared_ptr<PolygonObject> PolygonGroup::at(int index) const
{
    if(index < 0 || index >= m_polygonList.count())
        return nullptr;

    return m_polygonList.at(index);
}

PolygonObject *PolygonGroup::polygonAt(int index)
{
    return at(index).get();
}

void PolygonGroup::addPolygon(std::shared_ptr<PolygonObject> polygon)
{
    if(m_polygonList.contains(polygon))
        return;

    polygon->setParent(this);
    m_polygonList.append(polygon);
}

std::shared_ptr<PolygonObject> PolygonGroup::addPolygon()
{
    auto polygon = std::make_shared<PolygonObject>(this);
    m_polygonList.append(polygon);
    return m_polygonList.back();
}

void PolygonGroup::removePolygon(std::shared_ptr<PolygonObject> polygon)
{
    if(!m_polygonList.contains(polygon))
        return;

    m_polygonList.removeOne(polygon);
}

void PolygonGroup::removePolygonAt(int index)
{
    if(index < 0 && index >= m_polygonList.count())
        return;

    m_polygonList.removeAt(index);
}

void PolygonGroup::setData(const QVector<QVector3D> &data)
{
    Q_UNUSED(data)
}

void PolygonGroup::clearData()
{}

void PolygonGroup::append(const QVector3D &vertex)
{
    Q_UNUSED(vertex)
}

void PolygonGroup::append(const QVector<QVector3D> &other)
{
    Q_UNUSED(other)
}

void PolygonGroup::setPrimitiveType(int primitiveType)
{
    Q_UNUSED(primitiveType)
}
