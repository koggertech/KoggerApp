#ifndef TEXTRENDERER_H
#define TEXTRENDERER_H

#include <QString>

class QOpenGLFunctions;
class TextRenderer
{
public:
    TextRenderer() = delete;

    static void renderText(const QString& text, const QOpenGLFunctions& ctx);
};

#endif // TEXTRENDERER_H
