#ifndef SURFACEGRID_H
#define SURFACEGRID_H

#include <QObject>

#include <scenegraphicsobject.h>

class SurfaceGrid : public SceneGraphicsObject
{
    Q_OBJECT
    Q_PROPERTY(bool isTriangle READ isTriangle CONSTANT)
    Q_PROPERTY(bool isQuad     READ isQuad     CONSTANT)

public:
    explicit SurfaceGrid(QObject *parent = nullptr);

    bool isTriangle() const;
    bool isQuad() const;
};

#endif // SURFACEGRID_H
