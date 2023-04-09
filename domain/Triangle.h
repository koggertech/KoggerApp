#pragma once

#include "Point3D.h"
#include "Edge.h"
#include "Circle.h"
#include <math.h>

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

    //! Returns area of the triangle
    T area() const {return mArea;}

    Edge <T> minLengthEdge() const{
        auto min = std::min(AB(), AC());
        min = std::min(min, BC());
        return min;
    }

    Edge <T> maxLengthEdge() const{
        auto max = std::max(AB(), AC());
        max = std::max(max, BC());
        return max;
    }

    std::vector <Triangle <T>> splitByCenter()
    {
        // Находим координату центра треугольника

        T x_mean = (mA.x() + mB.x() + mC.x())/static_cast <T> (3.0f);
        T y_mean = (mA.y() + mB.y() + mC.y())/static_cast <T> (3.0f);
        //T z_mean = (mA.z() + mB.z() + mC.z())/static_cast <T> (3.0f);

        Point3D <T> centrum(x_mean, y_mean, static_cast <T>(0.0f));

        // Находим координаты середин сторон треугольника
        Point3D <T> ab_h{
            (mA.x() + mB.x()) / static_cast <double> (2.0f),
            (mA.y() + mB.y()) / static_cast <double> (2.0f),
            (mA.z() + mB.z()) / static_cast <double> (2.0f)
        };

        Point3D <T> bc_h{
            (mB.x() + mC.x()) / static_cast <double> (2.0f),
            (mB.y() + mC.y()) / static_cast <double> (2.0f),
            (mB.z() + mC.z()) / static_cast <double> (2.0f)
        };

        Point3D <T> ac_h{
            (mA.x() + mC.x()) / static_cast <double> (2.0f),
            (mA.y() + mC.y()) / static_cast <double> (2.0f),
            (mA.z() + mC.z()) / static_cast <double> (2.0f)
        };

        Triangle <T> t1(mA, ab_h, centrum);
        Triangle <T> t2(ab_h, mB, centrum);
        Triangle <T> t3(mB, bc_h, centrum);
        Triangle <T> t4(bc_h, mC, centrum);
        Triangle <T> t5(mC, ac_h, centrum);
        Triangle <T> t6(ac_h, mA, centrum);

        std::vector <Triangle <T>> result;

        result.push_back(t1);
        result.push_back(t2);
        result.push_back(t3);
        result.push_back(t4);
        result.push_back(t5);
        result.push_back(t6);

        return result;
    }

    std::vector <Triangle <T>> bisectionSplit()
    {
        // Находим координаты середин сторон треугольника
        Point3D <T> ab_h{
            (mA.x() + mB.x()) / static_cast <double> (2.0f),
            (mA.y() + mB.y()) / static_cast <double> (2.0f),
            (mA.z() + mB.z()) / static_cast <double> (2.0f)
        };

        Point3D <T> bc_h{
            (mB.x() + mC.x()) / static_cast <double> (2.0f),
            (mB.y() + mC.y()) / static_cast <double> (2.0f),
            (mB.z() + mC.z()) / static_cast <double> (2.0f)
        };

        Point3D <T> ac_h{
            (mA.x() + mC.x()) / static_cast <double> (2.0f),
            (mA.y() + mC.y()) / static_cast <double> (2.0f),
            (mA.z() + mC.z()) / static_cast <double> (2.0f)
        };

        std::vector <Triangle <T>> result;

        result.push_back(Triangle <T>(mA,ab_h,ac_h));
        result.push_back(Triangle <T>(ab_h,ac_h,bc_h));
        result.push_back(Triangle <T>(ac_h,bc_h,mC));
        result.push_back(Triangle <T>(ab_h,mB,bc_h));

        return result;
    }

    std::vector <Point3D <T>> vertices()
    {
        std::vector <Point3D <T>> vertices;

        vertices.push_back(mA);
        vertices.push_back(mB);
        vertices.push_back(mC);

        return vertices;
    }

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
    bool contains(const Point3D <T>& p)  {
        T b = vect(mA.x() - p.x(), mA.y() - p.y(), mB.x() - mA.x(), mB.y() - mA.y());

        T q = vect(mB.x() - p.x(), mB.y() - p.y(), mC.x() - mB.x(), mC.y() - mB.y());

        T r = vect(mC.x() - p.x(), mC.y() - p.y(), mA.x() - mC.x(), mA.y() - mC.y());

        return (b <= 0.0 && q <= 0.0 && r <= 0.0) || (b >= 0.0 && q >= 0.0 && r >= 0.0);
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
    //! Area of the triangle
    T mArea;

    //! Creates triangle circum circle
    void createCircumCircle() {

        // Calc half-perimeter of the triangle
        const T P = (mAB.length() + mBC.length() + mAC.length()) / static_cast <T> (2.0);
        // Calc area of the triangle
        const T mArea = sqrt(P * (P - mAB.length()) * (P - mBC.length()) * (P - mAC.length()));
        // Now, calc radius of the triangle circum circle
        const T R = (mAB.length() * mBC.length() * mAC.length()) / (4.0 * mArea);

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

    T vect(const T& x1, const T& y1, const T& x2, const T& y2)
    {
        return x1 * y2 - y1 * x2;
    }

};
