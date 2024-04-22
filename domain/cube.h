#ifndef CUBE_H
#define CUBE_H

#include <plane.h>

#include <QVector3D>
#include <QDebug>
#include <cmath>


class Cube {
public:
    Cube();
    Cube(float x_1, float x_2,
         float y_1, float y_2,
         float z_1, float z_2);

    float minimumX() const;
    float maximumX() const;
    float minimumY() const;
    float maximumY() const;
    float minimumZ() const;
    float maximumZ() const;
    QVector3D center() const;
    QVector3D bottomPos() const;
    float length() const;
    float width() const;
    float height() const;
    Cube merge(const Cube& other);
    bool isEmpty() const;
    bool isValid() const;
    Plane bottom() const;
    Plane top() const;

private:
    float m_xMin;
    float m_xMax;
    float m_yMin;
    float m_yMax;
    float m_zMin;
    float m_zMax;
    bool m_isValid;
};

inline QDebug operator<<(QDebug stream,const Cube& cube)
{
    stream << "\n";
    stream << "       ____________  \n";
    stream << "      /|          /| \n";
    stream << "     / |         / | \n";
    stream << "    /__|________/  | \n";
    stream << "    |  |        |  | h =" << cube.height() << "\n";
    stream << "    |  |________|__| \n";
    stream << "    | /         |  / \n";
    stream << "    |/          | / l =" << cube.length() << "\n";
    stream << "    |___________|/   \n";
    stream << "       w ="<<cube.width();
    stream << "\n";

    return stream;
}

#endif // CUBE_H
