#pragma once

#include <limits>
#include <type_traits>

namespace Equals {

    template<class T>
    typename std::enable_if<std::is_same<T, float>::value, bool>::type
    equal(T x, T y, int ulp = 2)
    {
        return fabsf(x - y) <= std::numeric_limits<float>::epsilon() * fabsf(x + y) * static_cast<float>(ulp)
            || fabsf(x - y) < std::numeric_limits<float>::min();
    }

    template<class T>
    typename std::enable_if<std::is_same<T, double>::value, bool>::type
    equal(T x, T y, int ulp = 2)
    {
        return fabs(x - y) <= std::numeric_limits<double>::epsilon() * fabs(x + y) * static_cast<double>(ulp)
            || fabs(x - y) < std::numeric_limits<double>::min();
    }

    template<class T>
    typename std::enable_if<std::is_same<T, long double>::value, bool>::type
    equal(T x, T y, int ulp = 2)
    {
        return fabs(x - y) <= std::numeric_limits<long double>::epsilon() * fabs(x + y) * static_cast<long double>(ulp)
            || fabs(x - y) < std::numeric_limits<long double>::min();
    }

}



//template<typename T>
//bool
//equal(const Edge<T>& e1, const Edge<T>& e2)
//{
//    return (equal(e1.p1(), e2.p1()) && equal(e1.p2(), e2.p2())) ||
//        (equal(e1.p1(), e2.p2()) && equal(e1.p2(), e2.p1()));
//}
//
//template<typename T>
//bool equal(const Point3D <T>& p1, const Point3D <T>& p2)
//{
//    return equal(p1.x(), p2.x()) && equal(p1.y(), p2.y());
//}
