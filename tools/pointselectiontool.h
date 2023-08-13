#ifndef POINTSELECTIONTOOL_H
#define POINTSELECTIONTOOL_H

#include <abstracttool3d.h>

class PointSelectionTool : public AbstractTool3d
{
public:
    PointSelectionTool();

    virtual std::shared_ptr <VertexObject> use(std::shared_ptr <VertexObject> workingObject) override;
};

#endif // POINTSELECTIONTOOL_H
