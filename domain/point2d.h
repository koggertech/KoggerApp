#ifndef POINT2D_H
#define POINT2D_H

//#include <hash_map>

#ifdef QT_CORE_LIB
#include <QHash>
#endif

template <typename T>
class Point2D
{
public:

    Point2D() = default;

    /**
     * @brief Constructor with params.
     * @param[in] x Value of x.
     * @param[in] y Value of y.
     */
    Point2D(const T& x, const T& y, size_t index = 0)
    : mX(x)
    , mY(y)
    , mIndex(index)
    {};

    /**
     * @brief Returns x value of point.
     * @return x value of point.
     */
    T x() const { return mX; };

    /**
     * @brief Returns y value of point.
     * @return y value of point.
     */
    T y() const { return mY; };

    /**
     * @brief Sets x value of point.
     * @param[in] value Value of x.
     */
    void setX(T value) { mX = value; };

    /**
     * @brief Sets x value of point.
     * @param[in] value Value of y.
     */
    void setY(T value) { mY = value; };

    size_t index() const {return mIndex;};

    bool operator==(const Point2D <T>& other) const {
        return mX == other.x() && mY == other.y();
    };

    bool operator<(const Point2D <T>& other) const {
        if (x() == other.x()) return x() < other.x();
        return y() < other.y();
    };

    bool operator!=(const Point2D& other) const {
        return x() != other.x() || y() != other.y();
    };

protected:

    T mX{static_cast <T>(0.0f)};
    T mY{static_cast <T>(0.0f)};
    size_t mIndex{0};

};

template <typename T>
bool operator==(const Point2D <T>& p1, const Point2D <T>& p2){
    return p1.x() == p2.x() && p1.y() == p2.y();
}

template <typename T>
bool operator<(const Point2D <T>& p1, const Point2D <T>& p2){
    if (p1.x() == p2.x()) return p1.x() < p2.x();
    return p1.y() < p2.y();
};

template <typename T>
struct std::hash <Point2D <T>>
{
    std::size_t operator()(const Point2D <T>& p) const noexcept
    {
        auto h1 = std::hash<double>{}(p.x());
        auto h2 = std::hash<double>{}(p.y());
        return h1 ^ (h2 << 1);
    }
};

#ifdef QT_CORE_LIB
template <typename T>
uint qHash(const Point2D <T>& p){
    auto h1 = qHash(p.x());
    auto h2 = qHash(p.y());
    return h1 ^ (h2 << 1);
}
#endif

#endif // POINT2D_H
