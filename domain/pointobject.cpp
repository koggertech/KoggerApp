#include "pointobject.h"
#include <draw_utils.h>

PointObject::PointObject(QObject *parent)
    : SceneObject(new PointObjectRenderImplementation, parent)
{
    RENDER_IMPL(PointObject)->setData({{0.0f, 0.0f, 0.0f}}, GL_POINTS);
}

SceneObject::SceneObjectType PointObject::type() const
{
    return SceneObject::SceneObjectType::Point;
}

float PointObject::x() const
{
    return RENDER_IMPL(PointObject)->x();
}

float PointObject::y() const
{
    return RENDER_IMPL(PointObject)->y();
}

float PointObject::z() const
{
    return RENDER_IMPL(PointObject)->z();
}

QVector3D PointObject::position() const
{
    return RENDER_IMPL(PointObject)->position();
}

void PointObject::setPosition(float x, float y, float z)
{
    RENDER_IMPL(PointObject)->setPosition(x,y,z);

    Q_EMIT changed();
}

void PointObject::setPosition(const QVector3D& pos)
{
    RENDER_IMPL(PointObject)->setPosition(pos);

    Q_EMIT changed();
}

void PointObject::setData(const QVector <QVector3D>& data, int primitiveType)
{
    Q_UNUSED(data);
    Q_UNUSED(primitiveType);
}

//-----------------------RenderImplementation-----------------------------//

PointObject::PointObjectRenderImplementation::PointObjectRenderImplementation()
    : SceneObject::RenderImplementation()
{}

PointObject::PointObjectRenderImplementation::~PointObjectRenderImplementation()
{}

void PointObject::PointObjectRenderImplementation::render(QOpenGLFunctions *ctx,
                                                          const QMatrix4x4 &mvp,
                                                          const QMap<QString
                                                          , std::shared_ptr<QOpenGLShaderProgram> > &shaderProgramMap) const
{
    if(!m_isVisible)
        return;

    if(!shaderProgramMap.contains("static"))
        return;

    auto shaderProgram = shaderProgramMap["static"];

    if (!shaderProgram->bind()){
        qCritical() << "Error binding shader program.";
        return;
    }

    ctx->glEnable(34370);

    int posLoc    = shaderProgram->attributeLocation("position");
    int matrixLoc = shaderProgram->uniformLocation("matrix");
    int colorLoc  = shaderProgram->uniformLocation("color");
    int widthLoc  = shaderProgram->uniformLocation("width");

    shaderProgram->setUniformValue(colorLoc, DrawUtils::colorToVector4d(m_color));
    shaderProgram->setUniformValue(matrixLoc, mvp);
    shaderProgram->setUniformValue(widthLoc, static_cast <float>(m_width));
    shaderProgram->enableAttributeArray(posLoc);
    shaderProgram->setAttributeArray(posLoc, m_data.constData());

    ctx->glDrawArrays(m_primitiveType, 0, m_data.size());

    ctx->glDisable(34370);

    shaderProgram->disableAttributeArray(posLoc);
    shaderProgram->release();
}

void PointObject::PointObjectRenderImplementation::setPosition(float x, float y, float z)
{
    m_data[0].setX(x);
    m_data[0].setY(y);
    m_data[0].setZ(z);
}

void PointObject::PointObjectRenderImplementation::setPosition(const QVector3D& pos)
{
    m_data[0].setX(pos.x());
    m_data[0].setY(pos.y());
    m_data[0].setZ(pos.z());
}

QVector3D PointObject::PointObjectRenderImplementation::position() const
{
    return m_data.at(0);
}

float PointObject::PointObjectRenderImplementation::x() const
{
    return m_data.first().x();
}

float PointObject::PointObjectRenderImplementation::y() const
{
    return m_data.first().y();
}

float PointObject::PointObjectRenderImplementation::z() const
{
    return m_data.first().z();
}
