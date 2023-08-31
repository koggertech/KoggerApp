#ifndef POINTOBJECT_H
#define POINTOBJECT_H

#include <displayedobject.h>

class PointObject : public DisplayedObject
{
    Q_OBJECT
    Q_PROPERTY(float x                        READ x              WRITE setX NOTIFY coordChanged)
    Q_PROPERTY(float y                        READ y              WRITE setY NOTIFY coordChanged)
    Q_PROPERTY(float z                        READ z              WRITE setZ NOTIFY coordChanged)


public:
    explicit PointObject(QObject *parent = nullptr);

    float x() const;
    float y() const;
    float z() const;

    void setX(float x);
    void setY(float y);
    void setZ(float z);
    void setCoord(float x, float y, float z);

signals:
    void coordChanged(float x, float y, float z);

private:
    float mX = 0.0f;
    float mY = 0.0f;
    float mZ = 0.0f;


};

#endif // POINTOBJECT_H
