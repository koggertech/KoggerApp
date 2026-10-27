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
    if (active_) {
        if (!thread_.isRunning()) {
            thread_.start();
        }
        QMetaObject::invokeMethod(worker_, &GamepadWorker::start, Qt::QueuedConnection);
    }
    else {
        QMetaObject::invokeMethod(worker_, &GamepadWorker::stop, Qt::QueuedConnection);
    }
    emit activeChanged();
}

void GamepadMonitor::onWorkerError(const QString& error)
{
    if (error_ == error) {
        return;
    }

    error_ = error;
    emit errorChanged();
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
