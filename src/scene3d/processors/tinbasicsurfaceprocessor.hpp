//###################################################################
//! Kogger
//!
//! @author Харламенко И.В.
//! @date 2023
//###################################################################

#pragma once

#include <QSet>
#include <QDebug>

#include <abstractsurfaceprocessor.hpp>
#include <surface.h>
#include <DelaunayTriangulation.h>


class TinBasicSurfaceProcessor : public AbstractSurfaceProcessor
{
public:

    //! @brief Выполняет базовый расчет поверхности.
    //! @note Является переопределенным методом родительского класса
    VertexObject process(const QVector <QVector3D>& points) override;

    void setParam(Param p, QVariant value) override;

private:

    double mTriangleEdgeLengthLimit = -1.0f;



    //! Хешер для типа данных точки (необходим для устранения повторяющихся
    //! точек с помощью QSet при триангуляции)
    template <typename T>
    inline uint qHash(const Point3D <T> &p, uint seed = 0)
    {
        uint h1 = qHash(p.x());
        uint h2 = qHash(p.y());
        uint h3 = qHash(p.z());
        return h1 ^ (h2 << 1) ^ (h3 << 2);
    }


};

