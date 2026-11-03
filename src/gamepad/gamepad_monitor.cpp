#include "gamepad_monitor.h"

#include "gamepad_worker.h"

#include <QHash>
#include <QVariantMap>

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
    connect(worker_, &GamepadWorker::padsChanged, this, &GamepadMonitor::onWorkerPads);
    connect(worker_, &GamepadWorker::batteryChanged, this, &GamepadMonitor::onWorkerBattery);
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

void GamepadMonitor::attachVirtualPad(int vendorId, int productId)
{
    if (virtualPadAllowed_) {
        GamepadWorker* worker = worker_;
        QMetaObject::invokeMethod(worker_, [worker, vendorId, productId] { worker->attachVirtualPad(vendorId, productId); },
                                  Qt::QueuedConnection);
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

void GamepadMonitor::selectPad(int id)
{
    GamepadWorker* worker = worker_;
    QMetaObject::invokeMethod(worker_, [worker, id] { worker->setPreferredPad(id); }, Qt::QueuedConnection);
}

void GamepadMonitor::setRumble(qreal low, qreal high)
{
    const bool changed = low != rumble_[0] || high != rumble_[1];
    if (!changed && low == 0.0 && high == 0.0) {
        return;
    }

    rumble_ = { low, high };
    GamepadWorker* worker = worker_;
    QMetaObject::invokeMethod(worker_, [worker, low, high] { worker->setRumble(low, high); }, Qt::QueuedConnection);
    if (changed) {
        emit rumbleChanged();
    }
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

void GamepadMonitor::onWorkerConnection(bool connected, int id, const QString& name, const QString& typeName, const QString& family,
                                        const QStringList& faceLabels, int availableButtons)
{
    connected_ = connected;
    padId_ = id;
    name_ = name;
    typeName_ = typeName;
    family_ = family;
    faceLabels_ = faceLabels;
    availableButtons_ = availableButtons;
    emit connectionChanged();
}

void GamepadMonitor::onWorkerPads(const QVariantList& pads)
{
    QHash<QString, int> total;
    for (const QVariant& pad : pads) {
        ++total[pad.toMap().value(QStringLiteral("name")).toString()];
    }

    QHash<QString, int> seen;
    QVariantList labelled;
    for (const QVariant& pad : pads) {
        QVariantMap entry = pad.toMap();
        const QString name = entry.value(QStringLiteral("name")).toString();
        const int nth = ++seen[name];
        entry.insert(QStringLiteral("label"), total.value(name) > 1 ? QStringLiteral("%1 %2").arg(name).arg(nth) : name);
        labelled << entry;
    }

    if (pads_ == labelled) {
        return;
    }

    pads_ = labelled;
    emit padsChanged();
}

void GamepadMonitor::onWorkerBattery(int percent, const QString& state)
{
    if (batteryPercent_ == percent && batteryState_ == state) {
        return;
    }

    batteryPercent_ = percent;
    batteryState_ = state;
    emit batteryChanged();
}

void GamepadMonitor::onWorkerState(int buttons, qreal leftX, qreal leftY, qreal rightX, qreal rightY, qreal leftTrigger, qreal rightTrigger)
{
    buttons_ = buttons;
    axes_ = { leftX, leftY, rightX, rightY, leftTrigger, rightTrigger };
    emit stateChanged();
}
