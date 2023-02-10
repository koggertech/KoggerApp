#include "Q3DSettingsController.h"

Q3DSettingsController::Q3DSettingsController(QObject *parent)
    : QObject{parent}
    ,mpThread(nullptr)
{

}

Q3DSettingsController::~Q3DSettingsController()
{
    if (mpThread){
        mpThread->terminate();
        mpThread->wait();
    }
}

void Q3DSettingsController::setModel(const ModelPointer pModel)
{
    mpModel = pModel;
}

void Q3DSettingsController::changeSceneVisibility(const bool visible)
{
    if (!mpModel) return;

    mpModel->changeSceneVisibility(visible);
}

void Q3DSettingsController::chageDisplayedObjectType(const QString& type)
{
    if (!mpModel) return;

    mpModel->changeDisplayedObjectType(type);
}

void Q3DSettingsController::updateDisplayedObject()
{
    if (!mpModel || !mpModel->triangulationAvailable())
        return;

    auto process = [this](){
        mpModel->updateSurface();
    };

    mpThread = QThread::create(process);
    mpThread->start();
}
