#include "navigation_arrow.h"

#include <QtMath>
#include <QtGlobal>
#include <cmath>
#include <draw_utils.h>

NavigationArrow::NavigationArrow(QObject *parent) :
    SceneObject(new NavigationArrowRenderImplementation, parent)
{
    auto* r = RENDER_IMPL(NavigationArrow);
    r->arrowVertices_ = makeArrowVertices();
    r->arrowNormals_ = makeArrowNormals(r->arrowVertices_);
    r->arrowRibs_ = makeArrowRibs();
    r->boatVertices_ = makeBoatVertices();
    r->boatNormals_ = makeBoatNormals(r->boatVertices_);
    r->boatRibs_ = makeBoatRibs();
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

void NavigationArrow::setSize(int size)
{
    auto* r = RENDER_IMPL(NavigationArrow);
    const int boundedSize = qBound(1, size, 5);

    if (r->size_ == boundedSize) {
        return;
    }

    r->size_ = boundedSize;
    Q_EMIT changed();
}

void NavigationArrow::setShape(int shape)
{
    auto* r = RENDER_IMPL(NavigationArrow);
    const Shape newShape = (shape == static_cast<int>(Shape::Boat)) ? Shape::Boat : Shape::Arrow;

    if (r->shape_ == newShape) {
        return;
    }

    r->shape_ = newShape;
    Q_EMIT changed();
}

QVector<QVector3D> NavigationArrow::makeArrowVertices() const
{
    QVector<QVector3D> verts;
    verts.reserve(static_cast<size_t>(6) * static_cast<size_t>(3));

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
    ribs.reserve(static_cast<size_t>(6) * static_cast<size_t>(3));

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

QVector<QVector<QVector3D>> NavigationArrow::buildBoatStations() const
{
    struct St { float y; float halfBeam; float keelZ; float topZ; };
    const St table[] = {
        { 0.0f, 0.70f, 0.26f, 0.86f },
        { 0.7f, 0.88f, 0.14f, 0.80f },
        { 1.8f, 1.10f, 0.04f, 0.72f },
        { 3.0f, 1.20f, 0.00f, 0.70f },
        { 4.3f, 1.05f, 0.06f, 0.76f },
        { 5.5f, 0.60f, 0.24f, 0.96f },
        { 6.6f, 0.00f, 0.56f, 1.20f },
    };
    const int count = static_cast<int>(sizeof(table) / sizeof(table[0]));

    QVector<QVector<QVector3D>> stations;
    stations.reserve(count);

    for (int i = 0; i < count; ++i) {
        const St& s = table[i];
        const float chineX = s.halfBeam * 0.75f;
        const float chineZ = s.keelZ + (s.topZ - s.keelZ) * 0.30f;

        QVector<QVector3D> sec;
        sec.reserve(5);
        sec << QVector3D(-s.halfBeam, s.y, s.topZ)
            << QVector3D(-chineX,     s.y, chineZ)
            << QVector3D( 0.0f,       s.y, s.keelZ)
            << QVector3D( chineX,     s.y, chineZ)
            << QVector3D( s.halfBeam, s.y, s.topZ);
        stations << sec;
    }

    return stations;
}

QVector<QVector3D> NavigationArrow::makeBoatVertices() const
{
    const QVector<QVector<QVector3D>> st = buildBoatStations();
    QVector<QVector3D> verts;
    if (st.size() < 2) {
        return verts;
    }

    const int sections = st.first().size();
    verts.reserve((st.size() - 1) * (sections - 1) * 6 + 12);

    for (int i = 0; i + 1 < st.size(); ++i) {
        const QVector<QVector3D>& a = st[i];
        const QVector<QVector3D>& b = st[i + 1];
        for (int j = 0; j + 1 < sections; ++j) {
            verts << a[j] << a[j + 1] << b[j + 1]
                  << a[j] << b[j + 1] << b[j];
        }
    }

    {
        const QVector<QVector3D>& s = st.first();
        verts << s[0] << s[1] << s[2]
              << s[0] << s[2] << s[3]
              << s[0] << s[3] << s[4];
    }

    auto addThwart = [&verts](float y, float halfW, float z) {
        const QVector3D p0(-halfW, y - 0.12f, z);
        const QVector3D p1( halfW, y - 0.12f, z);
        const QVector3D p2( halfW, y + 0.12f, z);
        const QVector3D p3(-halfW, y + 0.12f, z);
        verts << p0 << p1 << p2
              << p0 << p2 << p3;
    };
    addThwart(2.1f, 0.95f, 0.60f);
    addThwart(4.1f, 0.95f, 0.62f);

    return verts;
}

QVector<QVector3D> NavigationArrow::makeBoatNormals(const QVector<QVector3D>& tris) const
{
    QVector<QVector3D> normals;
    normals.reserve(tris.size());

    QVector3D centroid(0.0f, 0.0f, 0.0f);
    for (const QVector3D& v : tris) {
        centroid += v;
    }
    if (!tris.isEmpty()) {
        centroid /= static_cast<float>(tris.size());
    }

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
        const QVector3D faceCenter = (a + b + c) / 3.0f;
        if (QVector3D::dotProduct(n, faceCenter - centroid) < 0.0f) {
            n = -n;
        }
        normals << n << n << n;
    }

    return normals;
}

QVector<QVector3D> NavigationArrow::makeBoatRibs() const
{
    QVector<QVector<QVector3D>> st = buildBoatStations();
    QVector<QVector3D> ribs;
    if (st.size() < 2) {
        return ribs;
    }

    const QVector3D lift(0.0f, 0.0f, 0.02f);
    for (QVector<QVector3D>& sec : st) {
        for (QVector3D& p : sec) {
            p += lift;
        }
    }

    const int sections = st.first().size();
    const int gL = 0;
    const int kIdx = 2;
    const int gR = sections - 1;

    for (int i = 0; i + 1 < st.size(); ++i) {
        ribs << st[i][gL]   << st[i + 1][gL];
        ribs << st[i][gR]   << st[i + 1][gR];
        ribs << st[i][kIdx] << st[i + 1][kIdx];
    }

    const int hoops[] = { 0, 2, 4 };
    for (int h = 0; h < 3; ++h) {
        const QVector<QVector3D>& sec = st[hoops[h]];
        for (int j = 0; j + 1 < sections; ++j) {
            ribs << sec[j] << sec[j + 1];
        }
    }

    ribs << st.first()[gL] << st.first()[gR];

    return ribs;
}

void NavigationArrow::NavigationArrowRenderImplementation::render(QOpenGLFunctions *ctx,
                                                                  const QMatrix4x4 &mvp,
                                                                  const QMap<QString, std::shared_ptr<QOpenGLShaderProgram> > &shaderProgramMap) const
{
    if (!m_isVisible) {
        return;
    }

    const bool isBoat = (shape_ == Shape::Boat);
    const QVector<QVector3D>& vertices = isBoat ? boatVertices_ : arrowVertices_;
    const QVector<QVector3D>& normals  = isBoat ? boatNormals_  : arrowNormals_;
    const QVector<QVector3D>& ribs     = isBoat ? boatRibs_     : arrowRibs_;
    const QColor fillColor = isBoat ? boatFillColor_ : arrowFillColor_;
    const QColor ribColor  = isBoat ? boatRibColor_  : arrowRibColor_;

    auto litShaderProgram = shaderProgramMap.value("directional_lit", nullptr);
    auto lineShaderProgram = shaderProgramMap.value("static", nullptr);
    if (!shadowEnabled_) {
        litShaderProgram.reset();
    }

    if ((qFuzzyIsNull(angle_) && position_.isNull()) || (!litShaderProgram && !lineShaderProgram)) {
        return;
    }

    EffectiveShadowParams shadow;
    QVector<QVector3D> rotatedNormals;
    if (litShaderProgram) {
        shadow = effectiveShadowParams();
        rotatedNormals = normals;
        if (!qFuzzyIsNull(angle_)) {
            QMatrix4x4 normalTransform;
            normalTransform.setToIdentity();
            normalTransform.rotate(angle_, 0.0f, 0.0f, 1.0f);
            for (QVector3D& n : rotatedNormals) {
                n = normalTransform.mapVector(n);
                if (n.lengthSquared() > 1e-12f) {
                    n.normalize();
                } else {
                    n = QVector3D(0.0f, 0.0f, 1.0f);
                }
            }
        }
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
        litShaderProgram->setUniformValue(colorLoc, DrawUtils::colorToVector4d(fillColor));
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
            litShaderProgram->setAttributeArray(posLoc, vertices.constData());
            if (normalLoc >= 0) {
                litShaderProgram->enableAttributeArray(normalLoc);
                litShaderProgram->setAttributeArray(normalLoc, rotatedNormals.constData());
            }
            ctx->glDrawArrays(GL_TRIANGLES, 0, vertices.size());
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
        lineShaderProgram->setUniformValue(colorLoc, DrawUtils::colorToVector4d(fillColor));
        lineShaderProgram->enableAttributeArray(posLoc);
        lineShaderProgram->setAttributeArray(posLoc, vertices.constData());
        ctx->glDrawArrays(GL_TRIANGLES, 0, vertices.size());
        lineShaderProgram->disableAttributeArray(posLoc);
        lineShaderProgram->release();
    }

    if (lineShaderProgram && lineShaderProgram->bind()) {
        const int posLoc = lineShaderProgram->attributeLocation("position");
        const int colorLoc = lineShaderProgram->uniformLocation("color");
        const int matrixLoc = lineShaderProgram->uniformLocation("matrix");

        lineShaderProgram->setUniformValue(matrixLoc, mvp);
        lineShaderProgram->setUniformValue(colorLoc, DrawUtils::colorToVector4d(ribColor));
        lineShaderProgram->enableAttributeArray(posLoc);
        lineShaderProgram->setAttributeArray(posLoc, ribs.constData());

        ctx->glLineWidth(2.0f);
        ctx->glDrawArrays(GL_LINES, 0, ribs.size());
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

int NavigationArrow::NavigationArrowRenderImplementation::getSize() const
{
    return size_;
}
