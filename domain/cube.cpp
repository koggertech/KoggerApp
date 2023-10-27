#include "cube.h"

Cube::Cube()
{}

Cube::Cube(float x_1, float x_2,
         float y_1, float y_2,
         float z_1, float z_2)
:m_xMin(x_1), m_xMax(x_2)
,m_yMin(y_1), m_yMax(y_2)
,m_zMin(z_1), m_zMax(z_2)
{}

QVector3D Cube::center() const{
    return {
        m_xMin + length()/2.0f,
        m_yMin + width()/2.0f,
        m_zMin + height()/2.0f
    };
};

QVector3D Cube::bottomPos() const {
    return {
        m_xMin + length()/2.0f,
        m_yMin + width()/2.0f,
        m_zMin
    };
}

float Cube::length() const{
    return std::abs(m_xMax - m_xMin);
}

float Cube::width() const
{
    return std::abs(m_yMax - m_yMin);
}

float Cube::height() const{
    return std::abs(m_zMax - m_zMin);
}

Cube Cube::merge(const Cube &other)
{
    m_xMin = std::fminf(m_xMin, other.m_xMin);
    m_xMax = std::fmaxf(m_xMax, other.m_xMax);

    m_yMin = std::fminf(m_yMin, other.m_yMin);
    m_yMax = std::fmaxf(m_yMax, other.m_yMax);

    m_zMin = std::fminf(m_zMin, other.m_zMin);
    m_zMax = std::fmaxf(m_zMax, other.m_zMax);

    return *this;
}

float Cube::minimumX() const
{
    return m_xMin;
}

float Cube::maximumX() const
{
    return m_xMax;
}

float Cube::minimumY() const
{
    return m_yMin;
}

float Cube::maximumY() const
{
    return m_yMax;
}

float Cube::minimumZ() const
{
    return m_zMin;
}

float Cube::maximumZ() const
{
    return m_zMax;
}
