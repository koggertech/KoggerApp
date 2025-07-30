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

constexpr float rgbMaxValue = 255.0f;


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

    void setTheme(int id); // emun ThemeId
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


struct MatrixParams {
    MatrixParams() :
        originX(0.0f),
        originY(0.0f),
        width(-1),
        height(-1)
    { };

    float originX;
    float originY;
    int width;
    int height;

    bool isValid() const {
        if (width == -1 ||
            height == -1) {
            return false;
        }
        return true;
    }

    void print(QDebug stream) const {
        stream << "\n";
        stream << " _____________\n";
        stream << " |           |\n";
        stream << " |           |\n";
        stream << " |           |h =" << height << "\n";
        stream << " |           |\n";
        stream << " |___________|\n";
        stream << "        w =" << width << "\n";
        stream << " originX:" << originX << "\n";
        stream << " originY:" << originY << "\n";
    }
};

inline void concatenateMatrixParameters(MatrixParams &srcDst, const MatrixParams &src)
{
    if (!srcDst.isValid() && !src.isValid())
        return;

    if (!srcDst.isValid()) {
        srcDst = src;
        return;
    }

    if (!src.isValid()) {
        return;
    }

    int maxX = std::max(srcDst.originX + srcDst.width, src.originX + src.width);
    int maxY = std::max(srcDst.originY + srcDst.height, src.originY + src.height);

    srcDst.originX = std::min(srcDst.originX, src.originX);
    srcDst.originY = std::min(srcDst.originY, src.originY);

    srcDst.width = static_cast<int>(std::ceil(maxX - srcDst.originX));
    srcDst.height = static_cast<int>(std::ceil(maxY - srcDst.originY));
}


inline MatrixParams getMatrixParams(const QVector<QVector3D> &vertices)
{
    MatrixParams retVal;

    if (vertices.isEmpty()) {
        return retVal;
    }

    auto [minX, maxX] = std::minmax_element(vertices.begin(), vertices.end(), [](const QVector3D &a, const QVector3D &b) { return a.x() < b.x(); });
    auto [minY, maxY] = std::minmax_element(vertices.begin(), vertices.end(), [](const QVector3D &a, const QVector3D &b) { return a.y() < b.y(); });

    retVal.originX = minX->x();
    retVal.originY = minY->y();

    retVal.width  = static_cast<int>(std::ceil(maxX->x() - minX->x()));
    retVal.height = static_cast<int>(std::ceil(maxY->y() - minY->y()));

    return retVal;
}


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
