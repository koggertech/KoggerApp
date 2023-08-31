#ifndef SCENE3DTOOLBARCONTROLLER_H
#define SCENE3DTOOLBARCONTROLLER_H

#include <QObject>

#include <activeobjectprovider.h>
#include <tool3dworker.h>
#include <pointselectiontool.h>

class Toolbar3dController : public QObject
{
    Q_OBJECT

public:
    Toolbar3dController(std::shared_ptr <ActiveObjectProvider> selectedObjectModel,
                        std::shared_ptr <Tool3dWorker> toolWorker,
                        QObject* parent = nullptr);

    virtual ~Toolbar3dController();

    Q_INVOKABLE void setSelectionToolState(bool enabled);

private:

    std::shared_ptr <ActiveObjectProvider> mpActiveObjectProvider;
    std::shared_ptr <Tool3dWorker> mpToolWorker;
};

#endif // SCENE3DTOOLBARCONTROLLER_H
