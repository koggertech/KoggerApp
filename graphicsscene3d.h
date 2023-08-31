#ifndef GRAPHICSSCENE3D_H
#define GRAPHICSSCENE3D_H

#include <memory>

#include <QOpenGLFunctions>

#include <graphicssceneobject.h>

class GraphicsScene3d : protected QOpenGLFunctions
{
public:
    GraphicsScene3d();

    void addGraphicsObject(std::shared_ptr <GraphicsSceneObject> object);
    void removeGraphicsObject(std::shared_ptr <GraphicsSceneObject> object);
    void update();

private:
    QList <std::shared_ptr <GraphicsSceneObject>> m_objectList;
};

#endif // GRAPHICSSCENE3D_H
