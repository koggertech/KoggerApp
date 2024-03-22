#ifndef DRAWUTILS_H
#define DRAWUTILS_H

#include <QColor>
#include <QRectF>
#include <QVector4D>
#include <QOpenGLFunctions>

constexpr float rgbMaxValue = 255.0f;

namespace DrawUtils
{
    /**
     * @brief Converts color from QColor to QVector4D
     * @param[in] color Color object for convertion
     * @return Vector that contains normalized color
     */
    inline QVector4D colorToVector4d(const QColor &color)
    {
        return {static_cast <float>(color.red())   / rgbMaxValue,
                static_cast <float>(color.green()) / rgbMaxValue,
                static_cast <float>(color.blue())  / rgbMaxValue,
                static_cast <float>(color.alpha()) / rgbMaxValue};
    }

    inline QRectF viewportRect(QOpenGLFunctions* ctx)
    {
        GLint viewport[4];
        ctx->glGetIntegerv(GL_VIEWPORT, viewport);
        return QRectF(viewport[0], viewport[1], viewport[2], viewport[3]);
    }
}

#endif // DRAWUTILS_H
