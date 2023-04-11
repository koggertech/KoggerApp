#pragma once

#include "Point3D.h"
#include <math.h>

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

        mCenter.setX((p1.x() + p2.x()) / static_cast <T> (2.0));
        mCenter.setY((p1.y() + p2.y()) / static_cast <T> (2.0));
        mCenter.setZ((p1.z() + p2.z()) / static_cast <T> (2.0));
    }

    //! Returns first vertice of the edge
    Point3D <T> p1() const { return mP1; }
    //! Returns second vertice of edge
    Point3D <T> p2() const { return mP2; }
    //! Returns length of the edge
    T length() const { return mLength; };
    //! Returns center point of the edge
    Point3D <T> center() const { return mCenter;};
    //! Set the edge wrong or not for triangulation
    void setWrong(bool wrong) { mIsWrong = wrong; }
    //! Returns true if edge is wrong for triangulation
    bool isWrong() const { return mIsWrong; }

    bool operator<(const Edge <T>& other) const
    {
        return mLength < other.length();
    }

private:


    //! First point of the edge
    Point3D <T> mP1;
    //! Second point of the edge
    Point3D <T> mP2;
    //! Center point of the edge
    Point3D <T> mCenter;
    //! Length of the edge
    T mLength;

    bool mIsWrong;
};