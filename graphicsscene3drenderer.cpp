#include "graphicsscene3drenderer.h"
#include <drawutils.h>

#include <bottomtrack.h>
#include <surface.h>
#include <pointgroup.h>
#include <polygongroup.h>

#include <QThread>
#include <QDebug>

#include <textrenderer.h>

#include <ft2build.h>
#include FT_FREETYPE_H

GraphicsScene3dRenderer::GraphicsScene3dRenderer()
{
    m_shaderProgramMap["height"] = std::make_shared<QOpenGLShaderProgram>();
    m_shaderProgramMap["static"] = std::make_shared<QOpenGLShaderProgram>();
    m_shaderProgramMap["text"]   = std::make_shared<QOpenGLShaderProgram>();
    m_shaderProgramMap["texture"]   = std::make_shared<QOpenGLShaderProgram>();
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

    drawObjects();

    TextRenderer::instance();
}

void GraphicsScene3dRenderer::drawObjects()
{
    QMatrix4x4 model, view, projection;

    view = m_camera.m_view;
    projection.perspective(m_camera.fov(), m_viewSize.width()/m_viewSize.height(), 1.0f, 11000.0f);
    model.scale(1.0f, 1.0f, m_verticalScale);

    m_model = std::move(model);
    m_projection = std::move(projection);

    glEnable(GL_DEPTH_TEST);

    m_planeGridRenderImpl.render(this, m_model, view, m_projection, m_shaderProgramMap);
    m_bottomTrackRenderImpl.render(this, m_model, view, m_projection, m_shaderProgramMap);
    m_surfaceRenderImpl.render(this, m_projection * view * m_model, m_shaderProgramMap);
    m_pointGroupRenderImpl.render(this, m_projection * view * m_model, m_shaderProgramMap);
    m_polygonGroupRenderImpl.render(this, m_projection * view * m_model, m_shaderProgramMap);
    m_boatTrackRenderImpl.render(this, m_projection * view * m_model, m_shaderProgramMap);
    m_navigationArrowRenderImpl.render(this, m_projection * view * m_model, m_shaderProgramMap);
    glDisable(GL_DEPTH_TEST);

    //-----------Draw axes-------------
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    glViewport(viewport[2]-100,0,100,100);
    glDisable(GL_DEPTH_TEST);

    QMatrix4x4 axesView;
    QMatrix4x4 axesProjection;
    QMatrix4x4 axesModel;

    m_axesThumbnailCamera.setDistance(35);
    axesView = m_axesThumbnailCamera.m_view;
    axesProjection.perspective(m_camera.fov(), 100/100, 1.0f, 11000.0f);

    m_coordAxesRenderImpl.render(this, axesModel, axesView, axesProjection, m_shaderProgramMap);

    glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);

    //----------->Draw selection rect<-----------//
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0,viewport[2],0,viewport[3],-1,1);
    glBegin(GL_LINE_LOOP);
    auto color = DrawUtils::colorToVector4d(QColor(0.0f, 104.0f, 145.0f));
    glColor3f(color.x(), color.y(), color.z());
    glVertex2f(m_comboSelectionRect.topLeft().x(),m_comboSelectionRect.topLeft().y());
    glVertex2f(m_comboSelectionRect.topRight().x(),m_comboSelectionRect.topRight().y());
    glVertex2f(m_comboSelectionRect.bottomRight().x(),m_comboSelectionRect.bottomRight().y());
    glVertex2f(m_comboSelectionRect.bottomLeft().x(),m_comboSelectionRect.bottomLeft().y());
    glEnd();

    //------------>Draw scene bounding box<======//
    if(m_isSceneBoundingBoxVisible){
        if(!m_shaderProgramMap.contains("static"))
            return;

        auto shaderProgram = m_shaderProgramMap["static"];

        if (!shaderProgram->bind()){
            qCritical() << "Error binding shader program.";
            return;
        }

        QVector<QVector3D> boundingBox{
            // Bottom horizontal edges
            {m_boundingBox.minimumX(),m_boundingBox.minimumY(),m_boundingBox.minimumZ()},
            {m_boundingBox.minimumX()+m_boundingBox.length(),m_boundingBox.minimumY(),m_boundingBox.minimumZ()},
            {m_boundingBox.minimumX(),m_boundingBox.minimumY(),m_boundingBox.minimumZ()},
            {m_boundingBox.minimumX(),m_boundingBox.minimumY()+m_boundingBox.width(),m_boundingBox.minimumZ()},
            {m_boundingBox.minimumX(),m_boundingBox.minimumY()+m_boundingBox.width(),m_boundingBox.minimumZ()},
            {m_boundingBox.minimumX()+m_boundingBox.length(),m_boundingBox.minimumY()+m_boundingBox.width(),m_boundingBox.minimumZ()},
            {m_boundingBox.minimumX()+m_boundingBox.length(),m_boundingBox.minimumY()+m_boundingBox.width(),m_boundingBox.minimumZ()},
            {m_boundingBox.minimumX()+m_boundingBox.length(),m_boundingBox.minimumY(),m_boundingBox.minimumZ()},

            //Top horizontal edges
            {m_boundingBox.minimumX(),m_boundingBox.minimumY(),m_boundingBox.minimumZ()+m_boundingBox.height()},
            {m_boundingBox.minimumX()+m_boundingBox.length(),m_boundingBox.minimumY(),m_boundingBox.minimumZ()+m_boundingBox.height()},
            {m_boundingBox.minimumX(),m_boundingBox.minimumY(),m_boundingBox.minimumZ()+m_boundingBox.height()},
            {m_boundingBox.minimumX(),m_boundingBox.minimumY()+m_boundingBox.width(),m_boundingBox.minimumZ()+m_boundingBox.height()},
            {m_boundingBox.minimumX(),m_boundingBox.minimumY()+m_boundingBox.width(),m_boundingBox.minimumZ()+m_boundingBox.height()},
            {m_boundingBox.minimumX()+m_boundingBox.length(),m_boundingBox.minimumY()+m_boundingBox.width(),m_boundingBox.minimumZ()+m_boundingBox.height()},
            {m_boundingBox.minimumX()+m_boundingBox.length(),m_boundingBox.minimumY()+m_boundingBox.width(),m_boundingBox.minimumZ()+m_boundingBox.height()},
            {m_boundingBox.minimumX()+m_boundingBox.length(),m_boundingBox.minimumY(),m_boundingBox.minimumZ()+m_boundingBox.height()},

            // Vertical Edges
            {m_boundingBox.minimumX(),m_boundingBox.minimumY(),m_boundingBox.minimumZ()},
            {m_boundingBox.minimumX(),m_boundingBox.minimumY(),m_boundingBox.minimumZ()+m_boundingBox.height()},
            {m_boundingBox.minimumX()+m_boundingBox.length(),m_boundingBox.minimumY(),m_boundingBox.minimumZ()},
            {m_boundingBox.minimumX()+m_boundingBox.length(),m_boundingBox.minimumY(),m_boundingBox.minimumZ()+m_boundingBox.height()},
            {m_boundingBox.minimumX()+m_boundingBox.length(),m_boundingBox.minimumY()+m_boundingBox.width(),m_boundingBox.minimumZ()},
            {m_boundingBox.minimumX()+m_boundingBox.length(),m_boundingBox.minimumY()+m_boundingBox.width(),m_boundingBox.minimumZ()+m_boundingBox.height()},
            {m_boundingBox.minimumX(),m_boundingBox.minimumY()+m_boundingBox.width(),m_boundingBox.minimumZ()},
            {m_boundingBox.minimumX(),m_boundingBox.minimumY()+m_boundingBox.width(),m_boundingBox.minimumZ()+m_boundingBox.height()}
        };

        int posLoc    = shaderProgram->attributeLocation("position");
        int matrixLoc = shaderProgram->uniformLocation("matrix");
        int colorLoc  = shaderProgram->uniformLocation("color");

        shaderProgram->setUniformValue(colorLoc, DrawUtils::colorToVector4d(QColor(0.0f, 104.0f, 145.0f, 0.0f)));
        shaderProgram->setUniformValue(matrixLoc, m_projection*view*m_model);
        shaderProgram->enableAttributeArray(posLoc);
        shaderProgram->setAttributeArray(posLoc, boundingBox.constData());

        glEnable(GL_DEPTH_TEST);
        glLineWidth(2.0f);
        glDrawArrays(GL_LINES, 0, boundingBox.size());
        glLineWidth(1.0f);
        glDisable(GL_DEPTH_TEST);

        shaderProgram->disableAttributeArray(posLoc);
        shaderProgram->release();
    }
}
