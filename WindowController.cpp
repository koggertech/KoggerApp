#include "WindowController.h"


WindowController::WindowController(QWindow* window, QObject* parent) :
    QObject(parent),
    window_(window),
    isFullScreen_(false)
{

}

void WindowController::setWindow(QWindow* window)
{
    window_ = window;
}

void WindowController::toggleFullScreen()
{
    if (!window_)
        return;

    isFullScreen_ ? window_->showNormal() : window_->showFullScreen();

    isFullScreen_= !isFullScreen_;
}
