#include "textrenderer.h"
#include <draw_utils.h>
#include <QDebug>
#include <QFile>

#include <ft2build.h>
#include FT_FREETYPE_H


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

void TextRenderer::render(const QString &text, float scale, QVector2D pos, bool drawBackground,
                          QOpenGLFunctions *ctx, const QMatrix4x4 &projection, const QMap <QString, std::shared_ptr <QOpenGLShaderProgram>>& shaderProgramMap)
{
    const float padding = 5.0f;

    // text_back
    if (drawBackground) {
        auto backgroundShader = shaderProgramMap.value("text_back", nullptr);

        if (!backgroundShader) {
            qWarning() << "Shader program 'text_back' not found!";
            return;
        }

        if (!backgroundShader->bind()) {
            qCritical() << "Error binding background shader program.";
            return;
        }

        if (!m_arrayBuffer.bind()) {
            qCritical() << "Error binding vertex array buffer!";
            return;
        }

        QVector2D bgTopLeft = pos - QVector2D(padding, -padding);
        QVector2D bgBottomRight = pos;
        float maxHeight = 0.0f;

        for (auto it = text.begin(); it != text.end(); ++it) {
            uint16_t c = it->unicode();
            if (!m_chars.contains(c))
                continue;

            auto ch = m_chars.value(c);
            bgBottomRight.setX(bgBottomRight.x() + (ch.advance >> 6) * scale);
            maxHeight = qMax(maxHeight, ch.size.y() * scale);
        }
        bgBottomRight.setY(pos.y() - maxHeight);
        bgBottomRight += QVector2D(padding, -padding);

        float bgVertices[6][3] = {
            { bgTopLeft.x(), bgTopLeft.y(), 0.0f },
            { bgTopLeft.x(), bgBottomRight.y(), 0.0f },
            { bgBottomRight.x(), bgBottomRight.y(), 0.0f },

            { bgTopLeft.x(), bgTopLeft.y(), 0.0f },
            { bgBottomRight.x(), bgBottomRight.y(), 0.0f },
            { bgBottomRight.x(), bgTopLeft.y(), 0.0f }
        };

        backgroundShader->setUniformValue("mvp_matrix", projection);
        backgroundShader->setUniformValue("color", QVector4D(0.18f, 0.18f, 0.18f, 1.0f));

        m_arrayBuffer.write(0, bgVertices, 6 * 3 * sizeof(float));

        int vertexLocation = backgroundShader->attributeLocation("a_position");
        backgroundShader->enableAttributeArray(vertexLocation);
        backgroundShader->setAttributeBuffer(vertexLocation, GL_FLOAT, 0, 3);

        ctx->glDrawArrays(GL_TRIANGLES, 0, 6);

        backgroundShader->release();
        m_arrayBuffer.release();
    }

    // text
    {
        auto textShader = shaderProgramMap.value("text", nullptr);

        if (!textShader) {
            qWarning() << "Shader program 'text' not found!";
            return;
        }

        if (!textShader->bind()) {
            qCritical() << "Error binding text shader program.";
            return;
        }

        if (!m_arrayBuffer.bind()) {
            qCritical() << "Error binding vertex array buffer!";
            return;
        }

        ctx->glEnable(GL_BLEND);
        ctx->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        ctx->glActiveTexture(GL_TEXTURE0);

        textShader->setUniformValue("mvp_matrix", projection);

        auto it = text.begin();
        while (it != text.end()) {
            uint16_t c = it->unicode();
            if (!m_chars.contains(c)) {
                ++it;
                continue;
            }

            auto ch = m_chars.value(c);

            float pen_x = pos.x() + ch.bearing.x() * scale;
            float pen_y = pos.y() - (ch.bearing.y() * scale);

            const float w = ch.size.x() * scale;
            const float h = ch.size.y() * scale;

            float vertices[6][4] = {
                { pen_x,     pen_y,     0.0, 0.0 },
                { pen_x,     pen_y + h, 0.0, 1.0 },
                { pen_x + w, pen_y + h, 1.0, 1.0 },

                { pen_x,     pen_y,     0.0, 0.0 },
                { pen_x + w, pen_y + h, 1.0, 1.0 },
                { pen_x + w, pen_y,     1.0, 0.0 }
            };

            if (ch.texture) {
                ch.texture->bind();
                textShader->setUniformValue("texture", ch.texture->textureId());
                textShader->setUniformValue("textColor", DrawUtils::colorToVector4d(m_color));

                m_arrayBuffer.write(0, vertices, 6 * 4 * sizeof(float));

                int vertexLocation = textShader->attributeLocation("a_position");
                textShader->enableAttributeArray(vertexLocation);
                textShader->setAttributeBuffer(vertexLocation, GL_FLOAT, 0, 2, stride2d);

                int texcoordLocation = textShader->attributeLocation("a_texcoord");
                textShader->enableAttributeArray(texcoordLocation);
                textShader->setAttributeBuffer(texcoordLocation, GL_FLOAT, 2 * sizeof(float), 2, stride2d);

                ctx->glDrawArrays(GL_TRIANGLES, 0, 6);

                ch.texture->release();
            }

            pos.setX(pos.x() + (ch.advance >> 6) * scale);
            ++it;
        }

        ctx->glDisable(GL_BLEND);

        textShader->release();
        m_arrayBuffer.release();
    }
}

void TextRenderer::render3D(const QString &text, float scale, QVector3D pos, const QVector3D &dir, QOpenGLFunctions *ctx, const QMatrix4x4 &pvm, const QMap <QString, std::shared_ptr <QOpenGLShaderProgram>>& shaderProgramMap)
{
    Q_UNUSED(dir);

    auto shaderProgram = shaderProgramMap.value("text", nullptr);

    if (!shaderProgram) {
        qWarning() << "Shader program 'static' not found!";
        return;
    }

    if (!shaderProgram->bind()) {
        qCritical() << "Error binding text shader program.";
        return;
    }

    if(!m_arrayBuffer.bind()){
        qCritical() << "Error binding vertex array buffer!";
        return;
    }

    ctx->glEnable(GL_BLEND);
    ctx->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    shaderProgram->setUniformValue("mvp_matrix", pvm);

    auto it = text.begin();
    while(it != text.end()){
        uint16_t c = it->unicode();

        if(!m_chars.contains(c))
            continue;

        auto ch = m_chars.value(c);

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

        if (ch.texture) {
            ch.texture->bind();
            shaderProgram->setUniformValue("texture", m_chars[c].texture->textureId());
            shaderProgram->setUniformValue("textColor", DrawUtils::colorToVector4d(m_color));

            m_arrayBuffer.write(0,vertices, 6 * 5 * sizeof(float));

            int vertexLocation = shaderProgram->attributeLocation("a_position");
            shaderProgram->enableAttributeArray(vertexLocation);
            shaderProgram->setAttributeBuffer(vertexLocation, GL_FLOAT, 0, 3, stride3d);

            int texcoordLocation = shaderProgram->attributeLocation("a_texcoord");
            shaderProgram->enableAttributeArray(texcoordLocation);
            shaderProgram->setAttributeBuffer(texcoordLocation, GL_FLOAT, 3 * sizeof(float), 2, stride3d);

            ctx->glDrawArrays(GL_TRIANGLES, 0, 6);
            ch.texture->release();
        }

        pos.setX(pos.x() + (ch.advance >> 6) * scale);
        it++;
    }

    ctx->glDisable(GL_BLEND);

    shaderProgram->release();
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
{
    initBuffers();
    initFont();
}

TextRenderer::~TextRenderer()
{
    // cleanup
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

    QString resourcePath  = ":/assets/fonts/Roboto-VariableFont_wdth,wght.ttf";

    QFile fontFile(resourcePath);
    if (!fontFile.open(QIODevice::ReadOnly)) {
        qDebug() << "ERROR::FREETYPE: Failed to open font file from resources:" << resourcePath;
        return;
    }

    QByteArray fontData = fontFile.readAll();
    fontFile.close();

    if (FT_New_Memory_Face(ft,
                           reinterpret_cast<const FT_Byte*>(fontData.constData()),
                           fontData.size(),
                           0,
                           &face))
    {
        qDebug().noquote() << "ERROR::FREETYPE: Failed to load font from memory";
        return;
    }

    FT_Set_Pixel_Sizes(face, 0, m_fontPixelSize);


    auto loadGlyphRange = [&](uint16_t start, uint16_t end) {
        for (uint16_t c = start; c <= end; c++) {
            if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
                qDebug().noquote() << "ERROR::FREETYTPE: Failed to load Glyph for char code"
                                   << QString("0x%1").arg(c, 0, 16);
                continue;
            }

            QImage image((uchar*)face->glyph->bitmap.buffer,
                         face->glyph->bitmap.width,
                         face->glyph->bitmap.rows,
                         face->glyph->bitmap.pitch,
                         QImage::Format_Indexed8);

            Character character;
            character.num     = c;
            character.size    = QVector2D(face->glyph->bitmap.width, face->glyph->bitmap.rows);
            character.bearing = QVector2D(face->glyph->bitmap_left, face->glyph->bitmap_top);
            character.advance = face->glyph->advance.x;

            if (!image.isNull()) {
                auto texture = std::make_shared<QOpenGLTexture>(image, QOpenGLTexture::GenerateMipMaps);
                texture->create();
                texture->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
                texture->setMagnificationFilter(QOpenGLTexture::Linear);
                texture->setWrapMode(QOpenGLTexture::ClampToEdge);
                character.texture = texture;
            } else {
                // Для пробела и других невидимых символов оставляем texture = nullptr
                character.texture = nullptr;
            }

            m_chars.insert(c, character);
        }
    };

    // ASCII (English)
    loadGlyphRange(0x0020, 0x007F);
    // Polish
    loadGlyphRange(0x00A0, 0x017F);
    // Cyrillic (Russian)
    loadGlyphRange(0x0400, 0x04FF);

    FT_Done_Face(face);
    FT_Done_FreeType(ft);
}
