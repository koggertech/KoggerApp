#include "coordinate_axes.h"
#include "draw_utils.h"
#include "text_renderer.h"
#include <cmath>


QVector<QVector3D> CoordinateAxes::CoordinateAxesRenderImplementation::buildSmoothTriangleNormals(const QVector<QVector3D>& tris) const
{
    QVector<QVector3D> normals(tris.size(), QVector3D(0.0f, 0.0f, 1.0f));
    if (tris.size() < 3 || (tris.size() % 3) != 0) {
        return normals;
    }

    QVector<QVector3D> faceNormals(tris.size() / 3, QVector3D(0.0f, 0.0f, 1.0f));
    for (int i = 0; i + 2 < tris.size(); i += 3) {
        QVector3D n = QVector3D::crossProduct(tris[i + 1] - tris[i], tris[i + 2] - tris[i]);
        if (n.lengthSquared() > 1e-12f) {
            n.normalize();
        } else {
            n = QVector3D(0.0f, 0.0f, 1.0f);
        }
        faceNormals[i / 3] = n;
    }

    const auto samePos = [](const QVector3D& a, const QVector3D& b) {
        return std::fabs(a.x() - b.x()) < 1e-4f &&
               std::fabs(a.y() - b.y()) < 1e-4f &&
               std::fabs(a.z() - b.z()) < 1e-4f;
    };

    for (int i = 0; i < tris.size(); ++i) {
        QVector3D acc(0.0f, 0.0f, 0.0f);
        for (int t = 0; t + 2 < tris.size(); t += 3) {
            if (samePos(tris[i], tris[t]) ||
                samePos(tris[i], tris[t + 1]) ||
                samePos(tris[i], tris[t + 2])) {
                acc += faceNormals[t / 3];
            }
        }
        if (acc.lengthSquared() > 1e-12f) {
            acc.normalize();
            normals[i] = acc;
        }
    }

    return normals;
}

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

    QVector<QVector3D> axis_x{{0.0f, 0.0f, 0.0f},       {m_position.x()+10.0f, m_position.y(),       m_position.z()}};
    QVector<QVector3D> axis_y{{0.0f, 0.0f, 0.0f},       {m_position.x(),       m_position.y()+10.0f, m_position.z()}};
    QVector<QVector3D> axis_z{{0.0f, 0.0f, 0.0f},       {m_position.x(),       m_position.y(),       m_position.z()+10.0f}};

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

    TextRenderer::instance().render3D(QString('x'), 0.07, axis_x.last(), QVector3D(0.0f,1.0f,0.0f),ctx,mvp,shaderProgramMap);
    TextRenderer::instance().render3D(QString('y'), 0.07, axis_y.last(), QVector3D(0.0f,1.0f,0.0f),ctx,mvp,shaderProgramMap);
    TextRenderer::instance().render3D(QString('z'), 0.07, axis_z.last(), QVector3D(0.0f,1.0f,0.0f),ctx,mvp,shaderProgramMap);
}

void CoordinateAxes::CoordinateAxesRenderImplementation::render(QOpenGLFunctions *ctx,
                                                                const QMatrix4x4 &model,
                                                                const QMatrix4x4 &view,
                                                                const QMatrix4x4 &projection,
                                                                const QMap<QString,std::shared_ptr<QOpenGLShaderProgram>> &shaderProgramMap) const
{
    if (!shaderProgramMap.contains("static")) {
        return;
    }

    auto lineShaderProgram = shaderProgramMap["static"];
    auto litShaderProgram = shaderProgramMap.value("directional_lit", nullptr);

    QMatrix4x4 compassModelBase = model;
    compassModelBase.rotate(-90.0f, 0.0f, 0.0f, 1.0f);

    QMatrix4x4 compassModelNorth = compassModelBase;
    QMatrix4x4 compassModelSouth = compassModelBase;
    compassModelSouth.rotate(180.0f, 0.0f, 0.0f, 1.0f);

    const float s = 3.0f;

    QVector3D A = s * QVector3D(-2.0f, 0.0f, 0.0f);
    QVector3D C = s * QVector3D( 2.0f, 0.0f, 0.0f);
    QVector3D D = s * QVector3D( 0.0f, 5.0f, 0.0f);
    QVector3D E = s * QVector3D( 0.0f, 0.0f, 1.0f);

    QVector<QVector3D> tris;
    tris << A << E << D
         << E << C << D;

    QVector3D Ar = s * QVector3D(-2.0f, 0.0f, 0.02f);
    QVector3D Cr = s * QVector3D( 2.0f, 0.0f, 0.02f);
    QVector3D Dr = s * QVector3D( 0.0f, 5.0f, 0.02f);
    QVector3D Er = s * QVector3D( 0.0f, 0.0f, 1.02f);

    QVector<QVector3D> ribs;
    ribs << Cr << Dr
         << Dr << Ar
         << Dr << Er
         << Er << Ar
         << Er << Cr;

    const bool useLit = shadowEnabled_ && static_cast<bool>(litShaderProgram);
    QVector<QVector3D> normals;
    EffectiveShadowParams shadow;
    if (useLit) {
        normals = buildSmoothTriangleNormals(tris);
        shadow = effectiveShadowParams();
    }

    const auto drawBody = [&](const QMatrix4x4& mvp, const QColor& color) {
        if (useLit && litShaderProgram && litShaderProgram->bind()) {
            const int posLoc = litShaderProgram->attributeLocation("position");
            const int normalLoc = litShaderProgram->attributeLocation("normal");
            const int matrixLoc = litShaderProgram->uniformLocation("matrix");
            const int colorLoc = litShaderProgram->uniformLocation("color");
            const int lightDirLoc = litShaderProgram->uniformLocation("lightDir");
            const int ambientLoc = litShaderProgram->uniformLocation("ambient");
            const int intensityLoc = litShaderProgram->uniformLocation("intensity");
            const int highlightLoc = litShaderProgram->uniformLocation("highlightIntensity");

            if (posLoc >= 0) {
                litShaderProgram->setUniformValue(matrixLoc, mvp);
                litShaderProgram->setUniformValue(colorLoc, DrawUtils::colorToVector4d(color));
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
                litShaderProgram->enableAttributeArray(posLoc);
                litShaderProgram->setAttributeArray(posLoc, tris.constData());
                if (normalLoc >= 0) {
                    litShaderProgram->enableAttributeArray(normalLoc);
                    litShaderProgram->setAttributeArray(normalLoc, normals.constData());
                }
                ctx->glDrawArrays(GL_TRIANGLES, 0, tris.size());
                if (normalLoc >= 0) {
                    litShaderProgram->disableAttributeArray(normalLoc);
                }
                litShaderProgram->disableAttributeArray(posLoc);
            }
            litShaderProgram->release();
            return;
        }

        if (lineShaderProgram && lineShaderProgram->bind()) {
            const int posLoc = lineShaderProgram->attributeLocation("position");
            const int colorLoc = lineShaderProgram->uniformLocation("color");
            const int matrixLoc = lineShaderProgram->uniformLocation("matrix");
            const int isPointLoc = lineShaderProgram->uniformLocation("isPoint");
            const int isTriangleLoc = lineShaderProgram->uniformLocation("isTriangle");
            if (posLoc >= 0) {
                lineShaderProgram->setUniformValue(matrixLoc, mvp);
                lineShaderProgram->setUniformValue(colorLoc, DrawUtils::colorToVector4d(color));
                lineShaderProgram->setUniformValue(isPointLoc, false);
                lineShaderProgram->setUniformValue(isTriangleLoc, false);
                lineShaderProgram->enableAttributeArray(posLoc);
                lineShaderProgram->setAttributeArray(posLoc, tris.constData());
                ctx->glDrawArrays(GL_TRIANGLES, 0, tris.size());
                lineShaderProgram->disableAttributeArray(posLoc);
            }
            lineShaderProgram->release();
        }
    };

    const auto drawRibs = [&](const QMatrix4x4& mvp, const QColor& color) {
        if (!lineShaderProgram || !lineShaderProgram->bind()) {
            return;
        }

        const int posLoc = lineShaderProgram->attributeLocation("position");
        const int colorLoc = lineShaderProgram->uniformLocation("color");
        const int matrixLoc = lineShaderProgram->uniformLocation("matrix");
        const int isPointLoc = lineShaderProgram->uniformLocation("isPoint");
        const int isTriangleLoc = lineShaderProgram->uniformLocation("isTriangle");
        if (posLoc >= 0) {
            lineShaderProgram->setUniformValue(matrixLoc, mvp);
            lineShaderProgram->setUniformValue(colorLoc, DrawUtils::colorToVector4d(color));
            lineShaderProgram->setUniformValue(isPointLoc, false);
            lineShaderProgram->setUniformValue(isTriangleLoc, false);
            lineShaderProgram->enableAttributeArray(posLoc);
            lineShaderProgram->setAttributeArray(posLoc, ribs.constData());
            ctx->glLineWidth(2.0f);
            ctx->glDrawArrays(GL_LINES, 0, ribs.size());
            ctx->glLineWidth(1.0f);
            lineShaderProgram->disableAttributeArray(posLoc);
        }
        lineShaderProgram->release();
    };

    const QMatrix4x4 mvpNorth = projection * view * compassModelNorth;
    const QMatrix4x4 mvpSouth = projection * view * compassModelSouth;
    drawBody(mvpNorth, QColor(235, 52, 52));
    drawRibs(mvpNorth, QColor(99, 22, 22));
    drawBody(mvpSouth, QColor(47, 132, 227));
    drawRibs(mvpSouth, QColor(10, 40, 120));

    // labels
    GLint vp[4];
    ctx->glGetIntegerv(GL_VIEWPORT, vp);

    QRectF  vportLocal(0, 0, vp[2], vp[3]);
    QRect   viewportRectLocal(0, 0, vp[2], vp[3]);

    QMatrix4x4 textProjection;
    textProjection.ortho(vportLocal.toRect());

    const float scale = 0.8f;

    QMatrix4x4 mvNorth = view * compassModelNorth;
    QMatrix4x4 mvSouth = view * compassModelSouth;

    QVector2D nLabelPos =
        D.project(mvNorth, projection, viewportRectLocal).toVector2D();
    QVector2D sLabelPos =
        D.project(mvSouth, projection, viewportRectLocal).toVector2D();

    nLabelPos.setY(vportLocal.height() - nLabelPos.y());
    sLabelPos.setY(vportLocal.height() - sLabelPos.y());

    nLabelPos.setY(nLabelPos.y() - 4.0f);
    sLabelPos.setY(sLabelPos.y() - 4.0f);

    TextRenderer::instance().render("N", scale, nLabelPos, false, ctx, textProjection, shaderProgramMap);
    TextRenderer::instance().render("S", scale, sLabelPos, false, ctx, textProjection, shaderProgramMap);
}

