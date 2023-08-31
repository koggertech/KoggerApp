#ifndef GRAPHICSSCENEOBJECT_H
#define GRAPHICSSCENEOBJECT_H

#include "graphicssceneentity.h"

class GraphicsSceneObject : public GraphicsSceneEntity
{
    Q_OBJECT
public:
    explicit GraphicsSceneObject(QObject *parent = nullptr);
};

#endif // GRAPHICSSCENEOBJECT_H
