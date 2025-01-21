#ifndef TEXTRENDERER_H
#define TEXTRENDERER_H

#include <sceneobject.h>

#include <QOpenGLBuffer>
#include <QOpenGLTexture>
#include <QOpenGLShaderProgram>

#include <memory>

class QOpenGLFunctions;
class TextRenderer
{
public:
    static TextRenderer& instance();

    void setFontPixelSize(int size);
    void setColor(const QColor& color);
    void setBackgroundColor(const QColor& color);

    void render(const QString& text,
                float scale,
                QVector2D pos,
                QOpenGLFunctions* ctx,
                const QMatrix4x4& projection);
    /**
     * @brief Rebders text somewhere in the world
     * @param text - text
     * @param pos - text rect position
     * @param dir - text rect front direction
     * @param ctx - render context
     * @param pvm - project * view * model matrix
     */
    void render3D(const QString& text,
                  float scale,
                  QVector3D pos,
                  const QVector3D& dir,
                  QOpenGLFunctions* ctx,
                  const QMatrix4x4& pvm);

    void cleanup();


private:
    Q_DISABLE_COPY(TextRenderer)

    TextRenderer();
    virtual ~TextRenderer();


    void initShaders();
    void initBuffers();
    void initFont();

    void drawBackground(QVector2D pos, QVector2D size, QOpenGLFunctions* ctx);


private:
    struct Character
    {
        std::shared_ptr<QOpenGLTexture> texture;
        char num          = 0;
        GLuint    advance = 0; ///< horizontal offset of the next character
        QVector2D size;        ///< glyph size
        QVector2D bearing;     ///< bottom left corner position on font atlas

        friend QDebug operator<<(QDebug ds, const Character &ch)
        {
            ds << " --> Character " << ch.num                  << '\n'
               << "texId ="         << ch.texture->textureId() << '\n'
               << "advance ="       << ch.advance              << '\n'
               << "size ="          << ch.size                 << '\n'
               << "bearing ="       << ch.bearing              << '\n'
               << "<----------"                                << '\n';

            return ds;
        }
    };

    QMap<char,Character> m_chars;
    std::unique_ptr <QOpenGLShaderProgram> m_shaderProgram;
    QOpenGLBuffer m_arrayBuffer;
    QOpenGLBuffer m_indexBuffer;
    QColor m_color = {0,0,0};
    QColor m_backgroundColor = {255, 255, 255};
    int m_fontPixelSize = 64;

    static constexpr int stride3d = 5 * sizeof(float);
    static constexpr int stride2d = 4 * sizeof(float);
};

#endif // TEXTRENDERER_H
