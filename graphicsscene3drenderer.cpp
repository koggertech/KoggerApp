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
    initFont();
    doTexture();

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

    // --------- text shader ------------//
    success = m_shaderProgramMap["text"]->addCacheableShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/text.vsh");

    if (!success) qCritical() << "Error adding text vertex shader from source file.";

    success = m_shaderProgramMap["text"]->addCacheableShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/text.fsh");

    if (!success) qCritical() << "Error adding text fragment shader from source file.";

    success = m_shaderProgramMap["text"]->link();

    if (!success) qCritical() << "Error linking text shaders in shader program.";

    // --------- texture shader ------------//
    success = m_shaderProgramMap["texture"]->addCacheableShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/texture.vsh");

    if (!success) qCritical() << "Error adding texture vertex shader from source file.";

    success = m_shaderProgramMap["texture"]->addCacheableShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/texture.fsh");

    if (!success) qCritical() << "Error adding texture fragment shader from source file.";

    success = m_shaderProgramMap["texture"]->link();

    if (!success) qCritical() << "Error linking texture shaders in shader program.";
}

void GraphicsScene3dRenderer::render()
{
    glDepthMask(true);

    glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    drawObjects();

    TextRenderer::instance();
}


void GraphicsScene3dRenderer::doTexture()
{
    QImage image("D:/container.jpg");
    m_texture = std::unique_ptr<QOpenGLTexture>(new QOpenGLTexture(image));

    m_texture->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
    m_texture->setMagnificationFilter(QOpenGLTexture::Linear);
    m_texture->setWrapMode(QOpenGLTexture::Repeat);
    m_texture->generateMipMaps();

    m_geometryEngine = std::unique_ptr<GeometryEngine>(new GeometryEngine());
}

void GraphicsScene3dRenderer::displayTexture(const QMatrix4x4& model,
                                             const QMatrix4x4& view,
                                             const QMatrix4x4& projection)
{
    char s = 'Q';

    // Texture test
    Character character = m_characters[s];

    character.texture->bind();

    auto shaderProgram = m_shaderProgramMap["texture"];

    if (!shaderProgram->bind()){
        qCritical() << "Error binding texture shader program.";
        return;
    }

    shaderProgram->setUniformValue("mvp_matrix", projection * view * model);
    shaderProgram->setUniformValue("texture", 0);

    m_geometryEngine->drawCubeGeometry(shaderProgram.get());
}


void GraphicsScene3dRenderer::drawObjects()
{
    QMatrix4x4 model, view, projection;

    view = m_camera.m_view;
    projection.perspective(m_camera.fov(), m_viewSize.width()/m_viewSize.height(), 1.0f, 5000.0f);
    model.scale(1.0f, 1.0f, m_verticalScale);

    m_model = std::move(model);
    m_projection = std::move(projection);

    glEnable(GL_DEPTH_TEST);
    m_planeGridRenderImpl.render(this, m_projection * view * m_model, m_shaderProgramMap);
    m_bottomTrackRenderImpl.render(this, m_projection * view * m_model, m_shaderProgramMap);
    m_surfaceRenderImpl.render(this, m_projection * view * m_model, m_shaderProgramMap);
    m_pointGroupRenderImpl.render(this, m_projection * view * m_model, m_shaderProgramMap);
    m_polygonGroupRenderImpl.render(this, m_projection * view * m_model, m_shaderProgramMap);
    m_boatTrackRenderImpl.render(this, m_projection * view * m_model, m_shaderProgramMap);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    //Draw text
    const QString text = "Lolissimo";\
    const QVector3D dir(0.0f,1.0f,0.0f);
    const QMatrix4x4 pvm = m_projection*view*m_model;
    QVector3D pos(0.0f,0.0f,0.0f);

    TextRenderer::instance().render3D(text,pos,dir,this,pvm);

    //displayTexture(model, view, projection);
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);

    //-----------Draw axes-------------
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    glViewport(viewport[2]-100,0,100,100);
    glDisable(GL_DEPTH_TEST);

    QMatrix4x4 axesView;
    QMatrix4x4 axesProjection;
    QMatrix4x4 axesModel;

    axesView = m_axesThumbnailCamera.m_view;
    axesProjection.perspective(m_camera.fov(), 100/100, 1.0f, 5000.0f);

    m_coordAxesRenderImpl.render(this, axesProjection * axesView * axesModel, m_shaderProgramMap);

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



    //QVector3D textColor(255.0f, 0.0f, 0.0f);
    //QVector2D textPos(100.0f,100.0f);
    //GLfloat scale = 1.0f;
    //QString text = "Test text m";

    ////QMatrix4x4 textProjection;
    ////textProjection.ortho(viewport[0], viewport[1], viewport[2], viewport[3], 0.0f, 0.0f);

    //auto shaderProgram = m_shaderProgramMap["texture"];

    //if (!shaderProgram->bind()){
    //    qCritical() << "Error binding texture shader program.";
    //    return;
    //}

    //shaderProgram->setUniformValue("mvp_matrix", projection * view * model);
    //shaderProgram->setUniformValue("texture", 0);

    // Iterate through all characters
    //QString::const_iterator c;
    //for (c = text.begin(); c != text.end(); c++)
    //{
    //    Character ch = m_characters[(*c).toLatin1()];

    //    GLfloat xpos = textPos.x() + ch.bearing.x() * scale;
    //    GLfloat ypos = textPos.y() - (ch.size.y() - ch.bearing.y()) * scale;

    //    GLfloat w = ch.size.x() * scale;
    //    GLfloat h = ch.size.y() * scale;
    //    // Update VBO for each character
    //    GLfloat vertices[6][4] = {
    //        { xpos,     ypos + h,   0.0, 0.0 },
    //        { xpos,     ypos,       0.0, 1.0 },
    //        { xpos + w, ypos,       1.0, 1.0 },

    //        { xpos,     ypos + h,   0.0, 0.0 },
    //        { xpos + w, ypos,       1.0, 1.0 },
    //        { xpos + w, ypos + h,   1.0, 0.0 }
    //    };

    //    ch.texture->bind();

    //    m_geometryEngine->drawCubeGeometry(shaderProgram.get());

    //    // Now advance cursors for next glyph (note that advance is number of 1/64 pixels)

    //    textPos.setX(textPos.x() + (ch.advance >> 6) * scale); // Bitshift by 6 to get value in pixels (2^6 = 64 (divide amount of 1/64th pixels by 64 to get amount of pixels))
    //}
}

void GraphicsScene3dRenderer::initFont()
{
    FT_Library ft;
    FT_Face face;

    if (FT_Init_FreeType(&ft)){
        qDebug().noquote() << "ERROR::FREETYPE: Could not init FreeType Library";
        return;
    }

    //if (FT_New_Face(ft, ":/assets/fonts/arial.ttf", 0, &face)){
    if (FT_New_Face(ft, "D:/arial.ttf", 0, &face)){
        qDebug().noquote() << "ERROR::FREETYPE: Failed to load font";
        return;
    }

    FT_Set_Pixel_Sizes(face, 0, 24);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // Disable byte-alignment restriction

    for (GLubyte c = 0; c < 128; c++)
    {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)){
            qDebug().noquote() << "ERROR::FREETYTPE: Failed to load Glyph";
            continue;
        }

        GLuint texture;

        QImage image((uchar*)face->glyph->bitmap.buffer,
                     face->glyph->bitmap.width,
                     face->glyph->bitmap.rows,
                     face->glyph->bitmap.width * sizeof(uchar),
                     QImage::Format_Indexed8);

        Character character = {
           new QOpenGLTexture(image),
           texture,
           QVector2D(face->glyph->bitmap.width, face->glyph->bitmap.rows),
           QVector2D(face->glyph->bitmap_left, face->glyph->bitmap_top),
           face->glyph->advance.x
        };

        character.texture->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
        character.texture->setMagnificationFilter(QOpenGLTexture::Linear);
        character.texture->setWrapMode(QOpenGLTexture::MirroredRepeat);
        character.texture->generateMipMaps();

        m_characters.insert(c, character);
    }

    // Configure VAO/VBO for texture quads
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    FT_Done_Face(face);
    FT_Done_FreeType(ft);
}
