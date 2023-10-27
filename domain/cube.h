#ifndef CUBE_H
#define CUBE_H

#include <QVector3D>

#include <cmath>

class Cube{

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

private:
    float m_xMin = 0.0f;
    float m_xMax = 0.0f;
    float m_yMin = 0.0f;
    float m_yMax = 0.0f;
    float m_zMin = 0.0f;
    float m_zMax = 0.0f;
};


#endif // CUBE_H
