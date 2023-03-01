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

void Q3DSettingsController::chageDisplayedStage(const QString& stage)
{
    if (!mpModel) return;

    mpModel->chageDisplayedStage(stage);
}

void Q3DSettingsController::changeMaxTriangulationLineLength(const int length)
{
    if (!mpModel) return;

    mpModel->changeMaxTriangulationLineLength(length);
}

void Q3DSettingsController::setInterpolationLevel(const QString& level)
{
    uint8_t value = static_cast <uint8_t> (level.toUInt());
    mpModel->setInterpolationLevel(value);

    if (!mpModel || !mpModel->triangulationAvailable())
        return;

    auto process = [this](){
       mpModel->interpolate();
    };

    mpThread = QThread::create(process);
    mpThread->start();
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

void Q3DSettingsController::changeBottomTrackVisibility(const bool visible)
{
    mpModel->changeBottomTrackVisibility(visible);
}

void Q3DSettingsController::changeSurfaceVisibility(const bool visible)
{
    mpModel->changeSurfaceVisibility(visible);
}

void Q3DSettingsController::changeSurfaceGridVisibility(const bool visible)
{
    mpModel->changeSurfaceGridVisibility(visible);
}
