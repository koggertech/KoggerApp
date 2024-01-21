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

    /**
     * @brief Renders text on screen
     * @param text
     * @param pos
     * @param ctx
     * @param projection
     */
    void render2D(const QString& text,
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
                  QVector3D pos,
                  const QVector3D& dir,
                  QOpenGLFunctions* ctx,
                  const QMatrix4x4& pvm);

private:
    Q_DISABLE_COPY(TextRenderer)

    TextRenderer();
    virtual ~TextRenderer();

    void initShaders();
    void initBuffers();
    void initFont();

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
        }
    };

    QMap<char,Character> m_chars;
    std::unique_ptr <QOpenGLShaderProgram> m_shaderProgram;
    QOpenGLBuffer m_arrayBuffer;
    QOpenGLBuffer m_indexBuffer;

    static constexpr int stride3d = 5 * sizeof(float);
};

#endif // TEXTRENDERER_H
