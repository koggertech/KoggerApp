#pragma once

#include <math.h>

#include "point_3d.h"
#include "equals.h"

template <typename T>
class Edge
{
public:

    //! Constructor 1
    Edge()
        : mP1({ 0,0 })
        , mP2({ 1,1 })
        , mLength(static_cast <T> (0.0))
        , mIsWrong(false)
    {}

    //! Constructor 2
    Edge(const Point3D <T>& p1, const Point3D <T>& p2)
        : mP1(p1)
        , mP2(p2)
        , mLength(static_cast <T> (0.0))
        , mIsWrong(false)
    {

        calculateLength();
        calculateCenter();
    }

    //! Constructor 2
    Edge(const QVector3D& p1, const QVector3D& p2)
        : mP1(Point3D <T>::fromQVector3D(p1))
        , mP2(Point3D <T>::fromQVector3D(p2))
        , mLength(static_cast <T> (0.0))
        , mIsWrong(false)
    {
        calculateLength();
        calculateCenter();
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

    bool operator<(const Edge <T>& other) {
        return (p1() < other.p1() && p2() < other.p2());
    }

    bool operator==(const Edge <T>& other) const {
        return (p1().equal(other.p1()) && p2().equal(other.p2())) ||
               (p1().equal(other.p2()) && p2().equal(other.p1()));
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


#ifdef QT_CORE_LIB

    bool containsPoint(const QVector3D& point){


        auto edgeLength = sqrt(pow(mP2.x() - mP1.x(), 2) + pow(mP2.y() - mP1.y(), 2) + pow(mP2.z() - mP1.z(), 2));
        auto distToP1 = sqrt(pow(point.x() - mP1.x(), 2) + pow(point.y() - mP1.y(), 2) + pow(point.z() - mP1.z(), 2));
        auto distToP2 = sqrt(pow(point.x() - mP2.x(), 2) + pow(point.y() - mP2.y(), 2) + pow(point.z() - mP2.z(), 2));

        if ((distToP1 + distToP2) == edgeLength){
            return true;
        }

        return false;
    }

    bool intersectsWithLine(const QVector3D& origin,
                            const QVector3D& direction,
                            QVector3D& intersectionPoint,
                            bool dirNormalized = false)
    {
        auto dir = dirNormalized? direction : direction.normalized();

        auto p1 = mP1.toQVector3D();
        auto p2 = mP2.toQVector3D();

        auto da = p2 - p1;
        auto db = dir;
        auto dc = origin - p1;

        auto s = QVector3D::dotProduct(QVector3D::crossProduct(dc, db)
                                      ,QVector3D::crossProduct(da, db)) /
                                       QVector3D::crossProduct(da, db).lengthSquared();

        auto p = p1 + da * s;

        if (containsPoint(p)){
            intersectionPoint = p;
            return true;
        }

        return false;

    }
#endif //QT_CORE_LIB

private:

    void calculateLength(){
        mLength = sqrt(pow(mP2.x() - mP1.x(), 2) + pow(mP2.y() - mP1.y(), 2));
    }

    void calculateCenter(){
        mCenter.setX((mP1.x() + mP2.x()) / static_cast <T> (2.0));
        mCenter.setY((mP1.y() + mP2.y()) / static_cast <T> (2.0));
        mCenter.setZ((mP1.z() + mP2.z()) / static_cast <T> (2.0));
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

#ifdef QT_CORE_LIB
template <typename T>
uint qHash(const Edge <T> &e, uint seed = 4)
{
    Q_UNUSED(seed)

    auto h1 = qHash(e.p1());
    auto h2 = qHash(e.p2());
    return h1 ^ h2;
}
#endif
