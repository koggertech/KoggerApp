#include "pointobject.h"
#include <drawutils.h>

PointObject::PointObject(QObject *parent)
    : SceneGraphicsObject{parent}
{
    setPrimitiveType(GL_POINTS);
    m_data.append({0.0f, 0.0f, 0.0f});
}

void PointObject::draw(QOpenGLFunctions *ctx, const QMatrix4x4 &mvp, const QMap<QString, std::shared_ptr<QOpenGLShaderProgram> > &shaderProgramMap) const
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

    ctx->glEnable(GL_PROGRAM_POINT_SIZE);

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
    ctx->glDisable(GL_PROGRAM_POINT_SIZE);

    shaderProgram->disableAttributeArray(posLoc);
    shaderProgram->release();
}

SceneObject::SceneObjectType PointObject::type() const
{
    return SceneObject::SceneObjectType::Point;
}

float PointObject::x() const
{
    return m_data.first().x();
}

float PointObject::y() const
{
    return m_data.first().y();
}

float PointObject::z() const
{
    return m_data.first().z();
}

QVector3D PointObject::position() const
{
    return m_data.at(0);
}

void PointObject::setPosition(float x, float y, float z)
{
    m_data[0].setX(x);
    m_data[0].setY(y);
    m_data[0].setZ(z);
}

void PointObject::setPosition(QVector3D pos)
{
    m_data[0].setX(pos.x());
    m_data[0].setY(pos.y());
    m_data[0].setZ(pos.z());
}

void PointObject::append(const QVector3D &vertex)
{
    Q_UNUSED(vertex);
}

void PointObject::append(const QVector<QVector3D> &other)
{
    Q_UNUSED(other);
}

void PointObject::setData(const QVector<QVector3D> &data)
{
    Q_UNUSED(data);
}
