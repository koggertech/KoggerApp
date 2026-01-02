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

    inline bool nearlyEqual(const QVector3D& a, const QVector3D& b, float eps = 1e-5f)
    {
        return (std::abs(a.x() - b.x()) <= eps) &&
               (std::abs(a.y() - b.y()) <= eps) &&
               (std::abs(a.z() - b.z()) <= eps);
    }

    inline float cross2d(const QVector3D& a, const QVector3D& b, const QVector3D& c)
    {
        const float abx = b.x() - a.x();
        const float aby = b.y() - a.y();
        const float acx = c.x() - a.x();
        const float acy = c.y() - a.y();
        return abx * acy - aby * acx;
    }

    inline float signedArea2d(const QVector<QVector3D>& poly)
    {
        if (poly.size() < 3) {
            return 0.0f;
        }
        double area = 0.0;
        for (int i = 0; i < poly.size(); ++i) {
            const auto& p = poly[i];
            const auto& n = poly[(i + 1) % poly.size()];
            area += static_cast<double>(p.x()) * n.y() - static_cast<double>(n.x()) * p.y();
        }
        return static_cast<float>(area * 0.5);
    }

    inline bool pointInTriangle2d(const QVector3D& p,
                                  const QVector3D& a,
                                  const QVector3D& b,
                                  const QVector3D& c,
                                  bool ccw)
    {
        constexpr float eps = 1e-6f;
        const float c1 = cross2d(a, b, p);
        const float c2 = cross2d(b, c, p);
        const float c3 = cross2d(c, a, p);
        if (ccw) {
            return (c1 >= -eps) && (c2 >= -eps) && (c3 >= -eps);
        }
        return (c1 <= eps) && (c2 <= eps) && (c3 <= eps);
    }

    inline QVector<QVector3D> triangulatePolygonXY(const QVector<QVector3D>& input)
    {
        QVector<QVector3D> poly = input;
        if (poly.size() >= 2 && nearlyEqual(poly.first(), poly.last())) {
            poly.removeLast();
        }
        if (poly.size() < 3) {
            return {};
        }

        const bool ccw = signedArea2d(poly) > 0.0f;
        QVector<int> indices;
        indices.reserve(poly.size());
        for (int i = 0; i < poly.size(); ++i) {
            indices.push_back(i);
        }

        QVector<QVector3D> triangles;
        triangles.reserve((poly.size() - 2) * 3);

        const int maxIters = poly.size() * poly.size();
        int guard = 0;
        while (indices.size() > 2 && guard++ < maxIters) {
            bool earFound = false;
            for (int i = 0; i < indices.size(); ++i) {
                const int prev = indices[(i - 1 + indices.size()) % indices.size()];
                const int curr = indices[i];
                const int next = indices[(i + 1) % indices.size()];

                const float cr = cross2d(poly[prev], poly[curr], poly[next]);
                if (ccw ? (cr <= 1e-7f) : (cr >= -1e-7f)) {
                    continue;
                }

                bool contains = false;
                for (int j = 0; j < indices.size(); ++j) {
                    const int idx = indices[j];
                    if (idx == prev || idx == curr || idx == next) {
                        continue;
                    }
                    if (pointInTriangle2d(poly[idx], poly[prev], poly[curr], poly[next], ccw)) {
                        contains = true;
                        break;
                    }
                }
                if (contains) {
                    continue;
                }

                triangles.push_back(poly[prev]);
                triangles.push_back(poly[curr]);
                triangles.push_back(poly[next]);
                indices.removeAt(i);
                earFound = true;
                break;
            }
            if (!earFound) {
                break;
            }
        }

        if (triangles.isEmpty()) {
            for (int i = 1; i < poly.size() - 1; ++i) {
                triangles.push_back(poly[0]);
                triangles.push_back(poly[i]);
                triangles.push_back(poly[i + 1]);
            }
        }

        return triangles;
    }
}
