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

    shaderProgram->setUniformValue(matrixLoc, projection * view * model );
    shaderProgram->enableAttributeArray(posLoc);

    QVector<QVector3D> axis_x{{0.0f, 0.0f, 0.0f},       {m_position.x()+10.0f, m_position.y(),       m_position.z()}};
    QVector<QVector3D> axis_y{{0.0f, 0.0f, 0.0f},       {m_position.x(),       m_position.y()+10.0f, m_position.z()}};
    QVector<QVector3D> axis_z{{0.0f, 0.0f, 0.0f},       {m_position.x(),       m_position.y(),       m_position.z()+10.0f}};

    ctx->glLineWidth(4.0f);
    shaderProgram->setAttributeArray(posLoc, axis_x.constData());
    shaderProgram->setUniformValue(colorLoc, DrawUtils::colorToVector4d(QColor(239, 55, 82)));
    ctx->glDrawArrays(GL_LINE_STRIP, 0, axis_x.size());
    shaderProgram->setAttributeArray(posLoc, axis_y.constData());
    shaderProgram->setUniformValue(colorLoc, DrawUtils::colorToVector4d(QColor(109, 157, 30)));
    ctx->glDrawArrays(GL_LINE_STRIP, 0, axis_y.size());
    shaderProgram->setAttributeArray(posLoc, axis_z.constData());
    shaderProgram->setUniformValue(colorLoc, DrawUtils::colorToVector4d(QColor(47, 132, 227)));
    ctx->glDrawArrays(GL_LINE_STRIP, 0, axis_z.size());
    ctx->glLineWidth(1.0f);

    shaderProgram->disableAttributeArray(posLoc);
    shaderProgram->release();

    // Rendering text
    QRectF vport = DrawUtils::viewportRect(ctx);
    QMatrix4x4 textProjection;
    textProjection.ortho(vport.toRect());

    const float scale = 1.0;

    QVector2D xLabelPos = axis_x.last().project(view * model, projection, vport.toRect()).toVector2D();
    QVector2D yLabelPos = axis_y.last().project(view * model, projection, vport.toRect()).toVector2D();
    QVector2D zLabelPos = axis_z.last().project(view * model, projection, vport.toRect()).toVector2D();

    xLabelPos.setY(vport.height() - xLabelPos.y());
    yLabelPos.setY(vport.height() - yLabelPos.y());
    zLabelPos.setY(vport.height() - zLabelPos.y());

    TextRenderer::instance().render("n", scale, xLabelPos, false, ctx, textProjection,shaderProgramMap);
    TextRenderer::instance().render("e", scale, yLabelPos, false, ctx, textProjection,shaderProgramMap);
    TextRenderer::instance().render("a", scale, zLabelPos, false, ctx, textProjection,shaderProgramMap);
}
