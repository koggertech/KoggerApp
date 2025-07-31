#pragma once

#include <cmath>
#include <vector>
#include <stdint.h>
#include <QColor>
#include <QRgb>
#include <QRectF>
#include <QVector>
#include <QVector3D>
#include <QVector4D>
#include <QOpenGLFunctions>
#include <QDebug>


static constexpr float rgbMaxValue = 255.0f;

namespace mosaic {

class PlotColorTable // TODO: copy-paste from Plot2DEchogram
{
public:
    /*structures*/
    enum class ThemeId {
        kUndefined,
        kClassic,
        kSepia,
        kWRGBD,
        kWB,
        kBW
    };

    /*methods*/
    PlotColorTable();

    void setTheme(int id); // enum ThemeId
    void setLevels(float low, float high);
    void setLowLevel(float val);
    void setHighLevel(float val);
    int                     getTheme()      const;
    std::pair<float, float> getLevels()     const;
    float                   getLowLevel()   const;
    float                   getHighLevel()  const;
    QVector<QRgb>           getColorTable() const;
    std::vector<uint8_t>    getRgbaColors() const;

private:
    /*methods*/
    void update();
    void setColorScheme(const QVector<QColor>& colors, const QVector<int>& levels);

    /*data*/
    int themeId_;
    QVector<QRgb> colorTable_;
    QVector<QRgb> colorTableWithLevels_;
    float lowLevel_;
    float highLevel_;
    std::vector<uint8_t> rgbaColors_;
};

} // namespace mosaic

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
