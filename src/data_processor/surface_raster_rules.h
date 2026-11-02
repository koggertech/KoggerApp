#pragma once

#include <algorithm>
#include <cmath>
#include <QVector>
#include <QVector2D>
#include <QVector3D>
#include "data_processor_defs.h"
#include "math_defs.h"


namespace SurfaceRasterRules {

template<typename T>
T twiceArea(T pu, T pv, T qu, T qv, T ru, T rv)
{
    return (qu - pu) * (rv - pv) - (qv - pv) * (ru - pu);
}

template<typename T>
bool isDegenerate(T twiceAreaValue)
{
    return std::abs(twiceAreaValue) <= T(0.00001);
}

inline bool withinEdgeLimit(const QVector3D& a, const QVector3D& b, const QVector3D& c, float edgeLimit)
{
    const float maxEdge = std::max({ (a - b).length(), (b - c).length(), (c - a).length() });
    return !(std::isfinite(maxEdge) && maxEdge > edgeLimit);
}

template<typename T>
bool interpolateInTriangle(T au, T av, T bu, T bv, T cu, T cv, T denom,
                           T za, T zb, T zc, T pu, T pv, T& z)
{
    const T w0 = twiceArea(bu, bv, cu, cv, pu, pv) / denom;
    const T w1 = twiceArea(cu, cv, au, av, pu, pv) / denom;
    const T w2 = T(1) - w0 - w1;
    const T eps = T(kmath::fltEps);
    if (w0 < -eps || w1 < -eps || w2 < -eps) {
        return false;
    }
    z = w0 * za + w1 * zb + w2 * zc;
    return true;
}

template<typename T>
int extrapolationRadiusPx(T radiusPx, int stepPix)
{
    return std::max(1, static_cast<int>(std::ceil(radiusPx + T(0.5) * T(stepPix))));
}

inline bool extrapolationMayWrite(bool front, HeightType mark)
{
    if (front) {
        return mark != HeightType::kTriangulation && mark != HeightType::kMosaic;
    }
    return mark == HeightType::kUndefined;
}

inline bool diskNodeWritable(qint64 dx, qint64 dy, qint64 r2, bool useDir, float ux, float uy, HeightType mark)
{
    if (dx * dx + dy * dy > r2) {
        return false;
    }
    if (dx == 0 && dy == 0) {
        return false;
    }
    const bool front = !useDir || (static_cast<float>(dx) * ux + static_cast<float>(dy) * uy >= 0.f);
    return extrapolationMayWrite(front, mark);
}

inline bool trackDirection(const QVector<QVector3D>& track, int idx, float invRes, QVector2D& udir)
{
    auto finitePlanar = [](const QVector3D& p) {
        return std::isfinite(p.x()) && std::isfinite(p.y());
    };

    const QVector3D& p = track[idx];
    QVector2D v0(0.f, 0.f);
    QVector2D v1(0.f, 0.f);

    if (idx > 0 && finitePlanar(track[idx - 1])) {
        const QVector3D& pr = track[idx - 1];
        v0 = QVector2D((p.x() - pr.x()) * invRes, (p.y() - pr.y()) * invRes);
        if (v0.lengthSquared() > kmath::fltEps) {
            v0.normalize();
        }
    }

    if (idx + 1 < track.size() && finitePlanar(track[idx + 1])) {
        const QVector3D& nx = track[idx + 1];
        v1 = QVector2D((nx.x() - p.x()) * invRes, (nx.y() - p.y()) * invRes);
        if (v1.lengthSquared() > kmath::fltEps) {
            v1.normalize();
        }
    }

    const bool has0 = v0.lengthSquared() > kmath::fltEps;
    const bool has1 = v1.lengthSquared() > kmath::fltEps;
    if (!has0 && !has1) {
        return false;
    }

    const QVector2D v = (has0 && has1) ? (v0 + v1) : (has1 ? v1 : v0);
    if (v.lengthSquared() <= kmath::fltEps) {
        return false;
    }

    udir = v.normalized();
    return true;
}

}
