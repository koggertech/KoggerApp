#ifndef TOOL3DWORKER_H
#define TOOL3DWORKER_H

#include <QObject>

#include <memory>

#include <vertexobject.h>
#include <abstracttool3d.h>

class Tool3dWorker : public QObject
{
    Q_OBJECT

public:
    explicit Tool3dWorker(QObject *parent = nullptr);

    Tool3dWorker(std::shared_ptr <AbstractTool3d> workingTool,
                 std::shared_ptr <VertexObject> workingObject,
                 QObject *parent = nullptr);

    void setWorkingTool(std::shared_ptr <AbstractTool3d> tool);

    void setWorkingObject(std::shared_ptr <VertexObject> object);

    void useTool();

    void dropTool();

    std::shared_ptr <VertexObject> lastResult() const;

Q_SIGNALS:

    void workFinished();

private:

    std::shared_ptr <AbstractTool3d> mpWorkingTool;
    std::shared_ptr <VertexObject> mpLastWorkResult;
    std::shared_ptr <VertexObject> mpWorkingObject;
};

#endif // TOOL3DWORKER_H
