#include "coordinateaxes.h"
#include <drawutils.h>

CoordinateAxes::CoordinateAxes(QObject *parent)
    : SceneObject(new CoordinateAxesRenderImplementation,parent)
{}

CoordinateAxes::CoordinateAxesRenderImplementation::CoordinateAxesRenderImplementation()
{}

CoordinateAxes::CoordinateAxesRenderImplementation::~CoordinateAxesRenderImplementation()
{}

void CoordinateAxes::setPosition(const QVector3D &pos)
{
    m_position = pos;
    RENDER_IMPL(CoordinateAxes)->m_position = pos;

    Q_EMIT changed();
}

void CoordinateAxes::CoordinateAxesRenderImplementation::render(QOpenGLFunctions *ctx,
                                                                const QMatrix4x4 &mvp,
                                                                const QMap<QString, std::shared_ptr<QOpenGLShaderProgram> > &shaderProgramMap) const
{
    if(!shaderProgramMap.contains("static"))
        return;

    auto shaderProgram = shaderProgramMap["static"];

    if (!shaderProgram->bind()){
        qCritical() << "Error binding shader program.";
        return;
    }

    int posLoc    = shaderProgram->attributeLocation("position");
    int colorLoc  = shaderProgram->uniformLocation("color");
    int matrixLoc = shaderProgram->uniformLocation("matrix");


    shaderProgram->setUniformValue(matrixLoc, mvp);
    shaderProgram->enableAttributeArray(posLoc);

    QVector<QVector3D> axis_x{{m_position.x()-10.0f,m_position.y(),m_position.z()},{m_position.x()+10.0f,m_position.y(),m_position.z()}};
    QVector<QVector3D> axis_y{{m_position.x(),m_position.y()-10.0f,m_position.z()},{m_position.x(),m_position.y()+10.0f,m_position.z()}};
    QVector<QVector3D> axis_z{{m_position.x(),m_position.y(),m_position.z()-10.0f},{m_position.x(),m_position.y(),m_position.z()+10.0f}};

    ctx->glLineWidth(5.0f);
    shaderProgram->setAttributeArray(posLoc, axis_x.constData());
    shaderProgram->setUniformValue(colorLoc, DrawUtils::colorToVector4d(QColor(255, 0, 0)));
    ctx->glDrawArrays(GL_LINE_STRIP, 0, axis_x.size());
    shaderProgram->setAttributeArray(posLoc, axis_y.constData());
    shaderProgram->setUniformValue(colorLoc, DrawUtils::colorToVector4d(QColor(0, 255, 0)));
    ctx->glDrawArrays(GL_LINE_STRIP, 0, axis_y.size());
    shaderProgram->setAttributeArray(posLoc, axis_z.constData());
    shaderProgram->setUniformValue(colorLoc, DrawUtils::colorToVector4d(QColor(0, 0, 255)));
    ctx->glDrawArrays(GL_LINE_STRIP, 0, axis_z.size());
    ctx->glLineWidth(1.0f);

    shaderProgram->disableAttributeArray(posLoc);
    shaderProgram->release();
}
