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

void Q3DSettingsController::changeCalculationMethod(const QString& method)
{
    if (!mpModel) return;

    mpModel->changeCalculationMethod(method);
}

void Q3DSettingsController::changeSmoothingMethod(const QString& method)
{
    if (!mpModel) return;

    mpModel->changeSmoothingMethod(method);
}

void Q3DSettingsController::setTriangulationEdgeLengthLimit(double length)
{
    if (!mpModel) return;

    mpModel->setTriangulationEdgeLengthLimit(length);
}

void Q3DSettingsController::setInterpolationLevel(const QString& level)
{
    Q_UNUSED(level)
}


void Q3DSettingsController::updateDisplayedObject()
{
    if (!mpModel || !mpModel->triangulationAvailable())
        return;

    auto process = [this](){
      mpModel->updateSurface();
    };
#if !defined(Q_OS_ANDROID)
    mpThread = QThread::create(process);
    mpThread->start();
#endif
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

void Q3DSettingsController::changeContourVisibility(bool visible)
{
    mpModel->changeContourVisibility(visible);
}

void Q3DSettingsController::changeContourColor(QColor color)
{
    mpModel->changeContourColor(color);
}

void Q3DSettingsController::changeContourLineWidth(float width)
{
    mpModel->changeContourLineWidth(width);
}

void Q3DSettingsController::changeContourKeyPointsVisibility(bool visible)
{
    mpModel->changeContourKeyPointsVisibility(visible);
}

void Q3DSettingsController::changeContourKeyPointsColor(QColor color)
{
    mpModel->changeContourKeyPointsColor(color);
}

void Q3DSettingsController::changeGridType(QString type)
{
    mpModel->changeGridType(type);
}

void Q3DSettingsController::changeGridCellSize(double size)
{
    mpModel->changeGridCellSize(size);
}
