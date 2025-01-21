#include "textrenderer.h"
#include <draw_utils.h>
#include <QDebug>
#include <QFile>

#include <ft2build.h>
#include FT_FREETYPE_H

namespace {
static const QString vertexShader = R"shader(
                                            #version 330 core
                                            #ifdef GL_ES
                                            precision mediump int;
                                            precision mediump float;
                                            #endif

                                            uniform mat4 mvp_matrix;

                                            attribute vec4 a_position;
                                            attribute vec2 a_texcoord;

                                            varying vec2 v_texcoord;

                                            void main()
                                            {
                                                gl_Position = mvp_matrix * a_position;
                                                v_texcoord = a_texcoord;
                                            }

                                      )shader";

/*
    //Default blending shader implementation
    static const QString fragmentShader = R"shader(
                                            #version 330 core
                                            #ifdef GL_ES
                                            precision mediump int;
                                            precision mediump float;
                                            #endif

                                            uniform sampler2D tex;
                                            varying vec2 v_texcoord;
                                            out vec4 color;

                                            void main()
                                            {
                                                vec4 sampled = vec4(1.0,1.0,1.0, texture(tex, v_texcoord).r);
                                                color = vec4(vec3(0.1,0.1,1.0),1.0) * sampled;
                                            };

                                        )shader";
    */
//Signed distance fields shader implementation
static const QString fragmentShader = R"shader(
                                            #version 330 core
                                            #ifdef GL_ES
                                            precision mediump int;
                                            precision mediump float;
                                            #endif

                                            uniform vec4 textColor;
                                            uniform sampler2D tex;
                                            varying vec2 v_texcoord;
                                            out vec4 color;

                                            float width = 0.51;
                                            float edge = 0.02;

                                            void main()
                                            {
                                                float distance = 1.0 - texture(tex,v_texcoord).r;
                                                float alpha = 1.0 - smoothstep(width, width + edge, distance);
                                                color = vec4(vec3(textColor.r,textColor.g,textColor.b), alpha);
                                            };

                                        )shader";

}

TextRenderer& TextRenderer::instance()
{
    static TextRenderer instance;
    return instance;
}

void TextRenderer::setFontPixelSize(int size)
{
    if(m_fontPixelSize != size){
        m_fontPixelSize = size;
        initFont();
    }
}

void TextRenderer::setColor(const QColor &color)
{
    if(m_color != color)
        m_color = color;
}

void TextRenderer::setBackgroundColor(const QColor &color)
{
    if (m_backgroundColor != color)
        m_backgroundColor = color;
}

void TextRenderer::render(const QString &text, float scale, QVector2D pos, QOpenGLFunctions *ctx, const QMatrix4x4 &projection)
{
    if (!m_shaderProgram->bind()){
        qCritical() << "Error binding text shader program.";
        return;
    }

    if(!m_arrayBuffer.bind()){
        qCritical() << "Error binding vertex array buffer!";
        return;
    }

    ctx->glEnable(GL_BLEND);
    ctx->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    m_shaderProgram->setUniformValue("mvp_matrix", projection);

    auto it = text.begin();
    while(it != text.end()){
        auto c = it->toLatin1();

        if(!m_chars.contains(c))
            continue;

        auto ch = m_chars[c];

        float pen_x = pos.x() + ch.bearing.x() * scale;
        float pen_y = pos.y() - (ch.size.y()/* - ch.bearing.y()*/) * scale;

        const float w = ch.size.x() * scale;
        const float h = ch.size.y() * scale;

        float vertices[6][4] = {
            { pen_x,     pen_y,     0.0, 0.0 },
            { pen_x,     pen_y + h, 0.0, 1.0 },
            { pen_x + w, pen_y + h, 1.0, 1.0 },

            { pen_x,     pen_y ,    0.0, 0.0 },
            { pen_x + w, pen_y + h, 1.0, 1.0 },
            { pen_x + w, pen_y,     1.0, 0.0 }
        };

        ch.texture->bind();
        m_shaderProgram->setUniformValue("texture", m_chars[c].texture->textureId());
        m_shaderProgram->setUniformValue("textColor", DrawUtils::colorToVector4d(m_color));

        m_arrayBuffer.write(0,vertices, 6 * 4 * sizeof(float));

        int vertexLocation = m_shaderProgram->attributeLocation("a_position");
        m_shaderProgram->enableAttributeArray(vertexLocation);
        m_shaderProgram->setAttributeBuffer(vertexLocation, GL_FLOAT, 0, 2, stride2d);

        int texcoordLocation = m_shaderProgram->attributeLocation("a_texcoord");
        m_shaderProgram->enableAttributeArray(texcoordLocation);
        m_shaderProgram->setAttributeBuffer(texcoordLocation, GL_FLOAT, 2 * sizeof(float), 2, stride2d);

        ctx->glDrawArrays(GL_TRIANGLES, 0, 6);

        pos.setX(pos.x() + (ch.advance >> 6) * scale);

        ch.texture->release();

        it++;
    }

    ctx->glDisable(GL_BLEND);

    m_shaderProgram->release();
    m_arrayBuffer.release();
}

void TextRenderer::render3D(const QString &text, float scale, QVector3D pos, const QVector3D &dir, QOpenGLFunctions *ctx, const QMatrix4x4 &pvm)
{
    if (!m_shaderProgram->bind()){
        qCritical() << "Error binding text shader program.";
        return;
    }

    if(!m_arrayBuffer.bind()){
        qCritical() << "Error binding vertex array buffer!";
        return;
    }

    ctx->glEnable(GL_BLEND);
    ctx->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    m_shaderProgram->setUniformValue("mvp_matrix", pvm);

    auto it = text.begin();
    while(it != text.end()){
        auto c = it->toLatin1();

        if(!m_chars.contains(c))
            continue;

        auto ch = m_chars[c];


        float pen_x = pos.x() + ch.bearing.x() * scale;
        float pen_y = pos.y() - (ch.size.y() - ch.bearing.y()) * scale;
        float pen_z = pos.z();


        const float w = ch.size.x() * scale;
        const float h = ch.size.y() * scale;

        float vertices[6][5] = {
            { pen_x,     pen_y,     pen_z, 0.0, 0.0 },
            { pen_x,     pen_y + h, pen_z, 0.0, 1.0 },
            { pen_x + w, pen_y + h, pen_z, 1.0, 1.0 },

            { pen_x,     pen_y ,    pen_z, 0.0, 0.0 },
            { pen_x + w, pen_y + h, pen_z, 1.0, 1.0 },
            { pen_x + w, pen_y,     pen_z, 1.0, 0.0 }
        };

        ch.texture->bind();
        m_shaderProgram->setUniformValue("texture", m_chars[c].texture->textureId());
        m_shaderProgram->setUniformValue("textColor", DrawUtils::colorToVector4d(m_color));

        m_arrayBuffer.write(0,vertices, 6 * 5 * sizeof(float));

        int vertexLocation = m_shaderProgram->attributeLocation("a_position");
        m_shaderProgram->enableAttributeArray(vertexLocation);
        m_shaderProgram->setAttributeBuffer(vertexLocation, GL_FLOAT, 0, 3, stride3d);

        int texcoordLocation = m_shaderProgram->attributeLocation("a_texcoord");
        m_shaderProgram->enableAttributeArray(texcoordLocation);
        m_shaderProgram->setAttributeBuffer(texcoordLocation, GL_FLOAT, 3 * sizeof(float), 2, stride3d);

        ctx->glDrawArrays(GL_TRIANGLES, 0, 6);

        pos.setX(pos.x() + (ch.advance >> 6) * scale);

        ch.texture->release();

        it++;
    }

    ctx->glDisable(GL_BLEND);

    m_shaderProgram->release();
    m_arrayBuffer.release();
}

void TextRenderer::cleanup()
{
    if (QOpenGLContext::currentContext()) {
        for (auto& ch : m_chars) {
            if (ch.texture) {
                ch.texture->destroy();
            }
        }
    }
    m_chars.clear();

    m_arrayBuffer.destroy();
}

TextRenderer::TextRenderer()
    :m_shaderProgram(std::unique_ptr<QOpenGLShaderProgram>(new QOpenGLShaderProgram))
{
    initBuffers();
    initShaders();
    initFont();
}

TextRenderer::~TextRenderer()
{
    // cleanup
}

void TextRenderer::initShaders()
{
    bool success = m_shaderProgram->addCacheableShaderFromSourceCode(QOpenGLShader::Vertex, ::vertexShader);

    if (!success){
        qCritical() << "Error adding text vertex shader from source code.";
        return;
    }

    success = m_shaderProgram->addCacheableShaderFromSourceCode(QOpenGLShader::Fragment, ::fragmentShader);

    if (!success){
        qCritical() << "Error adding text fragment shader from source code.";
        return;
    }

    success = m_shaderProgram->link();

    if (!success){
        qCritical() << "Error linking text shaders from source code in shader program.";
        return;
    }
}

void TextRenderer::initBuffers()
{
    m_arrayBuffer.setUsagePattern(QOpenGLBuffer::DynamicDraw);
    m_arrayBuffer.create();
    m_arrayBuffer.bind();
    m_arrayBuffer.allocate(6 * 5 * sizeof(float));
}

void TextRenderer::initFont()
{
    m_chars.clear();

    FT_Library ft;
    FT_Face face;

    if (FT_Init_FreeType(&ft)){
        qDebug().noquote() << "ERROR::FREETYPE: Could not init FreeType Library";
        return;
    }

    QString resourcePath  = ":/assets/fonts/arial.ttf";

    QFile fontFile(resourcePath);
    if (!fontFile.open(QIODevice::ReadOnly)) {
        qDebug() << "ERROR::FREETYPE: Failed to open font file from resources:" << resourcePath;
        return;
    }

    QByteArray fontData = fontFile.readAll();
    fontFile.close();

    if (FT_New_Memory_Face(ft, reinterpret_cast<const FT_Byte*>(fontData.constData()), fontData.size(), 0, &face)) {
        qDebug().noquote() << "ERROR::FREETYPE: Failed to load font from memory";
        return;
    }

    FT_Set_Pixel_Sizes(face, 0, m_fontPixelSize);

    for (GLubyte c = 0; c < 128; c++)
    {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)){
            qDebug().noquote() << "ERROR::FREETYTPE: Failed to load Glyph";
            continue;
        }

        QImage image((uchar*)face->glyph->bitmap.buffer,
                     face->glyph->bitmap.width,
                     face->glyph->bitmap.rows,
                     face->glyph->bitmap.width * sizeof(uchar),
                     QImage::Format_Indexed8);

        Character character;

        character.num     = c;
        character.size    = QVector2D(face->glyph->bitmap.width, face->glyph->bitmap.rows);
        character.bearing = QVector2D(face->glyph->bitmap_left, face->glyph->bitmap_top);
        character.advance = face->glyph->advance.x;

        character.texture = std::make_shared<QOpenGLTexture>(image, QOpenGLTexture::GenerateMipMaps);
        character.texture->create();

        character.texture->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
        character.texture->setMagnificationFilter(QOpenGLTexture::Linear);
        character.texture->setWrapMode(QOpenGLTexture::MirroredRepeat);
        character.texture->allocateStorage();

        m_chars.insert(c, character);
    }

    //qDebug() << "ch ------> " << m_chars['c'].size;

    FT_Done_Face(face);
    FT_Done_FreeType(ft);
}

void TextRenderer::drawBackground(QVector2D pos, QVector2D size, QOpenGLFunctions *ctx)
{

}
