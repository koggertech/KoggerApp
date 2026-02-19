#include "text_renderer.h"
#include "draw_utils.h"
#include <QDebug>
#include <QFile>
#include <QOpenGLContext>
#include <QtGlobal>
#include <cstring>
#include <utility>

#include <ft2build.h> // NOLINT
#include FT_FREETYPE_H


TextRenderer& TextRenderer::instance()
{
    static TextRenderer* instance = new TextRenderer();
    return *instance;
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

QColor TextRenderer::getColor() const
{
    return m_color;
}

void TextRenderer::setBackgroundColor(const QColor &color)
{
    if (m_backgroundColor != color)
        m_backgroundColor = color;
}

int TextRenderer::getCharPixelHeight() const
{
    if (m_chars.contains('0')) {
        return m_chars['0'].size.y();
    }
    return m_fontPixelSize;
}

void TextRenderer::render(const QString &text, float scale, QVector2D pos, bool drawBackground,
                          QOpenGLFunctions *ctx, const QMatrix4x4 &projection, const QMap <QString, std::shared_ptr <QOpenGLShaderProgram>>& shaderProgramMap)
{
    QVector<Text2DItem> items;
    items.reserve(1);
    items.append(Text2DItem{text, scale, pos, drawBackground});
    render2DBatch(items, ctx, projection, shaderProgramMap);
}

void TextRenderer::render2DBatch(const QVector<Text2DItem> &items, QOpenGLFunctions *ctx, const QMatrix4x4 &projection, const QMap<QString, std::shared_ptr<QOpenGLShaderProgram> > &shaderProgramMap)
{
    if (items.isEmpty()) {
        return;
    }

    const float padding = 5.0f;
    bool anyBackground = false;
    int totalChars = 0;
    for (const auto& item : items) {
        anyBackground = anyBackground || item.drawBackground;
        totalChars += item.text.size();
    }

    if (anyBackground) {
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
            backgroundShader->release();
            return;
        }

        QVector<float> bgVertices;
        bgVertices.reserve(items.size() * 6 * 3);
        auto appendBgVertex = [&bgVertices](float x, float y) {
            bgVertices.append(x);
            bgVertices.append(y);
            bgVertices.append(0.0f);
        };

        for (const auto& item : items) {
            if (!item.drawBackground) {
                continue;
            }

            QVector2D bgTopLeft = item.pos - QVector2D(padding, -padding);
            QVector2D bgBottomRight = item.pos;
            float maxHeight = 0.0f;

            for (auto it = item.text.begin(); it != item.text.end(); ++it) {
                const uint16_t c = it->unicode();
                auto chIt = m_chars.constFind(c);
                if (chIt == m_chars.cend()) {
                    continue;
                }
                const auto& ch = chIt.value();
                bgBottomRight.setX(bgBottomRight.x() + (ch.advance >> 6) * item.scale);
                maxHeight = qMax(maxHeight, ch.size.y() * item.scale);
            }

            bgBottomRight.setY(item.pos.y() - maxHeight);
            bgBottomRight += QVector2D(padding, -padding);

            appendBgVertex(bgTopLeft.x(), bgTopLeft.y());
            appendBgVertex(bgTopLeft.x(), bgBottomRight.y());
            appendBgVertex(bgBottomRight.x(), bgBottomRight.y());

            appendBgVertex(bgTopLeft.x(), bgTopLeft.y());
            appendBgVertex(bgBottomRight.x(), bgBottomRight.y());
            appendBgVertex(bgBottomRight.x(), bgTopLeft.y());
        }

        if (!bgVertices.isEmpty()) {
            const int bytes = bgVertices.size() * int(sizeof(float));
            if (m_arrayBuffer.size() < bytes) {
                m_arrayBuffer.allocate(bgVertices.constData(), bytes);
            } else {
                m_arrayBuffer.write(0, bgVertices.constData(), bytes);
            }

            backgroundShader->setUniformValue("mvp_matrix", projection);
            backgroundShader->setUniformValue("color", QVector4D(0.18f, 0.18f, 0.18f, 1.0f));

            const int vertexLocation = backgroundShader->attributeLocation("a_position");
            backgroundShader->enableAttributeArray(vertexLocation);
            backgroundShader->setAttributeBuffer(vertexLocation, GL_FLOAT, 0, 3);
            ctx->glDrawArrays(GL_TRIANGLES, 0, bgVertices.size() / 3);
            backgroundShader->disableAttributeArray(vertexLocation);
        }

        backgroundShader->release();
        m_arrayBuffer.release();
    }

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
        textShader->release();
        return;
    }
    if (!m_fontAtlasTexture) {
        textShader->release();
        m_arrayBuffer.release();
        return;
    }

    ctx->glEnable(GL_BLEND);
    ctx->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    ctx->glActiveTexture(GL_TEXTURE0);

    textShader->setUniformValue("mvp_matrix", projection);
    textShader->setUniformValue("tex", 0);
    textShader->setUniformValue("textColor", DrawUtils::colorToVector4d(m_color));

    const int vertexLocation = textShader->attributeLocation("a_position");
    const int texcoordLocation = textShader->attributeLocation("a_texcoord");

    QVector<float> vertices;
    vertices.reserve(totalChars * 6 * 4);

    auto appendVertex = [&vertices](float x, float y, float u, float v) {
        vertices.append(x);
        vertices.append(y);
        vertices.append(u);
        vertices.append(v);
    };

    for (const auto& item : items) {
        float cursorX = item.pos.x();
        const float cursorY = item.pos.y();

        for (auto it = item.text.begin(); it != item.text.end(); ++it) {
            const uint16_t c = it->unicode();
            auto chIt = m_chars.constFind(c);
            if (chIt == m_chars.cend()) {
                continue;
            }

            const auto& ch = chIt.value();
            const float advance = (ch.advance >> 6) * item.scale;

            if (ch.hasBitmap) {
                const float penX = cursorX + ch.bearing.x() * item.scale;
                const float penY = cursorY - ch.bearing.y() * item.scale;
                const float w = ch.size.x() * item.scale;
                const float h = ch.size.y() * item.scale;

                const float u0 = ch.uvMin.x();
                const float v0 = ch.uvMin.y();
                const float u1 = ch.uvMax.x();
                const float v1 = ch.uvMax.y();

                appendVertex(penX,     penY,     u0, v0);
                appendVertex(penX,     penY + h, u0, v1);
                appendVertex(penX + w, penY + h, u1, v1);

                appendVertex(penX,     penY,     u0, v0);
                appendVertex(penX + w, penY + h, u1, v1);
                appendVertex(penX + w, penY,     u1, v0);
            }

            cursorX += advance;
        }
    }

    if (!vertices.isEmpty()) {
        const int bytes = vertices.size() * int(sizeof(float));
        if (m_arrayBuffer.size() < bytes) {
            m_arrayBuffer.allocate(vertices.constData(), bytes);
        } else {
            m_arrayBuffer.write(0, vertices.constData(), bytes);
        }

        textShader->enableAttributeArray(vertexLocation);
        textShader->setAttributeBuffer(vertexLocation, GL_FLOAT, 0, 2, stride2d);
        textShader->enableAttributeArray(texcoordLocation);
        textShader->setAttributeBuffer(texcoordLocation, GL_FLOAT, 2 * sizeof(float), 2, stride2d);

        m_fontAtlasTexture->bind(0);
        ctx->glDrawArrays(GL_TRIANGLES, 0, vertices.size() / 4);
        m_fontAtlasTexture->release();

        textShader->disableAttributeArray(texcoordLocation);
        textShader->disableAttributeArray(vertexLocation);
    }

    ctx->glDisable(GL_BLEND);
    textShader->release();
    m_arrayBuffer.release();
}

void TextRenderer::render3D(const QString &text, float scale, QVector3D pos, const QVector3D &dir, QOpenGLFunctions *ctx, const QMatrix4x4 &pvm, const QMap <QString, std::shared_ptr <QOpenGLShaderProgram>>& shaderProgramMap)
{
    QVector<Text3DItem> items;
    items.reserve(1);
    items.append(Text3DItem{QStringView{text}, scale, pos, dir});
    render3DBatch(items, ctx, pvm, shaderProgramMap);
}

void TextRenderer::render3DBatch(const QVector<Text3DItem> &items, QOpenGLFunctions *ctx, const QMatrix4x4 &pvm, const QMap<QString, std::shared_ptr<QOpenGLShaderProgram>> &shaderProgramMap)
{
    if (items.isEmpty()) {
        return;
    }

    auto shaderProgram = shaderProgramMap.value("text", nullptr);

    if (!shaderProgram) {
        qWarning() << "Shader program 'static' not found!";
        return;
    }

    if (!shaderProgram->bind()) {
        qCritical() << "Error binding text shader program.";
        return;
    }

    if (!m_arrayBuffer.bind()) {
        qCritical() << "Error binding vertex array buffer!";
        return;
    }
    if (!m_fontAtlasTexture) {
        shaderProgram->release();
        m_arrayBuffer.release();
        return;
    }

    ctx->glEnable(GL_BLEND);
    ctx->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    ctx->glActiveTexture(GL_TEXTURE0);

    shaderProgram->setUniformValue("mvp_matrix", pvm);
    shaderProgram->setUniformValue("tex", 0);
    shaderProgram->setUniformValue("textColor", DrawUtils::colorToVector4d(m_color));

    const int vertexLocation = shaderProgram->attributeLocation("a_position");
    const int texcoordLocation = shaderProgram->attributeLocation("a_texcoord");

    int totalChars = 0;
    for (const auto& item : items) {
        totalChars += item.text.size();
    }

    QVector<float> vertices;
    vertices.reserve(totalChars * 6 * 5);

    auto appendVertex = [&vertices](float x, float y, float z, float u, float v) {
        vertices.append(x);
        vertices.append(y);
        vertices.append(z);
        vertices.append(u);
        vertices.append(v);
    };

    auto appendRotatedVertex = [&](float localX, float localY, float cs, float sn, float worldX, float worldY, float worldZ, float u, float v) {
        const float rotatedX = localX * cs - localY * sn;
        const float rotatedY = localX * sn + localY * cs;
        appendVertex(rotatedX + worldX, rotatedY + worldY, worldZ, u, v);
    };

    for (const auto& item : items) {
        const QVector3D normalized = item.dir.normalized();
        const float cs = normalized.x();
        const float sn = normalized.y();
        float cursorX = item.pos.x();
        float cursorY = item.pos.y();
        const float scale = item.scale;

        for (auto it = item.text.begin(); it != item.text.end(); ++it) {
            const uint16_t c = it->unicode();
            auto chIt = m_chars.constFind(c);
            if (chIt == m_chars.cend()) {
                continue;
            }

            const auto& ch = chIt.value();
            const float adv = (ch.advance >> 6) * scale;

            if (ch.hasBitmap) {
                const float penX = ch.bearing.x() * scale;
                const float penY = -ch.bearing.y() * scale;
                const float w = ch.size.x() * scale;
                const float h = ch.size.y() * scale;

                const float u0 = ch.uvMin.x();
                const float v0 = ch.uvMin.y();
                const float u1 = ch.uvMax.x();
                const float v1 = ch.uvMax.y();

                appendRotatedVertex(penX,     penY,     cs, sn, cursorX, cursorY, item.pos.z(), u0, v0);
                appendRotatedVertex(penX,     penY + h, cs, sn, cursorX, cursorY, item.pos.z(), u0, v1);
                appendRotatedVertex(penX + w, penY + h, cs, sn, cursorX, cursorY, item.pos.z(), u1, v1);

                appendRotatedVertex(penX,     penY,     cs, sn, cursorX, cursorY, item.pos.z(), u0, v0);
                appendRotatedVertex(penX + w, penY + h, cs, sn, cursorX, cursorY, item.pos.z(), u1, v1);
                appendRotatedVertex(penX + w, penY,     cs, sn, cursorX, cursorY, item.pos.z(), u1, v0);
            }

            cursorX += adv * cs;
            cursorY += adv * sn;
        }
    }

    if (!vertices.isEmpty()) {
        const int bytes = vertices.size() * int(sizeof(float));
        if (m_arrayBuffer.size() < bytes) {
            m_arrayBuffer.allocate(vertices.constData(), bytes);
        } else {
            m_arrayBuffer.write(0, vertices.constData(), bytes);
        }

        shaderProgram->enableAttributeArray(vertexLocation);
        shaderProgram->setAttributeBuffer(vertexLocation, GL_FLOAT, 0, 3, stride3d);

        shaderProgram->enableAttributeArray(texcoordLocation);
        shaderProgram->setAttributeBuffer(texcoordLocation, GL_FLOAT, 3 * sizeof(float), 2, stride3d);

        m_fontAtlasTexture->bind(0);
        ctx->glDrawArrays(GL_TRIANGLES, 0, vertices.size() / 5);
        m_fontAtlasTexture->release();

        shaderProgram->disableAttributeArray(texcoordLocation);
        shaderProgram->disableAttributeArray(vertexLocation);
    }

    ctx->glDisable(GL_BLEND);

    shaderProgram->release();
    m_arrayBuffer.release();
}

void TextRenderer::cleanup()
{
    auto* glContext = QOpenGLContext::currentContext();
    if (!glContext) {
        return;
    }

    if (m_fontAtlasTexture) {
        m_fontAtlasTexture->destroy();
        m_fontAtlasTexture.reset();
    }

    m_chars.clear();

    if (m_arrayBuffer.isCreated()) {
        m_arrayBuffer.destroy();
    }
    if (m_indexBuffer.isCreated()) {
        m_indexBuffer.destroy();
    }
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
    if (m_fontAtlasTexture) {
        if (QOpenGLContext::currentContext()) {
            m_fontAtlasTexture->destroy();
        }
        m_fontAtlasTexture.reset();
    }
    m_chars.clear();

    FT_Library ft = nullptr;
    FT_Face face = nullptr;

    if (FT_Init_FreeType(&ft)) {
        qDebug().noquote() << "ERROR::FREETYPE: Could not init FreeType Library";
        return;
    }

    const QString resourcePath = ":/fonts/Roboto-VariableFont_wdth,wght.ttf";

    QFile fontFile(resourcePath);
    if (!fontFile.open(QIODevice::ReadOnly)) {
        qDebug() << "ERROR::FREETYPE: Failed to open font file from resources:" << resourcePath;
        FT_Done_FreeType(ft);
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
        FT_Done_FreeType(ft);
        return;
    }

    const int rasterPixelSize = qMax(1, m_fontPixelSize * m_glyphRasterScale);
    FT_Set_Pixel_Sizes(face, 0, rasterPixelSize);
    const float invRasterScale = 1.0f / static_cast<float>(m_glyphRasterScale);

    struct LoadedGlyph
    {
        Character character;
        QImage bitmap;
        int atlasX = 0;
        int atlasY = 0;
    };

    QVector<LoadedGlyph> loadedGlyphs;
    loadedGlyphs.reserve(0x007F - 0x0020 + 1 + 0x017F - 0x00A0 + 1 + 0x04FF - 0x0400 + 1);


    auto loadGlyphRange = [&](uint16_t start, uint16_t end) {
        for (uint16_t c = start; c <= end; ++c) {
            if (FT_Load_Char(face, c, FT_LOAD_DEFAULT | FT_LOAD_TARGET_LIGHT)) {
                qDebug().noquote() << "ERROR::FREETYTPE: Failed to load Glyph for char code"
                                   << QString("0x%1").arg(c, 0, 16);
                continue;
            }
            if (FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL)) {
                qDebug().noquote() << "ERROR::FREETYTPE: Failed to render Glyph for char code"
                                   << QString("0x%1").arg(c, 0, 16);
                continue;
            }

            LoadedGlyph loaded;
            loaded.character.num = c;
            loaded.character.size = QVector2D(face->glyph->bitmap.width, face->glyph->bitmap.rows) * invRasterScale;
            loaded.character.bearing = QVector2D(face->glyph->bitmap_left, face->glyph->bitmap_top) * invRasterScale;
            loaded.character.advance = static_cast<GLuint>(face->glyph->advance.x / m_glyphRasterScale);

            const FT_Bitmap& bmp = face->glyph->bitmap;
            if (bmp.width > 0 && bmp.rows > 0 && bmp.buffer) {
                QImage glyphImage((uchar*)bmp.buffer,
                                  bmp.width,
                                  bmp.rows,
                                  bmp.pitch,
                                  QImage::Format_Grayscale8);
                if (!glyphImage.isNull()) {
                    loaded.bitmap = glyphImage.copy();
                    loaded.character.hasBitmap = !loaded.bitmap.isNull();
                }
            }
            loadedGlyphs.append(std::move(loaded));
        }
    };

    // ASCII (English)
    loadGlyphRange(0x0020, 0x007F);
    // Polish
    loadGlyphRange(0x00A0, 0x017F);
    // Cyrillic (Russian)
    loadGlyphRange(0x0400, 0x04FF);

    const int atlasPadding = 1;
    const int atlasWidth = 1024;

    int penX = atlasPadding;
    int penY = atlasPadding;
    int rowHeight = 0;
    bool hasAnyBitmap = false;

    for (auto& loaded : loadedGlyphs) {
        if (!loaded.character.hasBitmap || loaded.bitmap.isNull()) {
            continue;
        }

        const int glyphW = loaded.bitmap.width();
        const int glyphH = loaded.bitmap.height();
        if (glyphW <= 0 || glyphH <= 0) {
            loaded.character.hasBitmap = false;
            continue;
        }

        hasAnyBitmap = true;

        if (penX + glyphW + atlasPadding > atlasWidth) {
            penX = atlasPadding;
            penY += rowHeight + atlasPadding;
            rowHeight = 0;
        }

        loaded.atlasX = penX;
        loaded.atlasY = penY;
        penX += glyphW + atlasPadding;
        rowHeight = qMax(rowHeight, glyphH);
    }

    if (hasAnyBitmap) {
        const int atlasHeight = qMax(1, penY + rowHeight + atlasPadding);
        QImage atlasImage(atlasWidth, atlasHeight, QImage::Format_Grayscale8);
        atlasImage.fill(0);

        const float invAtlasW = 1.0f / float(atlasWidth);
        const float invAtlasH = 1.0f / float(atlasHeight);

        for (auto& loaded : loadedGlyphs) {
            if (!loaded.character.hasBitmap || loaded.bitmap.isNull()) {
                continue;
            }

            const int glyphW = loaded.bitmap.width();
            const int glyphH = loaded.bitmap.height();
            for (int row = 0; row < glyphH; ++row) {
                auto* dst = atlasImage.scanLine(loaded.atlasY + row) + loaded.atlasX;
                const auto* src = loaded.bitmap.constScanLine(row);
                std::memcpy(dst, src, size_t(glyphW));
            }

            const float u0 = loaded.atlasX * invAtlasW;
            const float u1 = (loaded.atlasX + glyphW) * invAtlasW;
            const float v0 = loaded.atlasY * invAtlasH;
            const float v1 = (loaded.atlasY + glyphH) * invAtlasH;
            loaded.character.uvMin = QVector2D(u0, v0);
            loaded.character.uvMax = QVector2D(u1, v1);
        }

        m_fontAtlasTexture = std::make_shared<QOpenGLTexture>(atlasImage, QOpenGLTexture::DontGenerateMipMaps);
        if (!m_fontAtlasTexture->isCreated()) {
            m_fontAtlasTexture->create();
        }
        m_fontAtlasTexture->setMinificationFilter(QOpenGLTexture::Linear);
        m_fontAtlasTexture->setMagnificationFilter(QOpenGLTexture::Linear);
        m_fontAtlasTexture->setWrapMode(QOpenGLTexture::ClampToEdge);
    }

    for (const auto& loaded : loadedGlyphs) {
        m_chars.insert(loaded.character.num, loaded.character);
    }

    FT_Done_Face(face);
    FT_Done_FreeType(ft);
}
