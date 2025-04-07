#include "navigation_arrow.h"

#include <QtMath>
#include <draw_utils.h>


NavigationArrow::NavigationArrow(QObject *parent) :
    SceneObject(new NavigationArrowRenderImplementation, parent)
{
    auto* r = RENDER_IMPL(NavigationArrow);
    r->arrowVertices_ = makeArrowVertices();
    r->arrowRibs_ = makeArrowRibs();
}

NavigationArrow::NavigationArrowRenderImplementation::NavigationArrowRenderImplementation() :
    position_(QVector3D(0.0f, 0.0f, 0.0f))
{}

NavigationArrow::NavigationArrowRenderImplementation::~NavigationArrowRenderImplementation()
{}

void NavigationArrow::setPositionAndAngle(const QVector3D& position, float degAngle)
{
    auto* r = RENDER_IMPL(NavigationArrow);
    r->position_ = position;
    r->angle_ = degAngle;

    Q_EMIT changed();
}

void NavigationArrow::resetPositionAndAngle()
{
    auto* r = RENDER_IMPL(NavigationArrow);
    r->position_ = QVector3D(0.0f, 0.0f, 0.0f);
    r->angle_ = 0.0f;

    Q_EMIT changed();
}

QVector<QVector3D> NavigationArrow::makeArrowVertices() const
{
    QVector<QVector3D> verts;
    verts.reserve(6 * 3);

    QVector3D A( -2.0f, -1.0f,  0.0f );
    QVector3D B(  0.0f,  0.0f,  0.0f );
    QVector3D C(  2.0f, -1.0f,  0.0f );
    QVector3D D(  0.0f,  5.0f,  0.0f );
    QVector3D E(  0.0f,  1.0f,  1.0f );

    //verts << A << B << D
    //      << B << C << D
    verts << A << B << E
          << B << C << E
          << A << E << D
          << E << C << D;

    return verts;
}

QVector<QVector3D> NavigationArrow::makeArrowRibs() const
{
    QVector<QVector3D> ribs;
    ribs.reserve(6 * 3);

    QVector3D A( -2.0f, -1.0f,  0.05f );
    QVector3D B(  0.0f, -0.0f,  0.05f );
    QVector3D C(  2.0f, -1.0f,  0.05f );
    QVector3D D(  0.0f,  5.0f,  0.05f );
    QVector3D E(  0.0f,  1.0f,  1.05f );

    ribs << A << B
         << B << C
         << C << D
         << D << A
         << D << E
         << E << A
         << E << C;
//       << E << B;

    return ribs;
}

void NavigationArrow::NavigationArrowRenderImplementation::render(QOpenGLFunctions *ctx,
                                                                  const QMatrix4x4 &mvp,
                                                                  const QMap<QString, std::shared_ptr<QOpenGLShaderProgram> > &shaderProgramMap) const
{
#if defined(FAKE_COORDS)
    return;
#endif

    if (position_.isNull() || !m_isVisible || !shaderProgramMap.contains("static")) {
        return;
    }

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

    { // edges
        QVector<GLfloat> vertices;
        vertices.reserve(arrowVertices_.size() * 3);
        for (const QVector3D &v : arrowVertices_) {
            vertices << v.x() << v.y() << v.z();
        }
        shaderProgram->setAttributeArray(posLoc, vertices.constData(), 3);
        shaderProgram->setUniformValue(colorLoc, DrawUtils::colorToVector4d(QColor(235, 52, 52)));
        ctx->glDrawArrays(GL_TRIANGLES, 0, vertices.size() / 3);
    }

    { // ribs
        QVector<GLfloat> lineVertices;
        lineVertices.reserve(arrowRibs_.size() * 3);
        for (const QVector3D &v : arrowRibs_) {
            lineVertices << v.x() << v.y() << v.z();
        }
        shaderProgram->setAttributeArray(posLoc, lineVertices.constData(), 3);
        shaderProgram->setUniformValue(colorLoc, DrawUtils::colorToVector4d(QColor(99, 22, 22)));

        ctx->glLineWidth(2.0f);
        ctx->glDrawArrays(GL_LINES, 0, lineVertices.size() / 3);
    }

    shaderProgram->disableAttributeArray(posLoc);
    shaderProgram->release();
}

QVector3D NavigationArrow::NavigationArrowRenderImplementation::getPosition() const
{
    return position_;
}

float NavigationArrow::NavigationArrowRenderImplementation::getAngle() const
{
    return angle_;
}
