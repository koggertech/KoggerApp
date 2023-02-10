#pragma once

#include "Point2D.h"

//EDGE CLASS
template <typename T>
class Edge
{
public:

    //! Constructor 1
    Edge()
        : mP1({ 0,0 })
        , mP2({ 1,1 })
        , mIsWrong(false) {

    }

    //! Constructor 2
    Edge(const Point3D <T>& p1, const Point3D <T>& p2)
        : mP1(p1)
        , mP2(p2)
        , mLength(static_cast <T> (0.0))
        , mIsWrong(false) {

        const T dx = p2.x() - p1.x();
        const T dy = p2.y() - p1.y();
        mLength = sqrt(dx * dx + dy * dy);
    }

    //! Returns first vertice of the edge
    Point3D <T> p1() const { return mP1; }
    //! Returns second vertice of edge
    Point3D <T> p2() const { return mP2; }
    //! Returns length of the edge
    T length() const { return mLength; };
    //! Set the edge wrong or not for triangulation
    void setWrong(bool wrong) { mIsWrong = wrong; }
    //! Returns true if edge is wrong for triangulation
    bool isWrong() const { return mIsWrong; }

private:


    //! First point of the edge
    Point3D <T> mP1;
    //! Second point of the edge
    Point3D <T> mP2;
    //! Length of the edge
    T mLength;

    bool mIsWrong;
};
