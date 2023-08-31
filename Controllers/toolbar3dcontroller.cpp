#include "toolbar3dcontroller.h"

Toolbar3dController::Toolbar3dController(std::shared_ptr <ActiveObjectProvider> selectedObjectModel,
                                         std::shared_ptr <Tool3dWorker> toolWorker,
                                         QObject* parent)
: QObject(parent)
, mpActiveObjectProvider(selectedObjectModel)
, mpToolWorker(toolWorker)
{}

Toolbar3dController::~Toolbar3dController()
{

}

void Toolbar3dController::setSelectionToolState(bool enabled)
{
    if(!enabled){
        mpToolWorker->dropTool();
        return;
    }

    mpToolWorker->setWorkingTool(
                    std::make_shared <PointSelectionTool>()
                );

    //mpToolWorker->setWorkingObject(
    //                mpActiveObjectProvider->activeObject()
    //            );

    //mpToolWorker->useTool();
}
