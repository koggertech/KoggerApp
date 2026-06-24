#include "scene3d_renderer.h"
#include "draw_utils.h"

#include "bottom_track.h"
#include "point_group.h"
#include "polygon_group.h"

#include <QThread>
#include <QDebug>
#include <QOpenGLContext>
#include <QCoreApplication>

#include "text_renderer.h"
#include "themes.h"
//#include "ft2build.h"

//#include FT_FREETYPE_H

GraphicsScene3dRenderer::GraphicsScene3dRenderer() :
    scaleFactor_(1.0f)
{
#if defined(Q_OS_ANDROID) || defined(LINUX_ES)
    scaleFactor_ = 2.0f;
#endif
    m_shaderProgramMap["height"]     = std::make_shared<QOpenGLShaderProgram>();
    m_shaderProgramMap["static"]     = std::make_shared<QOpenGLShaderProgram>();
    m_shaderProgramMap["static_sec"] = std::make_shared<QOpenGLShaderProgram>();
    m_shaderProgramMap["usbl_arrow"] = std::make_shared<QOpenGLShaderProgram>();
    m_shaderProgramMap["directional_lit"]  = std::make_shared<QOpenGLShaderProgram>();
    m_shaderProgramMap["text"]       = std::make_shared<QOpenGLShaderProgram>();
    m_shaderProgramMap["text_back"]  = std::make_shared<QOpenGLShaderProgram>();
    m_shaderProgramMap["mosaic"]     = std::make_shared<QOpenGLShaderProgram>();
    m_shaderProgramMap["image"]      = std::make_shared<QOpenGLShaderProgram>();
    m_shaderProgramMap["isobaths"]   = std::make_shared<QOpenGLShaderProgram>();
}

GraphicsScene3dRenderer::~GraphicsScene3dRenderer()
{
    if (QOpenGLContext::currentContext()) {
        TextRenderer::instance().cleanup();
        m_shaderProgramMap.clear();
    }
}

void GraphicsScene3dRenderer::initialize()
{
    initializeOpenGLFunctions();

    m_isInitialized = true;

    // check OpenGL version
    //if (auto* glContext = QOpenGLContext::currentContext()) {
    //    const QSurfaceFormat fmt = glContext->format();
    //    qInfo() << "GL context:"
    //            << (fmt.renderableType() == QSurfaceFormat::OpenGLES ? "OpenGLES" : "OpenGL")
    //            << "version" << fmt.majorVersion() << "." << fmt.minorVersion()
    //            << "profile" << fmt.profile();
    //}

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
    // usbl arrow
    const char* usblArrowVertexPath = ":/shaders/base.vsh";
    const char* usblArrowFragmentPath = ":/shaders/usbl_arrow.fsh";
    if (!m_shaderProgramMap["usbl_arrow"]->addCacheableShaderFromSourceFile(QOpenGLShader::Vertex, usblArrowVertexPath))
        qCritical() << "Error adding usbl_arrow vertex shader from source file.";
    if (!m_shaderProgramMap["usbl_arrow"]->addCacheableShaderFromSourceFile(QOpenGLShader::Fragment, usblArrowFragmentPath))
        qCritical() << "Error adding usbl_arrow fragment shader from source file.";
    if (!m_shaderProgramMap["usbl_arrow"]->link())
        qCritical() << "Error linking usbl_arrow shaders in shader program.";
    // directional-lit color shader (used by arrow/compass/axes)
    if (!m_shaderProgramMap["directional_lit"]->addCacheableShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/directional_lit.vsh")) {
        qCritical() << "Error adding directional_lit vertex shader from source file.";
        qCritical().noquote() << m_shaderProgramMap["directional_lit"]->log();
    }
    if (!m_shaderProgramMap["directional_lit"]->addCacheableShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/directional_lit.fsh")) {
        qCritical() << "Error adding directional_lit fragment shader from source file.";
        qCritical().noquote() << m_shaderProgramMap["directional_lit"]->log();
    }
    if (!m_shaderProgramMap["directional_lit"]->link()) {
        qCritical() << "Error linking directional_lit shaders in shader program.";
        qCritical().noquote() << m_shaderProgramMap["directional_lit"]->log();
    }
    // height
    if (!m_shaderProgramMap["height"]->addCacheableShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/base.vsh"))
        qCritical() << "Error adding vertex shader from source file.";
    if (!m_shaderProgramMap["height"]->addCacheableShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/height_color.fsh"))
        qCritical() << "Error adding fragment shader from source file.";
    if (!m_shaderProgramMap["height"]->link())
        qCritical() << "Error linking shaders in shader program.";

    // mosaic
    if (!m_shaderProgramMap["mosaic"]->addCacheableShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/mosaic.vsh")) {
        qCritical() << "Error adding mosaic vertex shader from source file.";
        qCritical().noquote() << m_shaderProgramMap["mosaic"]->log();
    }
    if (!m_shaderProgramMap["mosaic"]->addCacheableShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/mosaic.fsh")) {
        qCritical() << "Error adding mosaic fragment shader from source file.";
        qCritical().noquote() << m_shaderProgramMap["mosaic"]->log();
    }
    if (!m_shaderProgramMap["mosaic"]->link()) {
        qCritical() << "Error linking mosaic shaders in shader program.";
        qCritical().noquote() << m_shaderProgramMap["mosaic"]->log();
    }

    // image
    if (!m_shaderProgramMap["image"]->addCacheableShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/image.vsh"))
        qCritical() << "Error adding image vertex shader from source file.";
    if (!m_shaderProgramMap["image"]->addCacheableShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/image.fsh"))
        qCritical() << "Error adding image fragment shader from source file.";
    if (!m_shaderProgramMap["image"]->link())
        qCritical() << "Error linking image shaders in shader program.";

    // text
    if (!m_shaderProgramMap["text"]->addCacheableShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/text.vsh"))
        qCritical() << "Error adding text vertex shader from source file.";
    if (!m_shaderProgramMap["text"]->addCacheableShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/text.fsh"))
        qCritical() << "Error adding text fragment shader from source file.";
    if (!m_shaderProgramMap["text"]->link())
        qCritical() << "Error linking text shaders in shader program.";

    // text back
    if (!m_shaderProgramMap["text_back"]->addCacheableShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/text_back.vsh"))
        qCritical() << "Error adding text_back vertex shader from source file.";
    if (!m_shaderProgramMap["text_back"]->addCacheableShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/text_back.fsh"))
        qCritical() << "Error adding text_back fragment shader from source file.";
    if (!m_shaderProgramMap["text_back"]->link())
        qCritical() << "Error linking text_back shaders in shader program.";

    // isobaths
    if (!m_shaderProgramMap["isobaths"]->addCacheableShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/isobaths_colored.vsh")) {
        qCritical() << "Error adding isobaths vertex shader from source file.";
        qCritical().noquote() << m_shaderProgramMap["isobaths"]->log();
    }
    if (!m_shaderProgramMap["isobaths"]->addCacheableShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/isobaths_colored.fsh")) {
        qCritical() << "Error adding isobaths fragment shader from source file.";
        qCritical().noquote() << m_shaderProgramMap["isobaths"]->log();
    }
    if (!m_shaderProgramMap["isobaths"]->link()) {
        qCritical() << "Error linking isobaths shaders in shader program.";
        qCritical().noquote() << m_shaderProgramMap["isobaths"]->log();
    }
}

void GraphicsScene3dRenderer::render()
{
    glDepthMask(true);

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f); // back color
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    TextRenderer::instance().setFontPixelSize(qRound(22 * renderScale()));

    drawObjects();

    TextRenderer::instance();
    TextRenderer::instance().setColor("white");
}

void GraphicsScene3dRenderer::drawObjects()
{
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);

    QMatrix4x4 model, view, projection;

    const float perspectiveEdge     { 5000.0f };
    const float nearPlanePersp      { 1.0f };
    const float farPlanePersp       { 20000.0f };
    const float nearPlaneOrthoCoeff { 0.05f };
    const float farPlaneOrthoCoeff  { 1.2f };

    float perspCoeff = m_camera.getHeightAboveGround() /  perspectiveEdge;
    qreal perspFixFov = m_camera.fov() + m_camera.fov() * perspCoeff;

    if (m_camera.getIsPerspective()) {
        projection.perspective(perspFixFov, m_viewSize.width() / m_viewSize.height(), nearPlanePersp, farPlanePersp);
    }
    else {
        float orth_v = m_camera.getHeightAboveGround();
        float aspect_ratio = m_viewSize.width() / m_viewSize.height();
        projection.ortho(-orth_v*aspect_ratio, orth_v*aspect_ratio, -orth_v, orth_v, orth_v * nearPlaneOrthoCoeff, orth_v * farPlaneOrthoCoeff);
    }

    view = m_camera.m_view;
    model.scale(1.0f, 1.0f, m_verticalScale);

    m_model = model;
    m_projection = projection;

    bool isOut = m_camera.getIsFarAwayFromOriginLla();

    mapViewRenderImpl_.render(this, m_model, view, m_projection, m_shaderProgramMap);

    glEnable(GL_DEPTH_TEST);
    if (!isOut) {
        imageViewRenderImpl_.render(this, m_projection * view * m_model, m_shaderProgramMap);
        m_pointGroupRenderImpl.render(this, m_projection * view * m_model, m_shaderProgramMap);
        m_polygonGroupRenderImpl.render(this, m_projection * view * m_model, m_shaderProgramMap);
        usblViewRenderImpl_.render(this, m_projection * view * m_model, m_shaderProgramMap);
    }
    glDisable(GL_DEPTH_TEST);

    if (!isOut) {
        glEnable(GL_DEPTH_TEST);
        surfaceViewRenderImpl_.render(this,      m_projection * view * m_model, m_shaderProgramMap);
        //isobathsViewRenderImpl_.render(this,     m_model, view, m_projection, m_shaderProgramMap);
        m_bottomTrackRenderImpl.render(this,     m_model, view, m_projection, m_shaderProgramMap);
        m_boatTrackRenderImpl.render(this,       m_model, view, m_projection, m_shaderProgramMap);
        glDisable(GL_DEPTH_TEST);

        //-----------Contacts-------------
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        contactsRenderImpl_.render(this, m_model, view, m_projection, m_shaderProgramMap);
        glDisable(GL_BLEND);

        //-----------Draw selection rect-------------
        if (!m_shaderProgramMap.contains("static_sec")) {
            return;
        }

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

        //-----------Grid & Draw scene bounding box-------------
        if (gridVisibility_ && planeGridType_) {
            if (!m_shaderProgramMap.contains("static")) {
                return;
            }

            auto shaderProgram = m_shaderProgramMap["static"];
            if (!shaderProgram->bind()) {
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

            m_planeGridRenderImpl.render(this, m_model, view, m_projection, m_shaderProgramMap);
        }
        if (gridVisibility_ && !planeGridType_) {
            m_planeGridRenderImpl.render(this, m_model, view, m_projection, m_shaderProgramMap);
        }

        //-----------Navigation arrow-------------
        {
            glEnable(GL_DEPTH_TEST);

            QMatrix4x4 nModel;
            nModel.setToIdentity();
            nModel.translate(navigationArrowRenderImpl_.getPosition());
            nModel.rotate(navigationArrowRenderImpl_.getAngle(), 0.f, 0.f, 1.f);
            float distance =  m_camera.distToFocusPoint();
            float perspFixFovRad = qDegreesToRadians(perspFixFov);
            float factor = 2.0f * distance * std::tan(perspFixFovRad * 0.5f) / m_viewSize.height();
            float worldScale = factor * 7.f * scaleFactor_;
            float navigationArrowSizeFactor = 1.0f;
            switch (qBound(1, navigationArrowRenderImpl_.getSize(), 5)) {
            case 1: navigationArrowSizeFactor = 1.0f; break;
            case 2: navigationArrowSizeFactor = 2.0f; break;
            case 3: navigationArrowSizeFactor = 3.0f; break;
            case 4: navigationArrowSizeFactor = 4.0f; break;
            case 5: navigationArrowSizeFactor = 5.0f; break;
            default: break;
            }
            nModel.scale(worldScale * navigationArrowSizeFactor);
            navigationArrowRenderImpl_.render(this, projection * view * nModel, m_shaderProgramMap);

            glDisable(GL_DEPTH_TEST);
        }

        //-----------Overlays that should work in any camera/reference state-------------
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        geoJsonLayerRenderImpl_.render(this, m_model, view, m_projection, m_shaderProgramMap);
        rulerToolRenderImpl_.render(this, m_model, view, m_projection, m_shaderProgramMap);
        glDisable(GL_BLEND);
    }

    const bool scaleBarVisible = scaleBar_ && m_camera.getIsPerspective();

    //-----------Compass-------------
    if (compass_) {
        int compassBase = 250;
        switch (compassSize_) {
        case 1: compassBase = 150; break;
        case 2: compassBase = 250; break;
        case 3: compassBase = 350; break;
        case 4: compassBase = 450; break;
        case 5: compassBase = 550; break;
        default: compassBase = 250; break;
        }
        constexpr float kCompassSizeFactor = 0.7f;
        const int compassSizePx = qRound(compassBase * renderScale() * kCompassSizeFactor);

        int compassX = viewport[0];
        int compassY = viewport[1];
        switch (compassPos_) {
        case 1: compassX = viewport[0] + viewport[2] - compassSizePx; compassY = viewport[1];                              break;
        case 2: compassX = viewport[0];                               compassY = viewport[1];                              break;
        case 3: compassX = viewport[0] + viewport[2] - compassSizePx; compassY = viewport[1] + viewport[3] - compassSizePx; break;
        default: compassX = viewport[0];                              compassY = viewport[1];                              break;
        }

        // bottom-right compass shares the corner with the scale bar — lift it just above
        if (scaleBarVisible && compassPos_ == 1) {
            const float charH = static_cast<float>(TextRenderer::instance().getCharPixelHeight());
            compassY += static_cast<int>(16.0f + 6.0f + 3.0f + charH + 4.0f);
        }

        drawCompass(compassX, compassY, compassSizePx);
    }

    //-----------Scale bar-------------
    if (scaleBarVisible) {
        const QRect vportRect(viewport[0], viewport[1], viewport[2], viewport[3]);
        drawScaleBar(vportRect, view);
    }
}

void GraphicsScene3dRenderer::drawCompass(int x, int y, int sizePx)
{
    GLint oldVP[4]; glGetIntegerv(GL_VIEWPORT, oldVP);
    GLboolean oldScissor = glIsEnabled(GL_SCISSOR_TEST);
    GLint oldScBox[4]; glGetIntegerv(GL_SCISSOR_BOX, oldScBox);

    glViewport(x, y, sizePx, sizePx);

    glEnable(GL_SCISSOR_TEST);
    glScissor(x, y, sizePx, sizePx);
    glClear(GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LEQUAL);

    QMatrix4x4 axesView, axesProjection, axesModel;
    m_axesThumbnailCamera.setDistance(55);
    axesView = m_axesThumbnailCamera.m_view;
    axesProjection.perspective(m_camera.fov(), 1.0f, 1.0f, 11000.0f);

    compassRenderImpl_.render(this, axesModel, axesView, axesProjection, m_shaderProgramMap);

    glViewport(oldVP[0], oldVP[1], oldVP[2], oldVP[3]);
    glScissor(oldScBox[0], oldScBox[1], oldScBox[2], oldScBox[3]);
    if (!oldScissor) {
        glDisable(GL_SCISSOR_TEST);
    }
}

void GraphicsScene3dRenderer::drawScaleBar(const QRect& vportRect, const QMatrix4x4& view)
{
    // fixed bottom-right map scale bar (independent of UI scale / DPI)
    const float sideMargin   = 16.0f;
    const float bottomMargin = 16.0f;
    const float tickH        = 6.0f;
    const float labelGap     = 3.0f;
    const float maxBarPx     = 120.0f;

    const float centerX = vportRect.x() + vportRect.width() - sideMargin - maxBarPx * 0.5f; // shifted center
    const float barY    = vportRect.y() + bottomMargin; // line; ticks point up

    // ground (Z=0) meters per horizontal screen pixel at the bar — valid under tilt
    const QMatrix4x4 mv = view * m_model;
    const float probePx = 100.0f;

    auto groundAt = [&](float sx, float sy, QVector3D& out) -> bool {
        const QVector3D nearP = QVector3D(sx, sy, 0.0f).unproject(mv, m_projection, vportRect);
        const QVector3D farP  = QVector3D(sx, sy, 1.0f).unproject(mv, m_projection, vportRect);
        const QVector3D d = farP - nearP;
        if (std::fabs(d.z()) < 1e-9f) {
            return false;
        }
        const float t = -nearP.z() / d.z();
        if (t < 0.0f) {
            return false;
        }
        out = nearP + d * t;
        return true;
    };

    QVector3D w0, w1;
    if (!groundAt(centerX, barY, w0) || !groundAt(centerX + probePx, barY, w1)) {
        return;
    }
    const float metersPerPixel = (w1 - w0).length() / probePx;
    if (!(metersPerPixel > 1e-9f)) {
        return;
    }
    const float pxPerMeter = 1.0f / metersPerPixel;

    static const float kSteps[] = { 0.1f, 0.25f, 0.5f, 1.f, 5.f, 10.f, 25.f, 50.f, 100.f, 200.f, 500.f,
                                    1000.f, 2000.f, 5000.f, 10000.f, 20000.f, 50000.f,
                                    100000.f, 200000.f, 500000.f, 1000000.f };
    const int stepCount = static_cast<int>(sizeof(kSteps) / sizeof(kSteps[0]));

    const float maxMeters = maxBarPx * metersPerPixel;

    float value = kSteps[0];
    for (int i = 0; i < stepCount; ++i) {
        if (kSteps[i] <= maxMeters) {
            value = kSteps[i];
        } else {
            break;
        }
    }

    const float barPx  = value * pxPerMeter;
    const float leftX  = centerX - barPx * 0.5f;
    const float rightX = centerX + barPx * 0.5f;

    if (auto sp = m_shaderProgramMap.value("static_sec", nullptr); sp && sp->bind()) {
        glDisable(GL_DEPTH_TEST);
        const float halfW = vportRect.width() / 2.0f;
        const float halfH = vportRect.height() / 2.0f;
        auto ndc = [&](float px, float py) -> QVector2D {
            return QVector2D(px / halfW - 1.0f, py / halfH - 1.0f);
        };
        QVector<QVector2D> lines = {
            ndc(leftX,  barY),  ndc(rightX, barY),
            ndc(leftX,  barY),  ndc(leftX,  barY + tickH),
            ndc(rightX, barY),  ndc(rightX, barY + tickH)
        };
        const int colorLoc = sp->uniformLocation("color");
        sp->setUniformValue(colorLoc, DrawUtils::colorToVector4d(QColor(255, 255, 255)));
        sp->enableAttributeArray(0);
        sp->setAttributeArray(0, lines.constData());
        glLineWidth(2.0f);
        glDrawArrays(GL_LINES, 0, lines.size());
        glLineWidth(1.0f);
        sp->disableAttributeArray(0);
        sp->release();
    }

    QString label;
    if (value < 1.0f) {
        label = QString::number(qRound(value * 100.0f)) + " " + QCoreApplication::translate("ScaleBar", "cm");
    } else if (value < 1000.0f) {
        label = QString::number(static_cast<int>(value)) + " " + QCoreApplication::translate("ScaleBar", "m");
    } else {
        label = QString::number(value / 1000.0) + " " + QCoreApplication::translate("ScaleBar", "km");
    }

    const float charH = static_cast<float>(TextRenderer::instance().getCharPixelHeight());
    const float approxW = label.size() * charH * 0.55f;
    const float barTopLeftY = vportRect.height() - barY;
    const float labelX = centerX - approxW * 0.5f;
    const float labelY = barTopLeftY - tickH - labelGap; // baseline above the up-ticks

    QMatrix4x4 textProjection;
    textProjection.ortho(vportRect);
    TextRenderer::instance().setColor(QColor(255, 255, 255));
    TextRenderer::instance().render(label, 1.0f, QVector2D(labelX, labelY), false, this, textProjection, m_shaderProgramMap);
}
