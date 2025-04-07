#include "point_group.h"
#include "draw_utils.h"

PointGroup::PointGroup(QObject *parent)
    : SceneObject(new PointGroupRenderImplementation, parent)
{}

PointGroup::PointGroup(RenderImplementation *impl, QObject *parent)
    : SceneObject(impl, parent)
{}

PointGroup::~PointGroup()
{}

SceneObject::SceneObjectType PointGroup::type() const
{
    return SceneObject::SceneObjectType::PointGroup;
}

void PointGroup::removeAt(int index)
{
    if(index < 0 && index >= m_pointList.count())
        return;

    m_pointList.removeAt(index);
    RENDER_IMPL(PointGroup)->removeRenderAt(index);

    Q_EMIT changed();
    Q_EMIT boundsChanged();
}

void PointGroup::pointObjectChanged()
{
    auto point = reinterpret_cast<PointObject*>(QObject::sender());
    RENDER_IMPL(PointGroup)->m_pointRenderImplList.replace(point->m_indexInGroup,
                                                           *(dynamic_cast<PointObject::PointObjectRenderImplementation*>(point->m_renderImpl)));

    //TODO: Looks like bad
    RENDER_IMPL(PointGroup)->createBounds();
    Q_EMIT changed();
    Q_EMIT boundsChanged();
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

    QObject::connect(point.get(), &PointObject::changed, this, &PointGroup::pointObjectChanged);

    point->setParent(this);
    point->m_indexInGroup = RENDER_IMPL(PointGroup)->m_pointRenderImplList.size();

    m_pointList.append(point);

    RENDER_IMPL(PointGroup)->appendPointRenderImpl(
                dynamic_cast<PointObject::PointObjectRenderImplementation*>(point->m_renderImpl)
            );

    Q_EMIT changed();
    Q_EMIT boundsChanged();
}

void PointGroup::setData(const QVector <QVector3D>& data, int primitiveType)
{
    Q_UNUSED(data)
    Q_UNUSED(primitiveType)
}

void PointGroup::clearData()
{
    m_pointList.clear();
    RENDER_IMPL(PointGroup)->clearData();

    Q_EMIT changed();
}

//-----------------------RenderImplementation-----------------------------//

PointGroup::PointGroupRenderImplementation::PointGroupRenderImplementation()
    : SceneObject::RenderImplementation()
{}

PointGroup::PointGroupRenderImplementation::~PointGroupRenderImplementation()
{}

void PointGroup::PointGroupRenderImplementation::render(QOpenGLFunctions *ctx, const QMatrix4x4 &mvp, const QMap<QString, std::shared_ptr<QOpenGLShaderProgram> > &shaderProgramMap) const
{
    if(!m_isVisible)
        return;

    for(const auto& renderImpl : m_pointRenderImplList)
        renderImpl.render(ctx, mvp, shaderProgramMap);
}

void PointGroup::PointGroupRenderImplementation::clearData()
{
    m_pointRenderImplList.clear();
}

void PointGroup::PointGroupRenderImplementation::appendPointRenderImpl(PointObject::PointObjectRenderImplementation *impl)
{
    m_pointRenderImplList.append(*impl);

    createBounds();
}

void PointGroup::PointGroupRenderImplementation::removeRenderAt(int index)
{
    if(index < 0 && index >= m_pointRenderImplList.count())
        return;

    m_pointRenderImplList.removeAt(index);

    createBounds();
}

void PointGroup::PointGroupRenderImplementation::createBounds()
{
    Cube bounds;

    for(const auto& pointRenderImpl : m_pointRenderImplList)
        bounds.merge(pointRenderImpl.bounds());

    m_bounds = std::move(bounds);
}
