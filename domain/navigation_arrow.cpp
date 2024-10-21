#include "navigation_arrow.h"

#include <QtMath>
#include <draw_utils.h>


NavigationArrow::NavigationArrow(QObject *parent)
    : SceneObject(new NavigationArrowRenderImplementation,parent), isEnabled_(true)
{}

NavigationArrow::NavigationArrowRenderImplementation::NavigationArrowRenderImplementation() : isEnabled_(true)
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
void NavigationArrow::setEnabled(bool state)
{
    isEnabled_ = state;
    RENDER_IMPL(NavigationArrow)->isEnabled_ = state;
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
    if (!isEnabled_ || !shaderProgramMap.contains("static") || cubeVertices_.empty())
        return;

    auto shaderProgram = shaderProgramMap["static"];

    if (!shaderProgram->bind()) {
        qCritical() << "Error binding shader program.";
        return;
    }

    int posLoc    = shaderProgram->attributeLocation("position");
    int colorLoc  = shaderProgram->uniformLocation("color");
    int matrixLoc = shaderProgram->uniformLocation("matrix");

    shaderProgram->setUniformValue(matrixLoc, mvp);
    shaderProgram->enableAttributeArray(posLoc);

    ctx->glLineWidth(1.0f);

    QVector<GLfloat> vertices;
    vertices.reserve(cubeVertices_.size() * 3);
    vertices.append(static_cast<GLfloat>(cubeVertices_[0].x()));
    vertices.append(static_cast<GLfloat>(cubeVertices_[0].y()));
    vertices.append(static_cast<GLfloat>(cubeVertices_[0].z()));
    vertices.append(static_cast<GLfloat>(cubeVertices_[3].x()));
    vertices.append(static_cast<GLfloat>(cubeVertices_[3].y()));
    vertices.append(static_cast<GLfloat>(cubeVertices_[3].z()));
    vertices.append(static_cast<GLfloat>(cubeVertices_[1].x()));
    vertices.append(static_cast<GLfloat>(cubeVertices_[1].y()));
    vertices.append(static_cast<GLfloat>(cubeVertices_[1].z()));
    vertices.append(static_cast<GLfloat>(cubeVertices_[2].x()));
    vertices.append(static_cast<GLfloat>(cubeVertices_[2].y()));
    vertices.append(static_cast<GLfloat>(cubeVertices_[2].z()));
    vertices.append(static_cast<GLfloat>(cubeVertices_[3].x()));
    vertices.append(static_cast<GLfloat>(cubeVertices_[3].y()));
    vertices.append(static_cast<GLfloat>(cubeVertices_[3].z()));
    vertices.append(static_cast<GLfloat>(cubeVertices_[1].x()));
    vertices.append(static_cast<GLfloat>(cubeVertices_[1].y()));
    vertices.append(static_cast<GLfloat>(cubeVertices_[1].z()));

    shaderProgram->setAttributeArray(posLoc, vertices.constData(), 3);
    shaderProgram->setUniformValue(colorLoc, DrawUtils::colorToVector4d(QColor(255, 0, 0)));
    ctx->glDrawArrays(GL_TRIANGLES, 0, vertices.size() / 3);

    shaderProgram->disableAttributeArray(posLoc);
    shaderProgram->release();
}
