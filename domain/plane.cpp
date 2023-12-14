#include "plane.h"

Plane::Plane(const QVector3D &bottomLeft, const QVector3D &topRight)
    : m_bottomLeft(bottomLeft)
    , m_topRight(topRight)
{

}

QVector3D Plane::bottomLeft() const
{
    return m_bottomLeft;
}

QVector3D Plane::topRight() const
{
    return m_topRight;
}

float Plane::width() const
{
    return std::abs(topRight().y()-bottomLeft().y());
}

float Plane::length() const
{
    return std::abs(topRight().x()-bottomLeft().x());
}

QSizeF Plane::size() const
{
    return {std::abs(topRight().y()-bottomLeft().y()),
            std::abs(topRight().x()-bottomLeft().x())};
}

QVector3D Plane::center() const
{
    auto x = m_bottomLeft.x()+length()/2.0f;
    auto y = m_bottomLeft.y()+width()/2.0f;
    auto z = m_bottomLeft.z();

    return {x,y,z};
}
