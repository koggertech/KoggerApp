#include "tool3dworker.h"

Tool3dWorker::Tool3dWorker(QObject *parent)
{

}

Tool3dWorker::Tool3dWorker(std::shared_ptr <AbstractTool3d> workingTool,
                           std::shared_ptr <VertexObject> workingObject,
                           QObject *parent)
: QObject{parent}
, mpWorkingTool(workingTool)
, mpWorkingObject(workingObject)
{}

void Tool3dWorker::setWorkingTool(std::shared_ptr<AbstractTool3d> tool)
{
    mpWorkingTool = tool;
}

void Tool3dWorker::setWorkingObject(std::shared_ptr<VertexObject> object)
{
    mpWorkingObject = object;
}

void Tool3dWorker::useTool()
{
    if (!mpWorkingTool || !mpWorkingObject)
        return;

    mpLastWorkResult = mpWorkingTool->use(mpWorkingObject);

    Q_EMIT workFinished();
}

void Tool3dWorker::dropTool()
{
    mpWorkingTool.reset();
}

std::shared_ptr<VertexObject> Tool3dWorker::lastResult() const
{
    return mpLastWorkResult;
}
