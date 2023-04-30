#ifndef ABSTRACTDELAUNAYPROCESSOR_H
#define ABSTRACTDELAUNAYPROCESSOR_H

#include <Triangle.h>
#include <point2d.h>
#include <Equals.h>

namespace DelaunayTin
{
    template <typename T>
    class AbstractDelaunayProcessor
    {
    public:
        virtual std::vector <Triangle <T>> build(std::vector <Point3D <T>> points, T edgeLengthLimit = -1.0f) = 0;
    };
}

#endif // ABSTRACTDELAUNAYPROCESSOR_H
