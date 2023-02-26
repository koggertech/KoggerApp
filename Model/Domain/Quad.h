#ifndef QUAD_H
#define QUAD_H

#include "Point3D.h"

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


private:

    Point3D <T> mA;
    Point3D <T> mB;
    Point3D <T> mC;
    Point3D <T> mD;

};

#endif // QUAD_H
