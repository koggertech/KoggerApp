#ifndef PLANE_H
#define PLANE_H

#include <QVector3D>
#include <QSizeF>

class Plane
{
public:
    Plane(const QVector3D& bottomLeft, const QVector3D& topRight);

    QVector3D bottomLeft() const;
    QVector3D topRight() const;
    float width() const;
    float length() const;
    QSizeF size() const;
    QVector3D center() const;

private:
     QVector3D m_bottomLeft;
     QVector3D m_topRight;
};

#endif // PLANE_H
