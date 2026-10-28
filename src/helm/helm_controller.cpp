#include "helm_controller.h"

#include <QGuiApplication>
#include <QSettings>

#include "device_manager_wrapper.h"
#include "gamepad_monitor.h"
#include "notifications.h"

namespace {

constexpr int kModeManual = 0;
constexpr int kModeHold = 4;
constexpr int kModeLoiter = 5;
constexpr int kCmdDoSetMode = 176;
constexpr int kResultAccepted = 0;
constexpr int kResultInProgress = 5;

constexpr qint64 kSampleTimeoutMs = 250;
constexpr int kModeTimeoutMs = 3000;
constexpr int kParamPaceMs = 100;
constexpr int kParamsPerPace = 5;
constexpr int kCheckPollMs = 100;
constexpr qint64 kCheckRetryMs = 2000;
constexpr qint64 kCheckDeadlineMs = 4000;

constexpr qreal kLimitStep = 0.25;
constexpr qreal kLimitMin = 0.25;
constexpr qreal kDeadzoneMax = 0.5;
constexpr int kRateMinHz = 10;
constexpr int kRateMaxHz = 25;
constexpr int kMsPerSecond = 1000;

constexpr int kRcChannels = 16;
constexpr int kAuxRcOverrideEnable = 46;
constexpr int kGcsSystemId = 255;
constexpr int kRcOptionsIgnoreOverrides = 1 << 1;

const char kSettingsGroup[] = "main/gamepadHelm";
const char kBindingsGroup[] = "bindings";
const char kParamOverrideTime[] = "RC_OVERRIDE_TIME";
const char kParamRcOptions[] = "RC_OPTIONS";
const char kParamFsThrottle[] = "FS_THR_ENABLE";
const char kParamFsGcs[] = "FS_GCS_ENABLE";
const char kParamGcsSysid[] = "MAV_GCS_SYSID";
const char kParamGcsSysidHigh[] = "MAV_GCS_SYSID_HI";
const char kParamGcsSysidOld[] = "SYSID_MYGCS";
const char kParamMapSteering[] = "RCMAP_ROLL";
const char kParamMapThrottle[] = "RCMAP_THROTTLE";
const char kParamSteerType[] = "PILOT_STEER_TYPE";

const char* bindingKey(int action)
{
    switch (action) {
    case helm::ActionThrottleForward:
        return "throttleForward";
    case helm::ActionThrottleReverse:
        return "throttleReverse";
    case helm::ActionSteering:
        return "steering";
    case helm::ActionToggleLoiter:
        return "toggleLoiter";
    case helm::ActionHold:
        return "hold";
    default:
        return "";
    }
}

const char* kindName(helm::ActionKind kind)
{
    switch (kind) {
    case helm::ActionKind::Throttle:
        return "throttle";
    case helm::ActionKind::Steering:
        return "steering";
    case helm::ActionKind::Button:
        break;
    }
    return "button";
}

QVariantList toVariantList(const std::vector<int>& values)
{
    QVariantList list;
    for (int value : values) {
        list << value;
    }
    return list;
}

QString rcParam(int channel, const char* suffix)
{
    return QStringLiteral("RC%1_%2").arg(channel).arg(QLatin1String(suffix));
}

bool modeKeepsSession(int mode)
{
    return mode == kModeManual || mode == kModeHold || mode == kModeLoiter;
}

}

HelmController::HelmController(GamepadMonitor* monitor, DeviceManagerWrapper* vehicle, Notifications* notifications, QObject* parent)
    : QObject(parent),
      monitor_(monitor),
      vehicle_(vehicle),
      notifications_(notifications),
      sendTimer_(this),
      paramTimer_(this),
      checkTimer_(this),
      modeTimer_(this)
{
    loadTuning();
    loadBindings();
    forwardLimit_ = startForwardLimit_;
    clock_.start();

    sendTimer_.setTimerType(Qt::PreciseTimer);
    sendTimer_.setInterval(kMsPerSecond / rateHz_);
    paramTimer_.setInterval(kParamPaceMs);
    checkTimer_.setInterval(kCheckPollMs);
    modeTimer_.setSingleShot(true);
    modeTimer_.setInterval(kModeTimeoutMs);

    connect(&sendTimer_, &QTimer::timeout, this, &HelmController::onSendTick);
    connect(&paramTimer_, &QTimer::timeout, this, &HelmController::pumpParams);
    connect(&checkTimer_, &QTimer::timeout, this, &HelmController::onCheckDeadline);
    connect(&modeTimer_, &QTimer::timeout, this, [this] { onModeResult(false); });

    connect(monitor_, &GamepadMonitor::connectionChanged, this, &HelmController::onPadConnection);
    connect(monitor_, &GamepadMonitor::stateChanged, this, &HelmController::onPadState);
    connect(vehicle_, &DeviceManagerWrapper::vruChanged, this, &HelmController::onVehicleChanged);
    connect(vehicle_, &DeviceManagerWrapper::autopilotParamReceived, this, &HelmController::onParam);
    connect(vehicle_, &DeviceManagerWrapper::autopilotCommandAcked, this, &HelmController::onCommandAcked);
    if (auto* app = qobject_cast<QGuiApplication*>(QCoreApplication::instance())) {
        connect(app, &QGuiApplication::applicationStateChanged, this, &HelmController::onApplicationStateChanged);
        connect(app, &QCoreApplication::aboutToQuit, this, [this] {
            if (state_ == Starting || state_ == Driving) {
                sendCommand(helm::Command{});
                vehicle_->autopilotSetMode(kModeLoiter);
            }
            finish(QString());
        });
    }

    onVehicleChanged();
}

HelmController::~HelmController()
{
    monitor_->setHelmActive(false);
}

bool HelmController::available() const
{
    return monitor_->supported() && monitor_->connected() && vehicle_->autopilotOnline();
}

void HelmController::setStickDeadzone(qreal value)
{
    tuning_.stickDeadzone = std::clamp(value, 0.0, kDeadzoneMax);
    saveTuning();
}

void HelmController::setTriggerDeadzone(qreal value)
{
    tuning_.triggerDeadzone = std::clamp(value, 0.0, kDeadzoneMax);
    saveTuning();
}

void HelmController::setThrottleExpo(qreal value)
{
    tuning_.throttleExpo = std::clamp(value, 0.0, 1.0);
    saveTuning();
}

void HelmController::setStartForwardLimit(qreal value)
{
    startForwardLimit_ = std::clamp(value, kLimitMin, 1.0);
    saveTuning();
}

void HelmController::setReverseLimit(qreal value)
{
    tuning_.reverseLimit = std::clamp(value, 0.0, 1.0);
    saveTuning();
}

void HelmController::setRateHz(int value)
{
    rateHz_ = std::clamp(value, kRateMinHz, kRateMaxHz);
    sendTimer_.setInterval(kMsPerSecond / rateHz_);
    saveTuning();
}

QVariantList HelmController::bindings() const
{
    QVariantList list;
    for (int action = 0; action < helm::ActionCount; ++action) {
        list << QVariantMap{ { QStringLiteral("action"), action },
                             { QStringLiteral("key"), QLatin1String(bindingKey(action)) },
                             { QStringLiteral("kind"), QLatin1String(kindName(helm::actionKind(action))) },
                             { QStringLiteral("input"), bindings_[action] } };
    }
    return list;
}

QVariantList HelmController::bindingInputs() const
{
    QVariantList list;
    for (int input : bindings_) {
        list << input;
    }
    return list;
}

void HelmController::setBindingInputs(const QVariantList& inputs)
{
    if (state_ != Off || inputs.size() != helm::ActionCount) {
        return;
    }
    helm::Bindings next{};
    for (int action = 0; action < helm::ActionCount; ++action) {
        next[action] = inputs.at(action).toInt();
    }
    if (!helm::valid(next) || next == bindings_) {
        return;
    }
    bindings_ = next;
    saveBindings();
}

QVariantMap HelmController::assignBinding(int action, int input)
{
    QVariantMap reply{ { QStringLiteral("ok"), false } };
    if (state_ != Off) {
        return reply;
    }
    helm::Bindings next = bindings_;
    const helm::AssignResult result = helm::assign(next, action, input);
    if (!result.ok) {
        return reply;
    }
    reply.insert(QStringLiteral("ok"), true);
    reply.insert(QStringLiteral("swapped"), toVariantList(result.swapped));
    reply.insert(QStringLiteral("cleared"), toVariantList(result.cleared));
    if (next != bindings_) {
        bindings_ = next;
        saveBindings();
    }
    return reply;
}

void HelmController::resetBindings()
{
    if (state_ != Off || bindings_ == helm::defaultBindings()) {
        return;
    }
    bindings_ = helm::defaultBindings();
    saveBindings();
}

void HelmController::engage()
{
    if (state_ != Off) {
        return;
    }
    if (!available()) {
        notify(tr("Gamepad control needs a connected gamepad and an online autopilot"));
        return;
    }

    QStringList names{ QLatin1String(kParamOverrideTime), QLatin1String(kParamRcOptions), QLatin1String(kParamFsThrottle),
                       QLatin1String(kParamFsGcs), QLatin1String(kParamGcsSysid), QLatin1String(kParamGcsSysidHigh),
                       QLatin1String(kParamGcsSysidOld), QLatin1String(kParamMapSteering), QLatin1String(kParamMapThrottle),
                       QLatin1String(kParamSteerType) };
    for (int channel = 1; channel <= kRcChannels; ++channel) {
        names << rcParam(channel, "OPTION");
    }

    params_.clear();
    wantedParams_ = QSet<QString>(names.cbegin(), names.cend());
    paramQueue_.clear();
    checkRetried_ = false;
    checkStartMs_ = clock_.elapsed();
    lastSampleMs_ = clock_.elapsed();
    previousButtons_ = monitor_->buttons();
    forwardLimit_ = startForwardLimit_;
    steeringNeutral_ = 0.0;
    throttleNeutral_ = 0.0;
    stopReason_.clear();

    monitor_->setHelmActive(true);
    setState(Checking);
    requestParams(names);
    checkTimer_.start();
}

void HelmController::disengage()
{
    if (state_ == Checking) {
        finish(tr("Gamepad control cancelled"));
        return;
    }
    safeStop(tr("Gamepad control off"));
}

void HelmController::onVehicleChanged()
{
    const bool online = vehicle_->autopilotOnline();
    monitor_->setPresenceActive(online);

    const bool available = this->available();
    if (available != available_) {
        available_ = available;
        emit availableChanged();
    }

    if (state_ == Off) {
        return;
    }
    if (!online) {
        finish(tr("Link to the vehicle lost — gamepad control off"));
        return;
    }

    const int mode = vehicle_->pilotModeState();
    if (pendingMode_ >= 0 && mode == pendingMode_) {
        onModeResult(true);
    }
    if ((state_ == Driving || state_ == Stopping) && pendingMode_ < 0 && !modeKeepsSession(mode)) {
        finish(tr("Mode changed to %1 — gamepad control off").arg(DeviceManagerWrapper::modeNameFor(mode)));
    }
}

void HelmController::onPadConnection()
{
    const bool available = this->available();
    if (available != available_) {
        available_ = available;
        emit availableChanged();
    }

    if (!monitor_->connected()) {
        safeStop(tr("Gamepad disconnected — holding position"));
    }
}

void HelmController::onPadState()
{
    lastSampleMs_ = clock_.elapsed();

    const int buttons = monitor_->buttons();
    const int pressed = buttons & ~previousButtons_;
    previousButtons_ = buttons;
    if (state_ != Driving) {
        return;
    }

    if (pressed & helm::buttonMask(bindings_, helm::ActionHold)) {
        requestMode(kModeHold, ModeIntent::Toggle);
    }
    else if ((pressed & helm::buttonMask(bindings_, helm::ActionToggleLoiter)) && pendingMode_ < 0) {
        requestMode(vehicle_->pilotModeState() == kModeLoiter ? kModeManual : kModeLoiter, ModeIntent::Toggle);
    }

    const bool up = pressed & (1 << helm::kButtonDpadUp);
    const bool down = pressed & (1 << helm::kButtonDpadDown);
    if (up != down) {
        forwardLimit_ = std::clamp(forwardLimit_ + (up ? kLimitStep : -kLimitStep), kLimitMin, 1.0);
        emit commandChanged();
    }
}

void HelmController::onParam(const QString& name, float value)
{
    if (state_ != Checking || !wantedParams_.contains(name)) {
        return;
    }

    params_.insert(name, double(value));
    if (name == QLatin1String(kParamMapSteering) || name == QLatin1String(kParamMapThrottle)) {
        const int channel = int(value);
        if (channel >= 1 && channel <= kRcChannels) {
            const QStringList names{ rcParam(channel, "MIN"), rcParam(channel, "TRIM"), rcParam(channel, "MAX") };
            for (const QString& rc : names) {
                wantedParams_.insert(rc);
            }
            requestParams(names);
        }
    }

    if (checkComplete()) {
        finishCheck();
    }
}

void HelmController::onCommandAcked(int command, int result)
{
    if (command != kCmdDoSetMode || pendingMode_ < 0) {
        return;
    }
    if (result == kResultInProgress) {
        modeTimer_.start();
        return;
    }
    onModeResult(result == kResultAccepted);
}

void HelmController::onApplicationStateChanged(Qt::ApplicationState appState)
{
#ifdef Q_OS_ANDROID
    if (appState != Qt::ApplicationActive) {
        safeStop(tr("App in the background — holding position"));
    }
#else
    setBackground(active() && appState != Qt::ApplicationActive);
#endif
}

void HelmController::onSendTick()
{
    if (state_ != Starting && state_ != Driving && state_ != Stopping) {
        return;
    }

    if ((state_ == Starting || state_ == Driving) && clock_.elapsed() - lastSampleMs_ > kSampleTimeoutMs) {
        safeStop(tr("Gamepad stopped responding — holding position"));
        if (state_ == Off) {
            return;
        }
    }

    helm::Command command;
    if (state_ == Driving && pendingMode_ < 0 && vehicle_->pilotModeState() == kModeManual) {
        helm::Tuning tuning = tuning_;
        tuning.forwardLimit = forwardLimit_;
        command = helm::shape(padState(), bindings_, tuning);
    }
    sendCommand(command);
}

void HelmController::onCheckDeadline()
{
    if (state_ != Checking) {
        checkTimer_.stop();
        return;
    }

    const qint64 elapsed = clock_.elapsed() - checkStartMs_;
    if (!checkRetried_ && elapsed >= kCheckRetryMs) {
        checkRetried_ = true;
        QStringList missing;
        for (const QString& name : std::as_const(wantedParams_)) {
            if (!params_.contains(name)) {
                missing << name;
            }
        }
        requestParams(missing);
    }
    if (elapsed >= kCheckDeadlineMs) {
        finishCheck();
    }
}

void HelmController::pumpParams()
{
    for (int i = 0; i < kParamsPerPace && !paramQueue_.isEmpty(); ++i) {
        vehicle_->autopilotRequestParam(paramQueue_.takeFirst());
    }
    if (paramQueue_.isEmpty()) {
        paramTimer_.stop();
    }
}

void HelmController::requestParams(const QStringList& names)
{
    paramQueue_ << names;
    if (!paramTimer_.isActive()) {
        pumpParams();
        if (!paramQueue_.isEmpty()) {
            paramTimer_.start();
        }
    }
}

bool HelmController::checkComplete() const
{
    for (const QString& name : wantedParams_) {
        const bool alternative = name == QLatin1String(kParamGcsSysid) || name == QLatin1String(kParamGcsSysidHigh)
                                 || name == QLatin1String(kParamGcsSysidOld);
        if (!alternative && !params_.contains(name)) {
            return false;
        }
    }
    return params_.contains(QLatin1String(kParamGcsSysid)) || params_.contains(QLatin1String(kParamGcsSysidOld));
}

void HelmController::finishCheck()
{
    if (state_ != Checking) {
        return;
    }
    checkTimer_.stop();
    paramTimer_.stop();
    paramQueue_.clear();

    auto has = [this](const char* name) { return params_.contains(QLatin1String(name)); };
    auto value = [this](const char* name) { return params_.value(QLatin1String(name)); };

    QStringList refusals;
    QStringList notes;

    for (const char* required : { kParamOverrideTime, kParamRcOptions, kParamFsThrottle, kParamFsGcs }) {
        if (!has(required)) {
            refusals << tr("%1: no answer from the vehicle").arg(QLatin1String(required));
        }
    }
    if (has(kParamOverrideTime) && value(kParamOverrideTime) == 0.0) {
        refusals << tr("RC_OVERRIDE_TIME is 0: the vehicle ignores gamepad control");
    }
    if (has(kParamRcOptions) && (int(value(kParamRcOptions)) & kRcOptionsIgnoreOverrides)) {
        refusals << tr("RC_OPTIONS bit 1 is set: the vehicle ignores gamepad control");
    }
    if (has(kParamFsThrottle) && has(kParamFsGcs) && int(value(kParamFsThrottle)) == 0 && int(value(kParamFsGcs)) == 0) {
        refusals << tr("FS_THR_ENABLE and FS_GCS_ENABLE are both 0: after a link loss the boat would keep the last command");
    }

    if (has(kParamGcsSysid)) {
        const int low = int(value(kParamGcsSysid));
        const int high = has(kParamGcsSysidHigh) ? int(value(kParamGcsSysidHigh)) : 0;
        const bool accepted = high <= low ? low == kGcsSystemId : (kGcsSystemId >= low && kGcsSystemId <= high);
        if (!accepted) {
            refusals << tr("MAV_GCS_SYSID does not include %1: the vehicle ignores gamepad control").arg(kGcsSystemId);
        }
    }
    else if (has(kParamGcsSysidOld)) {
        if (int(value(kParamGcsSysidOld)) != kGcsSystemId) {
            refusals << tr("SYSID_MYGCS is not %1: the vehicle ignores gamepad control").arg(kGcsSystemId);
        }
    }
    else {
        notes << tr("GCS system id: no answer from the vehicle");
    }

    bool overrideSwitch = false;
    for (int channel = 1; channel <= kRcChannels; ++channel) {
        const QString name = rcParam(channel, "OPTION");
        if (params_.contains(name) && int(params_.value(name)) == kAuxRcOverrideEnable) {
            overrideSwitch = true;
        }
    }
    notes << (overrideSwitch ? tr("Transmitter switch RC_OVERRIDE_ENABLE: set up")
                             : tr("Transmitter switch RC_OVERRIDE_ENABLE: not set up"));
    if (has(kParamSteerType)) {
        notes << tr("PILOT_STEER_TYPE: %1").arg(int(value(kParamSteerType)));
    }

    auto neutralFor = [this](const char* mapName) {
        const int channel = int(params_.value(QLatin1String(mapName), 0.0));
        const QString min = rcParam(channel, "MIN");
        const QString trim = rcParam(channel, "TRIM");
        const QString max = rcParam(channel, "MAX");
        if (!params_.contains(min) || !params_.contains(trim) || !params_.contains(max)) {
            return 0.0;
        }
        return helm::neutralFromRc(params_.value(min), params_.value(trim), params_.value(max));
    };
    steeringNeutral_ = neutralFor(kParamMapSteering);
    throttleNeutral_ = neutralFor(kParamMapThrottle);

    if (bindings_[helm::ActionThrottleForward] == helm::kNoInput || bindings_[helm::ActionSteering] == helm::kNoInput) {
        refusals << tr("Gamepad layout: forward throttle or steering has no input");
    }
    if (!monitor_->connected()) {
        refusals << tr("Gamepad disconnected");
    }
    else if (!helm::throttleReleased(padState(), bindings_, tuning_)) {
        refusals << tr("Release the throttle");
    }

    vehicleStatus_ = (refusals + notes).join(QLatin1Char('\n'));
    emit vehicleStatusChanged();

    if (!refusals.isEmpty()) {
        finish(tr("Gamepad control refused: %1").arg(refusals.join(QStringLiteral("; "))));
        return;
    }

    setState(Starting);
    sendTimer_.start();
    onSendTick();
    if (vehicle_->pilotModeState() == kModeManual) {
        setState(Driving);
    }
    else {
        requestMode(kModeManual, ModeIntent::Engage);
    }
}

void HelmController::requestMode(int mode, ModeIntent intent)
{
    pendingMode_ = mode;
    modeIntent_ = intent;
    modeTimer_.start();
    vehicle_->autopilotSetMode(mode);
}

void HelmController::onModeResult(bool ok)
{
    const ModeIntent intent = modeIntent_;
    const int mode = pendingMode_;
    pendingMode_ = -1;
    modeIntent_ = ModeIntent::None;
    modeTimer_.stop();

    switch (intent) {
    case ModeIntent::None:
        break;
    case ModeIntent::Engage:
        if (ok) {
            setState(Driving);
        }
        else {
            finish(tr("The vehicle refused Manual — gamepad control off"));
        }
        break;
    case ModeIntent::Toggle:
        if (!ok) {
            notify(tr("The vehicle refused %1").arg(DeviceManagerWrapper::modeNameFor(mode)));
        }
        break;
    case ModeIntent::StopLoiter:
        if (ok) {
            finish(stopReason_);
        }
        else {
            requestMode(kModeHold, ModeIntent::StopHold);
        }
        break;
    case ModeIntent::StopHold:
        finish(ok ? stopReason_ : tr("%1. The vehicle refused Loiter and Hold").arg(stopReason_));
        break;
    }
}

void HelmController::safeStop(const QString& reason)
{
    if (state_ == Off || state_ == Stopping) {
        return;
    }
    if (state_ == Checking) {
        finish(reason);
        return;
    }

    stopReason_ = reason;
    setState(Stopping);
    sendCommand(helm::Command{});
    if (!vehicle_->autopilotOnline() || vehicle_->pilotModeState() == kModeLoiter) {
        finish(reason);
        return;
    }
    requestMode(kModeLoiter, ModeIntent::StopLoiter);
}

void HelmController::finish(const QString& reason)
{
    sendTimer_.stop();
    paramTimer_.stop();
    checkTimer_.stop();
    modeTimer_.stop();
    paramQueue_.clear();
    pendingMode_ = -1;
    modeIntent_ = ModeIntent::None;
    monitor_->setHelmActive(false);
    if (command_.steering != 0.0 || command_.throttle != 0.0) {
        command_ = helm::Command{};
        emit commandChanged();
    }
    setBackground(false);
    const bool wasActive = state_ != Off;
    setState(Off);
    if (wasActive && !reason.isEmpty()) {
        notify(reason);
    }
}

void HelmController::setState(State state)
{
    if (state_ == state) {
        return;
    }
    state_ = state;
    emit stateChanged();
}

void HelmController::setBackground(bool background)
{
    if (background_ == background) {
        return;
    }
    background_ = background;
    emit backgroundChanged();
}

void HelmController::sendCommand(const helm::Command& command)
{
    if (command.steering != command_.steering || command.throttle != command_.throttle) {
        command_ = command;
        emit commandChanged();
    }
    vehicle_->autopilotManualControl(helm::toAxis(command.steering, steeringNeutral_),
                                     helm::toAxis(command.throttle, throttleNeutral_));
}

void HelmController::notify(const QString& text)
{
    if (notifications_) {
        notifications_->info(text);
    }
}

void HelmController::loadTuning()
{
    QSettings settings(QStringLiteral("KOGGER"), QStringLiteral("KoggerApp"));
    settings.beginGroup(QLatin1String(kSettingsGroup));
    const helm::Tuning defaults;
    tuning_.stickDeadzone = std::clamp(settings.value(QStringLiteral("stickDeadzone"), defaults.stickDeadzone).toDouble(), 0.0, kDeadzoneMax);
    tuning_.triggerDeadzone = std::clamp(settings.value(QStringLiteral("triggerDeadzone"), defaults.triggerDeadzone).toDouble(), 0.0, kDeadzoneMax);
    tuning_.throttleExpo = std::clamp(settings.value(QStringLiteral("throttleExpo"), defaults.throttleExpo).toDouble(), 0.0, 1.0);
    tuning_.reverseLimit = std::clamp(settings.value(QStringLiteral("reverseLimit"), defaults.reverseLimit).toDouble(), 0.0, 1.0);
    startForwardLimit_ = std::clamp(settings.value(QStringLiteral("forwardLimit"), defaults.forwardLimit).toDouble(), kLimitMin, 1.0);
    rateHz_ = std::clamp(settings.value(QStringLiteral("rateHz"), rateHz_).toInt(), kRateMinHz, kRateMaxHz);
    settings.endGroup();
}

void HelmController::saveTuning()
{
    QSettings settings(QStringLiteral("KOGGER"), QStringLiteral("KoggerApp"));
    settings.beginGroup(QLatin1String(kSettingsGroup));
    settings.setValue(QStringLiteral("stickDeadzone"), tuning_.stickDeadzone);
    settings.setValue(QStringLiteral("triggerDeadzone"), tuning_.triggerDeadzone);
    settings.setValue(QStringLiteral("throttleExpo"), tuning_.throttleExpo);
    settings.setValue(QStringLiteral("reverseLimit"), tuning_.reverseLimit);
    settings.setValue(QStringLiteral("forwardLimit"), startForwardLimit_);
    settings.setValue(QStringLiteral("rateHz"), rateHz_);
    settings.endGroup();
    emit tuningChanged();
}

void HelmController::loadBindings()
{
    QSettings settings(QStringLiteral("KOGGER"), QStringLiteral("KoggerApp"));
    settings.beginGroup(QLatin1String(kSettingsGroup));
    settings.beginGroup(QLatin1String(kBindingsGroup));
    helm::Bindings loaded = helm::defaultBindings();
    for (int action = 0; action < helm::ActionCount; ++action) {
        loaded[action] = settings.value(QLatin1String(bindingKey(action)), loaded[action]).toInt();
    }
    settings.endGroup();
    settings.endGroup();
    bindings_ = helm::valid(loaded) ? loaded : helm::defaultBindings();
}

void HelmController::saveBindings()
{
    QSettings settings(QStringLiteral("KOGGER"), QStringLiteral("KoggerApp"));
    settings.beginGroup(QLatin1String(kSettingsGroup));
    settings.beginGroup(QLatin1String(kBindingsGroup));
    for (int action = 0; action < helm::ActionCount; ++action) {
        settings.setValue(QLatin1String(bindingKey(action)), bindings_[action]);
    }
    settings.endGroup();
    settings.endGroup();
    emit bindingsChanged();
}

helm::PadState HelmController::padState() const
{
    helm::PadState pad;
    pad.buttons = monitor_->buttons();
    pad.leftX = monitor_->leftX();
    pad.leftY = monitor_->leftY();
    pad.rightX = monitor_->rightX();
    pad.rightY = monitor_->rightY();
    pad.leftTrigger = monitor_->leftTrigger();
    pad.rightTrigger = monitor_->rightTrigger();
    return pad;
}
