#include "graphicsscene3drenderer.h"
#include <drawutils.h>

#include <bottomtrack.h>
#include <surface.h>
#include <pointgroup.h>
#include <polygongroup.h>

#include <QThread>
#include <QDebug>

//#include <textrenderer.h> // TODO
//#include <ft2build.h>
//#include FT_FREETYPE_H


GraphicsScene3dRenderer::GraphicsScene3dRenderer()
{
    m_shaderProgramMap["height"]     = std::make_shared<QOpenGLShaderProgram>();
    m_shaderProgramMap["static"]     = std::make_shared<QOpenGLShaderProgram>();
    m_shaderProgramMap["static_sec"] = std::make_shared<QOpenGLShaderProgram>();
    m_shaderProgramMap["text"]       = std::make_shared<QOpenGLShaderProgram>();
    m_shaderProgramMap["texture"]    = std::make_shared<QOpenGLShaderProgram>();
    m_shaderProgramMap["mosaic"]     = std::make_shared<QOpenGLShaderProgram>();
}

GraphicsScene3dRenderer::~GraphicsScene3dRenderer()
{}

void GraphicsScene3dRenderer::initialize()
{
    initializeOpenGLFunctions();

    m_isInitialized = true;

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.3f, 0.3f, 0.3f, 0.0f);

    // static
    if (!m_shaderProgramMap["static"]->addCacheableShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/base.vsh"))
        qCritical() << "Error adding vertex shader from source file.";
    if (!m_shaderProgramMap["static"]->addCacheableShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/static_color.fsh"))
        qCritical() << "Error adding fragment shader from source file.";
    if (!m_shaderProgramMap["static"]->link())
        qCritical() << "Error linking shaders in shader program.";
    // static sec
    if (!m_shaderProgramMap["static_sec"]->addCacheableShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/base_sec.vsh"))
        qCritical() << "Error adding vertex shader from source file.";
    if (!m_shaderProgramMap["static_sec"]->addCacheableShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/static_color.fsh"))
        qCritical() << "Error adding fragment shader from source file.";
    if (!m_shaderProgramMap["static_sec"]->link())
        qCritical() << "Error linking shaders in shader program.";
    // height
    if (!m_shaderProgramMap["height"]->addCacheableShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/base.vsh"))
        qCritical() << "Error adding vertex shader from source file.";
    if (!m_shaderProgramMap["height"]->addCacheableShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/height_color.fsh"))
        qCritical() << "Error adding fragment shader from source file.";
    if (!m_shaderProgramMap["height"]->link())
        qCritical() << "Error linking shaders in shader program.";

    // mosaic
    if (!m_shaderProgramMap["mosaic"]->addCacheableShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/mosaic.vsh"))
        qCritical() << "Error adding mosaic vertex shader from source file.";
    if (!m_shaderProgramMap["mosaic"]->addCacheableShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/mosaic.fsh"))
        qCritical() << "Error adding mosaic fragment shader from source file.";
    if (!m_shaderProgramMap["mosaic"]->link())
        qCritical() << "Error linking mosaic shaders in shader program.";
}

void GraphicsScene3dRenderer::render()
{
    glDepthMask(true);

    glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    drawObjects();

    //TextRenderer::instance(); TODO
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
    m_planeGridRenderImpl.render(this,       m_model, view, m_projection, m_shaderProgramMap);
    m_bottomTrackRenderImpl.render(this,     m_model, view, m_projection, m_shaderProgramMap);
    m_surfaceRenderImpl.render(this,         m_projection * view * m_model, m_shaderProgramMap);
    sideScanViewRenderImpl_.render(this,     m_projection * view * m_model, m_shaderProgramMap);
    m_pointGroupRenderImpl.render(this,      m_projection * view * m_model, m_shaderProgramMap);
    m_polygonGroupRenderImpl.render(this,    m_projection * view * m_model, m_shaderProgramMap);
    m_boatTrackRenderImpl.render(this,       m_projection * view * m_model, m_shaderProgramMap);
    navigationArrowRenderImpl_.render(this,  m_projection * view * m_model, m_shaderProgramMap);
    glDisable(GL_DEPTH_TEST);

    //-----------Draw axes-------------
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    glViewport(viewport[2]-100,0,100,100);
    glDisable(GL_DEPTH_TEST);

    QMatrix4x4 axesView;
    QMatrix4x4 axesProjection;
    QMatrix4x4 axesModel;

    // m_axesThumbnailCamera.setDistance(35);
    axesView = m_axesThumbnailCamera.m_view;
    axesProjection.perspective(m_camera.fov(), 100/100, 1.0f, 11000.0f);

    m_coordAxesRenderImpl.render(this, axesModel, axesView, axesProjection, m_shaderProgramMap);

    glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);

    //-----------Draw selection rect-------------
    if(!m_shaderProgramMap.contains("static_sec"))
        return;

    auto shaderProgram = m_shaderProgramMap["static_sec"];
    if (!shaderProgram->bind()) {
        qCritical() << "Error binding shader program.";
        return;
    }

    const int colorLoc  = shaderProgram->uniformLocation("color");
    shaderProgram->setUniformValue(colorLoc, DrawUtils::colorToVector4d(QColor(0.0f, 104.0f, 145.0f, 0.0f)));
    shaderProgram->enableAttributeArray(0);

    const float halfWidth = viewport[2] / 2.0f;
    const float halfHeight = viewport[3] / 2.0f;
    QVector<QVector2D> rectVert = { { (m_comboSelectionRect.topLeft().x()     / halfWidth)  - 1.0f,
                                      (m_comboSelectionRect.topLeft().y()     / halfHeight) - 1.0f },
                                    { (m_comboSelectionRect.topRight().x()    / halfWidth)  - 1.0f,
                                      (m_comboSelectionRect.topRight().y()    / halfHeight) - 1.0f },
                                    { (m_comboSelectionRect.bottomRight().x() / halfWidth)  - 1.0f,
                                      (m_comboSelectionRect.bottomRight().y() / halfHeight) - 1.0f },
                                    { (m_comboSelectionRect.bottomLeft().x()  / halfWidth)  - 1.0f,
                                      (m_comboSelectionRect.bottomLeft().y()  / halfHeight) - 1.0f } };

    shaderProgram->setAttributeArray(0, rectVert.constData());
    glDrawArrays(GL_LINE_LOOP, 0, rectVert.size());
    shaderProgram->release();

    //-----------Draw scene bounding box-------------
    if (m_isSceneBoundingBoxVisible) {
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
