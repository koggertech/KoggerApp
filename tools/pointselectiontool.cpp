#include "pointselectiontool.h"

#include <QDebug>

PointSelectionTool::PointSelectionTool()
{

}

std::shared_ptr<VertexObject> PointSelectionTool::use(std::shared_ptr<VertexObject> workingObject)
{
    qDebug() << "Point selection tool";
}
