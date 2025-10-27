#pragma once

#include <algorithm>
#include <cmath>
#include <limits>
#include <QVector>
#include <QVector3D>
#include <QDebug>
#include "delaunay_defs.h"


namespace kmath {

static constexpr float fltEps = std::numeric_limits<float>::epsilon();
static constexpr float dblEps = std::numeric_limits<double>::epsilon();

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


inline MatrixParams getMatrixParams(const QVector<QVector3D> &vertices,float tileSideMeters)
{
    MatrixParams retVal;

    if (vertices.isEmpty() || !(tileSideMeters > 0.0f)) {
        return retVal;
    }

    float minX = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float minY = std::numeric_limits<float>::max();
    float maxY = std::numeric_limits<float>::lowest();

    for (const auto& v : vertices) {
        if (!std::isfinite(v.x()) || !std::isfinite(v.y()) || !std::isfinite(v.z())) {
            continue;
        }

        minX = std::min(minX, v.x());
        maxX = std::max(maxX, v.x());
        minY = std::min(minY, v.y());
        maxY = std::max(maxY, v.y());
    }
    if (minX == std::numeric_limits<float>::max()) {
        return retVal;
    }

    const int tx0 = int(std::floor(minX / tileSideMeters));
    const int ty0 = int(std::floor(minY / tileSideMeters));
    const int tx1 = int(std::floor((maxX - fltEps) / tileSideMeters));
    const int ty1 = int(std::floor((maxY - fltEps) / tileSideMeters));

    const int nx = tx1 - tx0 + 1;
    const int ny = ty1 - ty0 + 1;

    retVal.originX = tx0 * tileSideMeters;
    retVal.originY = ty0 * tileSideMeters;

    retVal.width  = int(std::floor(nx * tileSideMeters - fltEps));
    retVal.height = int(std::floor(ny * tileSideMeters - fltEps));
    // qDebug() << retVal.originX << retVal.originY << retVal.width << retVal.height;

    return retVal;
}

inline double dist2(const QPointF &a, const QPointF &b)
{
    double dx = a.x() - b.x();
    double dy = a.y() - b.y();
    return dx * dx + dy * dy;
}

inline QVector3D fvec(const delaunay::Point &p)
{
    return QVector3D(float(p.x), float(p.y), float(p.z));
}

inline QVector3D nanVec()
{
    return QVector3D(std::numeric_limits<float>::quiet_NaN(),
                     std::numeric_limits<float>::quiet_NaN(),
                     std::numeric_limits<float>::quiet_NaN());
}

inline float twiceArea(const QVector3D& p, const QVector3D& q, const QVector3D& r) // удвоенная площадь с ориентацией
{
    return (q.x() - p.x()) * (r.y() - p.y()) - (q.y() - p.y()) * (r.x() - p.x());
}

} // namespace kmath
