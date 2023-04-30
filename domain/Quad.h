#ifndef QUAD_H
#define QUAD_H

#include <vector>

#include <Point3D.h>
#include <Edge.h>

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


private:

    Point3D <T> mA;
    Point3D <T> mB;
    Point3D <T> mC;
    Point3D <T> mD;

};

#endif // QUAD_H
