#pragma once

#include "Point2D.h"
#include "Edge.h"
#include "Circle.h"

// TRIANGLE CLASS
template <typename T>
class Triangle
{
public:

    //! Constructor 1
    Triangle()
        : mA(0.0, 0.0, 0.0)
        , mB(0.0, 0.0, 0.0)
        , mC(0.0, 0.0, 0.0)
        , mAB(0.0, 0.0, 0.0)
        , mBC(0.0, 0.0, 0.0)
        , mAC(0.0, 0.0, 0.0)
        , mWrong(false) {

    };

    //! Constructor 2
    Triangle(
          const Point3D <T>& A
        , const Point3D <T>& B
        , const Point3D <T>& C)
        : mA(A)
        , mB(B)
        , mC(C)
        , mAB(A, B)
        , mBC(B, C)
        , mAC(A, C)
        , mWrong(false) {

        createCircumCircle();
    }

    //! Returns first vertice of triangle
    Point3D <T> A() const { return mA; }
    //! Returns second vertice of triangle
    Point3D <T> B() const { return mB; }
    //! Returns third vertice of triangle
    Point3D <T> C() const { return mC; }
    //! Returns AB - edge
    Edge <T> AB() const { return mAB; }
    //! Returns AC - edge
    Edge <T> AC() const { return mAC; }
    //! Returns BC - edge
    Edge <T> BC() const { return mBC; }
    //! Set the triangle wrong or not for triangulation
    void setWrong(bool wrong) { mWrong = wrong; }
    //! Returns true if triangle is wrong for triangulation
    bool isWrong() const { return mWrong; }
    //! Returns reference to the circum circle of the triangle
    Circle <T> circle() const { return mCircumCircle; }
    //! Returns true if any edge of the triangle shared with given edge
    bool sharedWithEdge(const Edge <T>& edge) const {
        return equal(edge, mAB) || equal(edge, mAC) || equal(edge, mBC);
    }
    //! Checks whether the triangle contains some point
    bool contains(const Point3D <T>& point) const {
        return equal(mA, point) || equal(mB, point) || equal(mC, point);
    }

    //! Equal operator
    bool operator==(const Triangle& other) const {
        return mA == other.A() && mB == other.B() && mC == other.C();
    }

    //! Not-equal operator
    bool operator!=(const Triangle& other) const {
        return mA != other.A() || mB != other.B() || mC != other.C();
    }

private:
    //! First vertice of triangle
    Point3D <T> mA;
    //! Second vertice of triangle
    Point3D <T> mB;
    //! Third vertice of triangle
    Point3D <T> mC;
    //! Edge 1
    Edge <T> mAB;
    //! Edge 2
    Edge <T> mBC;
    //! Edge 3
    Edge <T> mAC;
    //! Sign == true if edge wrong for triangulation
    bool mWrong;
    //! Circum circle of the triangle
    Circle <T> mCircumCircle;

    //! Creates triangle circum circle
    void createCircumCircle() {

        // Calc half-perimeter of the triangle
        const T P = (mAB.length() + mBC.length() + mAC.length()) / static_cast <T> (2.0);
        // Calc area of the triangle
        const T S = sqrt(P * (P - mAB.length()) * (P - mBC.length()) * (P - mAC.length()));
        // Now, calc radius of the triangle circum circle
        const T R = (mAB.length() * mBC.length() * mAC.length()) / (4.0 * S);

        // Calc triangle circum circle center coordinates
        T A_n = (mA.x() * mA.x() + mA.y() * mA.y());
        T B_n = (mB.x() * mB.x() + mB.y() * mB.y());
        T C_n = (mC.x() * mC.x() + mC.y() * mC.y());

        T D = static_cast <T> (2.0) * ((mA.x() * (mB.y() - mC.y()) + mB.x() * (mC.y() - mA.y()) + mC.x() * (mA.y() - mB.y())));

        T center_x = (A_n * (mB.y() - mC.y()) + B_n * (mC.y() - mA.y()) + C_n * (mA.y() - mB.y())) / D;
        T center_y = (A_n * (mC.x() - mB.x()) + B_n * (mA.x() - mC.x()) + C_n * (mB.x() - mA.x())) / D;

        Point3D <T> center(center_x, center_y, static_cast <T>(0.0));

        // Creating triangle circum circle
        mCircumCircle = Circle <T>(R, center);
    }
};
