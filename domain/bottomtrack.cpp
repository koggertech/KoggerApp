#include "bottomtrack.h"

#include <constants.h>

BottomTrack::BottomTrack(QObject* parent)
    : SceneGraphicsObject(parent)
{
    setPrimitiveType(GL_LINE_STRIP);
}

BottomTrack::~BottomTrack()
{}

float BottomTrack::routeLength() const
{
    return 0.0f;
}

void BottomTrack::setData(const QVector<QVector3D> &data)
{
    QVector <QVector3D> filtered;

    if (m_filter){
        m_filter->apply(data,filtered);
        SceneGraphicsObject::setData(filtered);
        return;
    }

    SceneGraphicsObject::setData(data);
}

SceneObject::SceneObjectType BottomTrack::type() const
{
    return SceneObjectType::BottomTrack;
}

void BottomTrack::draw(QOpenGLFunctions* ctx,
                       const QMatrix4x4& mvp,
                       const QMap <QString, std::shared_ptr <QOpenGLShaderProgram>>& shaderProgramMap) const
{
    if(!m_isVisible)
        return;

    if(!shaderProgramMap.contains("height"))
        return;

    auto shaderProgram = shaderProgramMap["height"];

    if (!shaderProgram->bind()){
        qCritical() << "Error binding shader program.";
        return;
    }

    int posLoc    = shaderProgram->attributeLocation("position");
    int maxZLoc   = shaderProgram->uniformLocation("max_z");
    int minZLoc   = shaderProgram->uniformLocation("min_z");
    int matrixLoc = shaderProgram->uniformLocation("matrix");

    QVector4D color(0.8f, 0.2f, 0.7f, 1.0f);
    int colorLoc = shaderProgram->uniformLocation("color");

    shaderProgram->setUniformValue(colorLoc,color);
    shaderProgram->setUniformValue(maxZLoc, m_boundingBox.maximumZ());
    shaderProgram->setUniformValue(minZLoc, m_boundingBox.minimumZ());
    shaderProgram->setUniformValue(matrixLoc, mvp);
    shaderProgram->enableAttributeArray(posLoc);
    shaderProgram->setAttributeArray(posLoc, m_data.constData());

    ctx->glLineWidth(4.0);
    ctx->glDrawArrays(primitiveType(), 0, m_data.size());
    ctx->glLineWidth(1.0);

    shaderProgram->disableAttributeArray(posLoc);
    shaderProgram->release();
}
