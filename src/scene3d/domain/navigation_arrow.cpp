#include "navigation_arrow.h"

#include <QtMath>
#include <cmath>
#include <draw_utils.h>

NavigationArrow::NavigationArrow(QObject *parent) :
    SceneObject(new NavigationArrowRenderImplementation, parent)
{
    auto* r = RENDER_IMPL(NavigationArrow);
    r->arrowVertices_ = makeArrowVertices();
    r->arrowNormals_ = makeArrowNormals(r->arrowVertices_);
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

    if (std::isfinite(position.x()) &&
        std::isfinite(position.y())) {
        r->position_ = position;
    }

    if (std::isfinite(degAngle)) {
        r->angle_ = degAngle;
    }

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

QVector<QVector3D> NavigationArrow::makeArrowNormals(const QVector<QVector3D>& tris) const
{
    QVector<QVector3D> normals;
    normals.reserve(tris.size());

    for (int i = 0; i + 2 < tris.size(); i += 3) {
        const QVector3D& a = tris[i];
        const QVector3D& b = tris[i + 1];
        const QVector3D& c = tris[i + 2];
        QVector3D n = QVector3D::crossProduct(b - a, c - a);
        const float len = n.length();
        if (len < 1e-6f) {
            n = QVector3D(0.0f, 0.0f, 1.0f);
        } else {
            n /= len;
        }
        normals << n << n << n;
    }

    return normals;
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
    if (!m_isVisible) {
        return;
    }

    auto litShaderProgram = shaderProgramMap.value("directional_lit", nullptr);
    auto lineShaderProgram = shaderProgramMap.value("static", nullptr);
    if (!shadowEnabled_) {
        litShaderProgram.reset();
    }

    if ((qFuzzyIsNull(angle_) && position_.isNull()) || (!litShaderProgram && !lineShaderProgram)) {
        return;
    }

    EffectiveShadowParams shadow;
    if (litShaderProgram) {
        shadow = effectiveShadowParams();
    }

    if (litShaderProgram && litShaderProgram->bind()) {
        const int posLoc = litShaderProgram->attributeLocation("position");
        const int normalLoc = litShaderProgram->attributeLocation("normal");
        const int matrixLoc = litShaderProgram->uniformLocation("matrix");
        const int colorLoc = litShaderProgram->uniformLocation("color");
        const int lightDirLoc = litShaderProgram->uniformLocation("lightDir");
        const int ambientLoc = litShaderProgram->uniformLocation("ambient");
        const int intensityLoc = litShaderProgram->uniformLocation("intensity");
        const int highlightLoc = litShaderProgram->uniformLocation("highlightIntensity");

        litShaderProgram->setUniformValue(matrixLoc, mvp);
        litShaderProgram->setUniformValue(colorLoc, DrawUtils::colorToVector4d(QColor(235, 52, 52)));
        if (lightDirLoc >= 0) {
            litShaderProgram->setUniformValue(lightDirLoc, shadow.lightDir);
        }
        if (ambientLoc >= 0) {
            litShaderProgram->setUniformValue(ambientLoc, shadow.ambient);
        }
        if (intensityLoc >= 0) {
            litShaderProgram->setUniformValue(intensityLoc, shadow.intensity);
        }
        if (highlightLoc >= 0) {
            litShaderProgram->setUniformValue(highlightLoc, shadow.highlightIntensity);
        }

        if (posLoc >= 0) {
            litShaderProgram->enableAttributeArray(posLoc);
            litShaderProgram->setAttributeArray(posLoc, arrowVertices_.constData());
            if (normalLoc >= 0) {
                litShaderProgram->enableAttributeArray(normalLoc);
                litShaderProgram->setAttributeArray(normalLoc, arrowNormals_.constData());
            }
            ctx->glDrawArrays(GL_TRIANGLES, 0, arrowVertices_.size());
            if (normalLoc >= 0) {
                litShaderProgram->disableAttributeArray(normalLoc);
            }
            litShaderProgram->disableAttributeArray(posLoc);
        }
        litShaderProgram->release();
    } else if (lineShaderProgram && lineShaderProgram->bind()) {
        const int posLoc = lineShaderProgram->attributeLocation("position");
        const int colorLoc = lineShaderProgram->uniformLocation("color");
        const int matrixLoc = lineShaderProgram->uniformLocation("matrix");

        lineShaderProgram->setUniformValue(matrixLoc, mvp);
        lineShaderProgram->setUniformValue(colorLoc, DrawUtils::colorToVector4d(QColor(235, 52, 52)));
        lineShaderProgram->enableAttributeArray(posLoc);
        lineShaderProgram->setAttributeArray(posLoc, arrowVertices_.constData());
        ctx->glDrawArrays(GL_TRIANGLES, 0, arrowVertices_.size());
        lineShaderProgram->disableAttributeArray(posLoc);
        lineShaderProgram->release();
    }

    if (lineShaderProgram && lineShaderProgram->bind()) {
        const int posLoc = lineShaderProgram->attributeLocation("position");
        const int colorLoc = lineShaderProgram->uniformLocation("color");
        const int matrixLoc = lineShaderProgram->uniformLocation("matrix");

        lineShaderProgram->setUniformValue(matrixLoc, mvp);
        lineShaderProgram->setUniformValue(colorLoc, DrawUtils::colorToVector4d(QColor(99, 22, 22)));
        lineShaderProgram->enableAttributeArray(posLoc);
        lineShaderProgram->setAttributeArray(posLoc, arrowRibs_.constData());

        ctx->glLineWidth(2.0f);
        ctx->glDrawArrays(GL_LINES, 0, arrowRibs_.size());
        ctx->glLineWidth(1.0f);

        lineShaderProgram->disableAttributeArray(posLoc);
        lineShaderProgram->release();
    }
}

QVector3D NavigationArrow::NavigationArrowRenderImplementation::getPosition() const
{
    return position_;
}

float NavigationArrow::NavigationArrowRenderImplementation::getAngle() const
{
    return angle_;
}
