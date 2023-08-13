#ifndef ABSTRACTTOOL3D_H
#define ABSTRACTTOOL3D_H

#include <vertexobject.h>

#include <memory>

class AbstractTool3d
{
public:

    virtual std::shared_ptr <VertexObject> use(std::shared_ptr <VertexObject> workingObject) = 0;

};

#endif // ABSTRACTTOOL3D_H
