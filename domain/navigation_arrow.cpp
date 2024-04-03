#include "navigation_arrow.h"

#include <QtMath>
#include <drawutils.h>


NavigationArrow::NavigationArrow(QObject *parent)
    : SceneObject(new NavigationArrowRenderImplementation,parent)
{}

NavigationArrow::NavigationArrowRenderImplementation::NavigationArrowRenderImplementation()
{}

NavigationArrow::NavigationArrowRenderImplementation::~NavigationArrowRenderImplementation()
{}

void NavigationArrow::setPositionAndAngle(const QVector3D& position, float degAngle)
{
    QVector<QVector3D> renderNavArrow = cubeVertices_;
    moveToPosition(renderNavArrow, position);
    rotateByDegrees(renderNavArrow, degAngle);
    RENDER_IMPL(NavigationArrow)->cubeVertices_ = renderNavArrow;
    Q_EMIT changed();
}

void NavigationArrow::moveToPosition(QVector<QVector3D>& cubeVertices, const QVector3D& position) const
{
    for (QVector3D &vertex : cubeVertices)
        vertex += position;
}

void NavigationArrow::rotateByDegrees(QVector<QVector3D>& cubeVertices, float degAngle) const
{
    QVector3D center = QVector3D(0, 0, 0);
    for (const QVector3D &vertex : cubeVertices)
        center += vertex;
    center /= cubeVertices.size();
    for (QVector3D &vertex : cubeVertices)
        vertex -= center;
    const float angleInRadians = qDegreesToRadians(degAngle);
    const float cosAngle = qCos(angleInRadians);
    const float sinAngle = qSin(angleInRadians);
    for (QVector3D &vertex : cubeVertices) {
        const float x = vertex.x();
        const float y = vertex.y();
        vertex.setX(x * cosAngle - y * sinAngle);
        vertex.setY(x * sinAngle + y * cosAngle);
    }
    for (QVector3D &vertex : cubeVertices)
        vertex += center;
}

void NavigationArrow::NavigationArrowRenderImplementation::render(QOpenGLFunctions *ctx,
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

    ctx->glLineWidth(1.0f);
    shaderProgram->setAttributeArray(posLoc, cubeVertices_.constData());
    shaderProgram->setUniformValue(colorLoc, DrawUtils::colorToVector4d(QColor(255, 0, 0)));
    ctx->glDrawArrays(GL_QUADS, 0, cubeVertices_.size());
    ctx->glLineWidth(1.0f);

    shaderProgram->disableAttributeArray(posLoc);
    shaderProgram->release();
}
