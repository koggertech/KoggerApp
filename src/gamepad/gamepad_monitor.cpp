#include "gamepad_monitor.h"

#include "gamepad_worker.h"

GamepadMonitor::GamepadMonitor(QObject* parent)
    : QObject(parent),
      worker_(new GamepadWorker)
{
    thread_.setObjectName(QStringLiteral("GamepadInput"));
    worker_->moveToThread(&thread_);

    connect(&thread_, &QThread::finished, worker_, &QObject::deleteLater);
    connect(worker_, &GamepadWorker::errorChanged, this, &GamepadMonitor::onWorkerError);
    connect(worker_, &GamepadWorker::readyChanged, this, &GamepadMonitor::onWorkerReady);
    connect(worker_, &GamepadWorker::connectionChanged, this, &GamepadMonitor::onWorkerConnection);
    connect(worker_, &GamepadWorker::stateChanged, this, &GamepadMonitor::onWorkerState);
}

GamepadMonitor::~GamepadMonitor()
{
    if (thread_.isRunning()) {
        thread_.quit();
        thread_.wait();
    }
    else {
        delete worker_;
    }
}

bool GamepadMonitor::supported() const
{
#ifdef KOGGER_SDL3
    return true;
#else
    return false;
#endif
}

void GamepadMonitor::setActive(bool active)
{
    if (active_ == active) {
        return;
    }

    active_ = active;
    updateOwners();
    emit activeChanged();
}

void GamepadMonitor::setPresenceActive(bool active)
{
    if (presenceActive_ == active) {
        return;
    }

    presenceActive_ = active;
    updateOwners();
}

void GamepadMonitor::setHelmActive(bool active)
{
    if (helmActive_ == active) {
        return;
    }

    helmActive_ = active;
    updateOwners();
}

void GamepadMonitor::attachVirtualPad()
{
    if (virtualPadAllowed_) {
        QMetaObject::invokeMethod(worker_, &GamepadWorker::attachVirtualPad, Qt::QueuedConnection);
    }
}

void GamepadMonitor::detachVirtualPad()
{
    if (virtualPadAllowed_) {
        QMetaObject::invokeMethod(worker_, &GamepadWorker::detachVirtualPad, Qt::QueuedConnection);
    }
}

void GamepadMonitor::setVirtualInput(int buttons, qreal leftX, qreal leftY, qreal rightX, qreal rightY, qreal leftTrigger, qreal rightTrigger)
{
    if (!virtualPadAllowed_) {
        return;
    }

    GamepadWorker* worker = worker_;
    QMetaObject::invokeMethod(worker_, [=] {
        worker->setVirtualInput(buttons, leftX, leftY, rightX, rightY, leftTrigger, rightTrigger);
    }, Qt::QueuedConnection);
}

void GamepadMonitor::updateOwners()
{
    GamepadWorker* worker = worker_;

    const bool routing = active_ || helmActive_;
    if (routing != routing_) {
        routing_ = routing;
        QMetaObject::invokeMethod(worker_, [worker, routing] { worker->setRouting(routing); }, Qt::QueuedConnection);
    }

    const bool continuous = helmActive_;
    if (continuous != continuous_) {
        continuous_ = continuous;
        QMetaObject::invokeMethod(worker_, [worker, continuous] { worker->setContinuous(continuous); }, Qt::QueuedConnection);
    }

    const bool running = active_ || presenceActive_ || helmActive_;
    if (running == running_) {
        return;
    }

    running_ = running;
    if (running_) {
        if (!thread_.isRunning()) {
            thread_.start();
        }
        QMetaObject::invokeMethod(worker_, &GamepadWorker::start, Qt::QueuedConnection);
    }
    else {
        QMetaObject::invokeMethod(worker_, &GamepadWorker::stop, Qt::QueuedConnection);
    }
    emit runningChanged();
}

void GamepadMonitor::onWorkerError(const QString& error)
{
    if (error_ == error) {
        return;
    }

    error_ = error;
    emit errorChanged();
}

void GamepadMonitor::onWorkerReady(bool ready)
{
    if (ready_ == ready) {
        return;
    }

    ready_ = ready;
    emit readyChanged();
}

void GamepadMonitor::onWorkerConnection(bool connected, const QString& name, const QString& typeName, int availableButtons)
{
    connected_ = connected;
    name_ = name;
    typeName_ = typeName;
    availableButtons_ = availableButtons;
    emit connectionChanged();
}

void GamepadMonitor::onWorkerState(int buttons, qreal leftX, qreal leftY, qreal rightX, qreal rightY, qreal leftTrigger, qreal rightTrigger)
{
    buttons_ = buttons;
    axes_ = { leftX, leftY, rightX, rightY, leftTrigger, rightTrigger };
    emit stateChanged();
}
