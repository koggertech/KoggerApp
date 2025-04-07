#ifndef SURFACEGRID_H
#define SURFACEGRID_H

#include <QObject>

#include "scene_object.h"

class SurfaceGrid : public SceneObject
{
    Q_OBJECT
    Q_PROPERTY(bool isTriangle READ isTriangle CONSTANT)
    Q_PROPERTY(bool isQuad     READ isQuad     CONSTANT)

public:
    explicit SurfaceGrid(QObject *parent = nullptr);
    virtual ~SurfaceGrid();

    bool isTriangle() const;
    bool isQuad() const;

private:
    friend class Surface;
};

#endif // SURFACEGRID_H
