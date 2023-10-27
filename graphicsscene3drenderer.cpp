#include "graphicsscene3drenderer.h"

#include <bottomtrack.h>
#include <surface.h>
#include <pointgroup.h>
#include <polygongroup.h>
#include <scenegraphicsobject.h>

#include <QThread>
#include <QDebug>
#include <QtMath>

//static const qreal sphereRadius = -500.0f;

GraphicsScene3dRenderer::GraphicsScene3dRenderer()
{
    m_shaderProgramMap["height"] = std::make_shared<QOpenGLShaderProgram>();
    m_shaderProgramMap["static"] = std::make_shared<QOpenGLShaderProgram>();
}

GraphicsScene3dRenderer::~GraphicsScene3dRenderer()
{}

void GraphicsScene3dRenderer::initialize()
{
    initializeOpenGLFunctions();

    m_isInitialized = true;

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.3f, 0.3f, 0.3f, 0.0f);

    // ---------
    bool success = m_shaderProgramMap["static"]->addCacheableShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/base.vsh");

    if (!success) qCritical() << "Error adding vertex shader from source file.";

    success = m_shaderProgramMap["static"]->addCacheableShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/staticcolor.fsh");

    if (!success) qCritical() << "Error adding fragment shader from source file.";

    success = m_shaderProgramMap["static"]->link();

    if (!success) qCritical() << "Error linking shaders in shader program.";

    // --------
    success = m_shaderProgramMap["height"]->addCacheableShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/base.vsh");

    if (!success) qCritical() << "Error adding vertex shader from source file.";

    success = m_shaderProgramMap["height"]->addCacheableShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/heightcolor.fsh");

    if (!success) qCritical() << "Error adding fragment shader from source file.";

    success = m_shaderProgramMap["height"]->link();

    if (!success) qCritical() << "Error linking shaders in shader program.";
}

void GraphicsScene3dRenderer::render()
{
    glDepthMask(true);

    glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

    glFrontFace(GL_CW);
    glEnable(GL_DEPTH_TEST);

    drawObjects();

    glDisable(GL_DEPTH_TEST);
}

void GraphicsScene3dRenderer::drawObjects()
{
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);

    QMatrix4x4 model, view, projection;

    view = m_camera.m_view;
    model.rotate(90.0f, QVector3D(1.0f, 0.0f, 0.0f));
    projection.perspective(m_camera.fov(), m_viewSize.width()/m_viewSize.height(), 1.0f, 5000.0f);

    glViewport(viewport[2]-100,0,100,100);
    glDisable(GL_DEPTH_TEST);

    QMatrix4x4 axesView;
    QMatrix4x4 axesProjection;

    axesView = m_axesThumbnailCamera.m_view;
    axesProjection.perspective(m_camera.fov(), 100/100, 1.0f, 5000.0f);

    m_coordAxesRenderImpl.render(this, axesProjection * axesView * model, m_shaderProgramMap);

    glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);

    glEnable(GL_DEPTH_TEST);
    //m_coordAxesRenderImpl.render(this, projection * view * model, m_shaderProgramMap);
    m_planeGridRenderImpl.render(this, projection * view * model, m_shaderProgramMap);
    m_bottomTrackRenderImpl.render(this, projection * view * model, m_shaderProgramMap);
    m_surfaceRenderImpl.render(this, projection * view * model, m_shaderProgramMap);
    m_pointGroupRenderImpl.render(this, projection * view * model, m_shaderProgramMap);
    m_polygonGroupRenderImpl.render(this, projection * view * model, m_shaderProgramMap);

    //for(const auto& object : qAsConst(m_objectList))
    //    object->draw(this, m_projection * m_view * m_model, m_shaderProgramMap);
}
