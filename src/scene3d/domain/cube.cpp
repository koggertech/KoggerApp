#include "cube.h"


Cube::Cube()
    : m_xMin(0.f), m_xMax(0.f),
    m_yMin(0.f), m_yMax(0.f),
    m_zMin(0.f), m_zMax(0.f),
    m_isValid(false)
{}

Cube::Cube(float x_1, float x_2,
           float y_1, float y_2,
           float z_1, float z_2)
    : m_xMin(x_1), m_xMax(x_2),
    m_yMin(y_1), m_yMax(y_2),
    m_zMin(z_1), m_zMax(z_2),
    m_isValid(true)
{}

QVector3D Cube::center() const
{
    return { m_xMin + length() / 2.f,
             m_yMin + width()  / 2.f,
             m_zMin + height() / 2.f };
};

QVector3D Cube::bottomPos() const
{
    return { m_xMin + length() / 2.f,
             m_yMin + width()  / 2.f,
             m_zMin };
}

float Cube::length() const
{
    return std::fabs(m_xMax - m_xMin);
}

float Cube::width() const
{
    return std::fabs(m_yMax - m_yMin);
}

float Cube::height() const{
    return std::fabs(m_zMax - m_zMin);
}

Cube Cube::merge(const Cube &other)
{
    //if (isEmpty()) // TODO: chacking
    //    return other;

    //if (other.isEmpty())
    //    return *this;

    float n1{ m_xMin }, f1{ m_xMin }; // x borders
    m_xMax < m_xMin ? n1 = m_xMax : f1 = m_xMax;
    float n2{ other.m_xMin }, f2{ other.m_xMin };
    other.m_xMax < other.m_xMin ? n2 = other.m_xMax : f2 = other.m_xMax;

    float l1{ m_yMin }, r1{ m_yMin }; // y borders
    m_yMax < m_yMin ? l1 = m_yMax : r1 = m_yMax;
    float l2{ other.m_yMin }, r2{ other.m_yMin };
    other.m_yMax < other.m_yMin ? l2 = other.m_yMax : r2 = other.m_yMax;

    float t1{ m_zMin }, b1{ m_zMin };  // z borders
    m_zMax < m_zMin ? t1 = m_zMax : b1 = m_zMax;
    float t2{ other.m_zMin }, b2{ other.m_zMin };
    other.m_zMax < other.m_zMin ? t2 = other.m_zMax : b2 = other.m_zMax;

    m_xMin = qMin(n1, n2);
    m_xMax = qMax(f1, f2);
    m_yMin = qMin(l1, l2);
    m_yMax = qMax(r1, r2);
    m_zMin = qMin(t1, t2);
    m_zMax = qMax(b1, b2);

    return *this;
}

bool Cube::isEmpty() const
{
    return (m_xMin == m_xMax) && (m_yMin == m_yMax) && (m_zMin == m_zMax);
}

bool Cube::isValid() const
{
    return m_isValid;
}

Plane Cube::bottom() const
{
    return Plane({ m_xMin, m_yMin, m_zMin },
                 { m_xMax, m_yMax, m_zMin });
}

Plane Cube::top() const
{
    return Plane({ m_xMin, m_yMin, m_zMax },
                 { m_xMax, m_yMax, m_zMax });
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
