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

        mLength = sqrt(pow(p2.x() - p1.x(), 2) + pow(p2.y() - p1.y(), 2));

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

    //bool operator<(const Edge <T>& other) const
    //{
    //    return mLength < other.length();
    //}

    bool operator<(const Edge <T>& other)
    {
        return (p1() < other.p1() && p2() < other.p2());
    }

    bool operator==(const Edge <T>& other) const{
        //return ((p1() == other.p1() && p2() == other.p2()) /*|| (p1() == other.p2() && p2() == other.p1())*/);
        return (equal(p1(),other.p1()) && equal(p2(),other.p2())) || (equal(p1(),other.p2()) && equal(p2(),other.p1()));
                /*|| (p1() == other.p2() && p2() == other.p1())*/
    }


    bool isNeighbor(Edge <double> other) {
        int count = 0;
        if (p1() == other.p1() || p1() == other.p2()) {
            count++;
        }
        if (p2() == other.p1() || p2() == other.p2()) {
            count++;
        }
        return count == 2;
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

template <typename T>
bool operator==(const Edge <T>& e1, const Edge <T>& e2)
{
    return e1.operator==(e2);
}

template <typename T>
struct std::hash <Edge <T>>
{
    std::size_t operator()(const Edge <T>& edge) const noexcept
    {
        auto h1 = std::hash<Point3D <T>>{}(edge.p1());
        auto h2 = std::hash<Point3D <T>>{}(edge.p2());
        return h1 ^ h2;
    }
};
