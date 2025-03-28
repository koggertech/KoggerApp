#include "polygon_group.h"
#include "polygon_object.h"

PolygonGroup::PolygonGroup(QObject *parent)
    : SceneObject(new PolygonGroupRenderImplementation, parent)
{}

PolygonGroup::~PolygonGroup()
{}

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

    QObject::connect(polygon.get(), &PolygonObject::changed, this, &PolygonGroup::polygonObjectChanged);

    polygon->setParent(this);
    polygon->m_indexInGroup = RENDER_IMPL(PolygonGroup)->m_polygonRenderImplList.size();
    m_polygonList.append(polygon);

    RENDER_IMPL(PolygonGroup)->appendPolygonRenderImpl(
                (dynamic_cast<PolygonObject::PolygonObjectRenderImplementation*>(polygon->m_renderImpl)));

    Q_EMIT changed();
}

void PolygonGroup::removePolygon(std::shared_ptr<PolygonObject> polygon)
{
    if(!m_polygonList.contains(polygon))
        return;

    m_polygonList.removeOne(polygon);

    RENDER_IMPL(PolygonGroup)->removeAt(polygon->m_indexInGroup);

    Q_EMIT changed();
}

void PolygonGroup::removePolygonAt(int index)
{
    if(index < 0 && index >= m_polygonList.count())
        return;

    m_polygonList.removeAt(index);

    RENDER_IMPL(PolygonGroup)->removeAt(index);

    Q_EMIT changed();
}

void PolygonGroup::setData(const QVector<QVector3D>& data, int primitiveType)
{
    Q_UNUSED(data)
    Q_UNUSED(primitiveType)
}

void PolygonGroup::clearData()
{}

void PolygonGroup::polygonObjectChanged()
{
    auto polygon = reinterpret_cast<PolygonObject*>(QObject::sender());
    RENDER_IMPL(PolygonGroup)->m_polygonRenderImplList.replace(polygon->m_indexInGroup,
                                                           *(dynamic_cast<PolygonObject::PolygonObjectRenderImplementation*>(polygon->m_renderImpl)));
    Q_EMIT changed();
}

//-----------------------RenderImplementation-----------------------------//
PolygonGroup::PolygonGroupRenderImplementation::PolygonGroupRenderImplementation()
{}

PolygonGroup::PolygonGroupRenderImplementation::~PolygonGroupRenderImplementation()
{}

void PolygonGroup::PolygonGroupRenderImplementation::render(QOpenGLFunctions *ctx,
                                                            const QMatrix4x4 &mvp,
                                                            const QMap<QString, std::shared_ptr<QOpenGLShaderProgram> > &shaderProgramMap) const
{
    if(!m_isVisible)
        return;

    for(const auto& renderImpl : m_polygonRenderImplList)
        renderImpl.render(ctx, mvp, shaderProgramMap);
}

void PolygonGroup::PolygonGroupRenderImplementation::appendPolygonRenderImpl(PolygonObject::PolygonObjectRenderImplementation *impl)
{
    m_polygonRenderImplList.append(*impl);
}

void PolygonGroup::PolygonGroupRenderImplementation::removeAt(int index)
{
    if(index < 0 && index >= m_polygonRenderImplList.count())
        return;

    m_polygonRenderImplList.removeAt(index);
}
