#include "pointgroup.h"
#include <drawutils.h>
#include <pointobject.h>

PointGroup::PointGroup(QObject *parent)
: SceneGraphicsObject(parent)
{
    setPrimitiveType(GL_POINTS);
}

PointGroup::~PointGroup()
{}

SceneObject::SceneObjectType PointGroup::type() const
{
    return SceneObject::SceneObjectType::PointGroup;
}

void PointGroup::draw(QOpenGLFunctions *ctx, const QMatrix4x4 &mvp, const QMap<QString, std::shared_ptr<QOpenGLShaderProgram> > &shaderProgramMap) const
{
    if(!m_isVisible)
        return;

    for(const auto& point : m_pointList)
        point->draw(ctx, mvp, shaderProgramMap);
}

void PointGroup::removeAt(int index)
{
    if(index < 0 && index >= m_pointList.count())
        return;

    m_pointList.removeAt(index);
}

std::shared_ptr<PointObject> PointGroup::at(int index) const
{
    if(index < 0 || index >= m_pointList.count())
        return nullptr;

    return m_pointList.at(index);
}

void PointGroup::append(std::shared_ptr <PointObject> point)
{
    if(m_pointList.contains(point))
        return;

    point->setParent(this);

    m_pointList.append(point);
}

void PointGroup::setData(const QVector<QVector3D> &data)
{
    Q_UNUSED(data)
}

void PointGroup::clearData()
{
    m_pointList.clear();
}

void PointGroup::append(const QVector3D &vertex)
{
    Q_UNUSED(vertex)
}

void PointGroup::append(const QVector<QVector3D> &other)
{
    Q_UNUSED(other)
}
