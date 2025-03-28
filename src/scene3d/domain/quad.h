#ifndef QUAD_H
#define QUAD_H

#include <vector>

#include "point_3d.h"
#include "edge.h"
#include "triangle.h"

#ifdef QT_CORE_LIB
#include <QVector>
#include <QVector3D>
#endif

template <typename T>
class Quad
{

public:

    //! Constructor 1
    Quad()
        : mA(0.0, 0.0, 0.0)
        , mB(0.0, 0.0, 0.0)
        , mC(0.0, 0.0, 0.0)
        , mD(0.0, 0.0, 0.0)
    {}

    //! Constructor 1
    Quad(const Point3D <T>& A,
         const Point3D <T>& B,
         const Point3D <T>& C,
         const Point3D <T>& D)
        : mA(A)
        , mB(B)
        , mC(C)
        , mD(D)
    {}

#ifdef QT_CORE_LIB

    Quad(const QVector3D& A,
         const QVector3D& B,
         const QVector3D& C,
         const QVector3D& D)
        : mA(Point3D <T>::fromQVector3D(A))
        , mB(Point3D <T>::fromQVector3D(B))
        , mC(Point3D <T>::fromQVector3D(C))
        , mD(Point3D <T>::fromQVector3D(D))
    {}

#endif

    //! Returns first vertice of quad
    Point3D <T> A() const { return mA; }
    //! Returns second vertice of quad
    Point3D <T> B() const { return mB; }
    //! Returns third vertice of quad
    Point3D <T> C() const { return mC; }
    //! Returns fourth vertice of quad
    Point3D <T> D() const { return mD; }


    Point3D <T>& refA() { return mA; }

    Point3D <T>& refB() { return mB; }

    Point3D <T>& refC() { return mC; }

    Point3D <T>& refD() { return mD; }

    std::vector <Point3D <T>> vertices()
    {
        std::vector <Point3D <T>> vertices;

        vertices.push_back(mA);
        vertices.push_back(mB);
        vertices.push_back(mC);
        vertices.push_back(mD);

        return vertices;
    }

    std::vector <Edge <T>> edges() const
    {
        std::vector <Edge <T>> edges;

        edges.push_back({mA, mB});
        edges.push_back({mB, mC});
        edges.push_back({mC, mD});
        edges.push_back({mD, mA});

        return edges;
    }

#ifdef QT_CORE_LIB

    bool intersectsWithLine(const QVector3D& origin,
                            const QVector3D& direction,
                            QVector3D& intersectionPoint,
                            bool dirNormalized = false) const
    {
        auto dir = direction;

        if (!dirNormalized)
            dir.normalize();

        auto triangles = QVector <Triangle <T>>{
                                                   Triangle <T> (mA, mB, mC),
                                                   Triangle <T> (mC, mD, mA),
                                                   Triangle <T> (mD, mA, mB),
                                                   Triangle <T> (mB, mC, mD)
                                                };

        for (const auto& triangle : triangles){

            bool intersects = triangle.intersectsWithLine(origin,
                                                          direction,
                                                          intersectionPoint,
                                                          dirNormalized);
            if (intersects){
                return true;
            }
        }

        return false;
    }

    T distanceToPoint(const QVector3D& point) const{
        return point.distanceToPlane(mA.toQVector3D(),
                                     mB.toQVector3D(),
                                     mC.toQVector3D());
    }

#endif


private:

    Point3D <T> mA;
    Point3D <T> mB;
    Point3D <T> mC;
    Point3D <T> mD;

};

#endif // QUAD_H
