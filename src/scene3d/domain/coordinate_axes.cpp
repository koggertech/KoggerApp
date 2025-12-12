#include "coordinate_axes.h"
#include "draw_utils.h"
#include "text_renderer.h"

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

    auto shaderProgram = shaderProgramMap["static"];
    if (!shaderProgram->bind()) {
        qCritical() << "Error binding shader program.";
        return;
    }

    const int posLoc    = shaderProgram->attributeLocation("position");
    const int colorLoc  = shaderProgram->uniformLocation("color");
    const int matrixLoc = shaderProgram->uniformLocation("matrix");

    shaderProgram->enableAttributeArray(posLoc);

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

    { // red
        QMatrix4x4 mvpNorth = projection * view * compassModelNorth;
        shaderProgram->setUniformValue(matrixLoc, mvpNorth);

        QVector<GLfloat> vertices;
        vertices.reserve(tris.size() * 3);
        for (auto it = tris.cbegin(); it != tris.cend(); ++it) {
            vertices << it->x() << it->y() << it->z();
        }

        shaderProgram->setAttributeArray(posLoc, vertices.constData(), 3);
        shaderProgram->setUniformValue(colorLoc,
                                       DrawUtils::colorToVector4d(QColor(235, 52, 52)));
        ctx->glDrawArrays(GL_TRIANGLES, 0, vertices.size() / 3);

        QVector<GLfloat> lineVertices;
        lineVertices.reserve(ribs.size() * 3);
        for (auto it = ribs.cbegin(); it != ribs.cend(); ++it) {
            lineVertices << it->x() << it->y() << it->z();
        }

        shaderProgram->setAttributeArray(posLoc, lineVertices.constData(), 3);
        shaderProgram->setUniformValue(colorLoc,
                                       DrawUtils::colorToVector4d(QColor(99, 22, 22)));

        ctx->glLineWidth(2.0f);
        ctx->glDrawArrays(GL_LINES, 0, lineVertices.size() / 3);
        ctx->glLineWidth(1.0f);
    }

    { // blue
        QMatrix4x4 mvpSouth = projection * view * compassModelSouth;
        shaderProgram->setUniformValue(matrixLoc, mvpSouth);

        QVector<GLfloat> vertices;
        vertices.reserve(tris.size() * 3);
        for (auto it = tris.cbegin(); it != tris.cend(); ++it) {
            vertices << it->x() << it->y() << it->z();
        }

        shaderProgram->setAttributeArray(posLoc, vertices.constData(), 3);
        shaderProgram->setUniformValue(colorLoc, DrawUtils::colorToVector4d(QColor(47, 132, 227)));
        ctx->glDrawArrays(GL_TRIANGLES, 0, vertices.size() / 3);

        QVector<GLfloat> lineVertices;
        lineVertices.reserve(ribs.size() * 3);
        for (auto it = ribs.cbegin(); it != ribs.cend(); ++it) {
            lineVertices << it->x() << it->y() << it->z();
        }

        shaderProgram->setAttributeArray(posLoc, lineVertices.constData(), 3);
        shaderProgram->setUniformValue(colorLoc, DrawUtils::colorToVector4d(QColor(10, 40, 120)));

        ctx->glLineWidth(2.0f);
        ctx->glDrawArrays(GL_LINES, 0, lineVertices.size() / 3);
        ctx->glLineWidth(1.0f);
    }

    shaderProgram->disableAttributeArray(posLoc);
    shaderProgram->release();

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
