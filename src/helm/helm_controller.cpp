#include "helm_controller.h"

#include <QGuiApplication>
#include <QSettings>

#include "device_manager_wrapper.h"
#include "gamepad_monitor.h"

namespace {

constexpr int kSeverityWarning = 4;
constexpr int kSeverityInfo = 6;
constexpr int kModeManual = 0;
constexpr int kModeAcro = 1;
constexpr int kModeHold = 4;
constexpr int kModeLoiter = 5;
constexpr int kModeAuto = 10;
constexpr int kModeRtl = 11;
constexpr int kCmdDoSetMode = 176;
constexpr int kCmdRequestMessage = 512;
constexpr int kMsgRcChannels = 65;
constexpr int kFsGcsStopKeepAuto = 2;
constexpr int kWriteDeadlineMs = 1500;
constexpr int kResultAccepted = 0;
constexpr int kResultInProgress = 5;

constexpr qint64 kSampleTimeoutMs = 250;
constexpr int kModeTimeoutMs = 3000;
constexpr int kParamPaceMs = 100;
constexpr int kParamsPerPace = 5;
constexpr int kCheckPollMs = 100;
constexpr qint64 kCheckRetryMs = 2000;
constexpr qint64 kCheckDeadlineMs = 4000;

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
const char kPadsGroup[] = "pads";
const char kDefaultFamily[] = "generic";
const char kLastFamilyKey[] = "lastFamily";
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
    case helm::ActionManual:
        return "manual";
    case helm::ActionHold:
        return "hold";
    case helm::ActionLoiter:
        return "loiter";
    case helm::ActionAuto:
        return "auto";
    case helm::ActionCourseHold:
        return "courseHold";
    case helm::ActionRtl:
        return "rtl";
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
    return mode == kModeManual || mode == kModeAcro || mode == kModeHold || mode == kModeLoiter;
}

bool modeTakesPad(int mode)
{
    return mode == kModeManual || mode == kModeAcro;
}

int modeBit(int mode)
{
    return (mode >= 0 && mode < 31) ? (1 << mode) : 0;
}

}

HelmController::HelmController(GamepadMonitor* monitor, DeviceManagerWrapper* vehicle, QObject* parent)
    : QObject(parent),
      monitor_(monitor),
      vehicle_(vehicle),
      sendTimer_(this),
      paramTimer_(this),
      checkTimer_(this),
      modeTimer_(this),
      writeTimer_(this)
{
    {
        QSettings settings(QStringLiteral("KOGGER"), QStringLiteral("KoggerApp"));
        const QString last = settings.value(QStringLiteral("%1/%2").arg(QLatin1String(kSettingsGroup), QLatin1String(kLastFamilyKey)),
                                            QLatin1String(kDefaultFamily)).toString();
        padFamily_ = monitor_->family().isEmpty() ? last : monitor_->family();
    }
    loadTuning();
    loadProfile();
    clock_.start();

    sendTimer_.setTimerType(Qt::PreciseTimer);
    sendTimer_.setInterval(kMsPerSecond / rateHz_);
    paramTimer_.setInterval(kParamPaceMs);
    checkTimer_.setInterval(kCheckPollMs);
    modeTimer_.setSingleShot(true);
    modeTimer_.setInterval(kModeTimeoutMs);
    writeTimer_.setSingleShot(true);
    writeTimer_.setInterval(kWriteDeadlineMs);

    connect(&sendTimer_, &QTimer::timeout, this, &HelmController::onSendTick);
    connect(&paramTimer_, &QTimer::timeout, this, &HelmController::pumpParams);
    connect(&checkTimer_, &QTimer::timeout, this, &HelmController::onCheckDeadline);
    connect(&modeTimer_, &QTimer::timeout, this, [this] { onModeResult(false); });
    connect(&writeTimer_, &QTimer::timeout, this, &HelmController::onWriteDeadline);

    connect(monitor_, &GamepadMonitor::connectionChanged, this, &HelmController::onPadConnection);
    connect(monitor_, &GamepadMonitor::stateChanged, this, &HelmController::onPadState);
    connect(vehicle_, &DeviceManagerWrapper::vruChanged, this, &HelmController::onVehicleChanged);
    connect(vehicle_, &DeviceManagerWrapper::autopilotParamReceived, this, &HelmController::onParam);
    connect(vehicle_, &DeviceManagerWrapper::autopilotRcChannelCount, this, &HelmController::onRcChannelCount);
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
    saveProfile();
    emit tuningChanged();
}

void HelmController::setTriggerDeadzone(qreal value)
{
    tuning_.triggerDeadzone = std::clamp(value, 0.0, kDeadzoneMax);
    saveProfile();
    emit tuningChanged();
}

void HelmController::setThrottleExpo(qreal value)
{
    tuning_.throttleExpo = std::clamp(value, 0.0, 1.0);
    saveTuning();
}

void HelmController::setForwardLimit(qreal value)
{
    tuning_.forwardLimit = std::clamp(value, 0.0, 1.0);
    saveTuning();
}

void HelmController::setReverseLimit(qreal value)
{
    tuning_.reverseLimit = std::clamp(value, 0.0, 1.0);
    saveTuning();
}

void HelmController::resetTuning()
{
    const helm::Tuning defaults;
    tuning_.forwardLimit = defaults.forwardLimit;
    tuning_.reverseLimit = defaults.reverseLimit;
    tuning_.steeringLimit = defaults.steeringLimit;
    tuning_.throttleExpo = defaults.throttleExpo;
    setRateHz(kDefaultRateHz);
}

void HelmController::setSteeringLimit(qreal value)
{
    tuning_.steeringLimit = std::clamp(value, 0.0, 1.0);
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
    if (!layoutEditable() || inputs.size() != helm::ActionCount) {
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
    if (!layoutEditable()) {
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
    if (!layoutEditable() || bindings_ == helm::defaultBindings()) {
        return;
    }
    bindings_ = helm::defaultBindings();
    saveBindings();
}

void HelmController::resetPadSettings()
{
    if (!layoutEditable()) {
        return;
    }
    const helm::Tuning defaults;
    tuning_.stickDeadzone = defaults.stickDeadzone;
    tuning_.triggerDeadzone = defaults.triggerDeadzone;
    bindings_ = helm::defaultBindings();
    saveProfile();
    emit tuningChanged();
    emit bindingsChanged();
}

void HelmController::engage()
{
    if (state_ != Off || confirmationBusy_) {
        return;
    }
    setConfirmation(ConfirmNone, false);
    if (configuring_) {
        notify(tr("Close the gamepad settings to take control"), kSeverityWarning);
        return;
    }
    if (!available()) {
        notify(tr("Gamepad control needs a connected gamepad and an online autopilot"), kSeverityWarning);
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
    bumpersSpent_ = (previousButtons_ & helm::kArmComboMask) != 0;
    steeringNeutral_ = 0.0;
    throttleNeutral_ = 0.0;
    stopReason_.clear();
    rcChannelCount_ = -1;
    modeBeforeEngage_ = vehicle_->pilotModeState();
    seenMode_ = modeBeforeEngage_;
    padMissionModes_ = 0;

    monitor_->setHelmActive(true);
    setState(Checking);
    requestParams(names);
    vehicle_->autopilotRequestMessage(kMsgRcChannels);
    checkTimer_.start();
}

void HelmController::setConfiguring(bool configuring)
{
    if (configuring_ == configuring) {
        return;
    }
    configuring_ = configuring;
    emit configuringChanged();
    emit layoutEditableChanged();
    if (configuring_) {
        cancelConfirmation();
        if (state_ != Off && state_ != Driving) {
            safeStop(tr("Gamepad settings opened — gamepad control off"),
                     tr("Gamepad settings opened — the transmitter has the boat"));
        }
    }
    else if (state_ == Driving) {
        holdUntilReleased_ = true;
        if (!helm::throttleReleased(padState(), bindings_, tuning_)) {
            notify(tr("Release the throttle to drive again"), kSeverityWarning);
        }
    }
}

void HelmController::confirm()
{
    if (confirmation_ == ConfirmTransmitter) {
        transmitterConfirmed_ = true;
        setConfirmation(ConfirmNone, false);
        engage();
        return;
    }
    if (confirmation_ != ConfirmLinkLossStop || confirmationBusy_) {
        return;
    }
    if (!vehicle_->autopilotOnline()) {
        setConfirmation(ConfirmNone, false);
        return;
    }
    setConfirmation(ConfirmLinkLossStop, true);
    writeRetried_ = false;
    vehicle_->autopilotSetParam(QLatin1String(kParamFsGcs), float(kFsGcsStopKeepAuto));
    vehicle_->autopilotRequestParam(QLatin1String(kParamFsGcs));
    writeTimer_.start();
}

void HelmController::cancelConfirmation()
{
    writeTimer_.stop();
    setConfirmation(ConfirmNone, false);
}

void HelmController::onWriteDeadline()
{
    if (!confirmationBusy_) {
        return;
    }
    if (!writeRetried_) {
        writeRetried_ = true;
        vehicle_->autopilotRequestParam(QLatin1String(kParamFsGcs));
        writeTimer_.start();
        return;
    }
    setConfirmation(ConfirmNone, false);
    notify(tr("The boat did not confirm the setting — gamepad control not started"), kSeverityWarning);
}

void HelmController::setConfirmation(int confirmation, bool busy)
{
    if (confirmation_ == confirmation && confirmationBusy_ == busy) {
        return;
    }
    confirmation_ = confirmation;
    confirmationBusy_ = busy;
    emit confirmationChanged();
}

void HelmController::onRcChannelCount(int count)
{
    if (state_ == Off) {
        return;
    }
    rcChannelCount_ = count;
    if (state_ == Checking && checkComplete()) {
        finishCheck();
    }
}

void HelmController::disengage()
{
    if (state_ == Checking) {
        finish(tr("Gamepad control cancelled"));
        return;
    }
    safeStop(tr("Gamepad control off"), tr("Gamepad control off — the transmitter has the boat"));
}

void HelmController::onVehicleChanged()
{
    const bool online = vehicle_->autopilotOnline();
    monitor_->setPresenceActive(online);
    if (!online) {
        cancelConfirmation();
    }

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
    if (mode != seenMode_) {
        seenMode_ = mode;
        if (modeKeepsSession(mode)) {
            padMissionModes_ = 0;
        }
    }
    const bool keeps = modeKeepsSession(mode) || (modeBit(mode) & padMissionModes_) != 0;
    if ((state_ == Driving || state_ == Stopping) && pendingMode_ < 0 && !keeps) {
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
        cancelConfirmation();
        safeStop(tr("Gamepad disconnected — holding position"), tr("Gamepad disconnected — the transmitter has the boat"));
        return;
    }
    if (state_ == Off && monitor_->family() != padFamily_) {
        padFamily_ = monitor_->family();
        QSettings(QStringLiteral("KOGGER"), QStringLiteral("KoggerApp"))
            .setValue(QStringLiteral("%1/%2").arg(QLatin1String(kSettingsGroup), QLatin1String(kLastFamilyKey)), padFamily_);
        loadProfile();
        emit padFamilyChanged();
        emit tuningChanged();
        emit bindingsChanged();
    }
}

void HelmController::onPadState()
{
    lastSampleMs_ = clock_.elapsed();

    const int buttons = monitor_->buttons();
    const int previous = previousButtons_;
    previousButtons_ = buttons;
    const bool live = state_ == Driving && !configuring_;
    if (!live && (buttons & helm::kArmComboMask) != 0) {
        bumpersSpent_ = true;
    }
    const int fired = helm::firedButtons(previous, buttons, bumpersSpent_);
    if (!live) {
        return;
    }

    if ((buttons & helm::kArmComboMask) == helm::kArmComboMask && (previous & helm::kArmComboMask) != helm::kArmComboMask) {
        toggleArm();
    }

    auto firedFor = [this, fired](helm::Action action) { return (fired & helm::buttonMask(bindings_, action)) != 0; };
    if (firedFor(helm::ActionHold)) {
        requestPadMode(kModeHold);
    }
    else if (pendingMode_ < 0) {
        if (firedFor(helm::ActionManual)) {
            requestPadMode(kModeManual);
        }
        else if (firedFor(helm::ActionCourseHold)) {
            requestPadMode(vehicle_->pilotModeState() == kModeAcro ? kModeManual : kModeAcro);
        }
        else if (firedFor(helm::ActionLoiter)) {
            requestPadMode(kModeLoiter);
        }
        else if (firedFor(helm::ActionAuto)) {
            requestPadMode(kModeAuto);
        }
        else if (firedFor(helm::ActionRtl)) {
            requestPadMode(kModeRtl);
        }
    }
}

bool HelmController::throttleHeld() const
{
    return state_ != Off && !helm::throttleReleased(padState(), bindings_, tuning_);
}

void HelmController::requestPadMode(int mode)
{
    if (vehicle_->pilotModeState() == mode) {
        return;
    }
    if (!modeKeepsSession(mode)) {
        padMissionModes_ |= modeBit(mode);
    }
    requestMode(mode, ModeIntent::Toggle);
}

void HelmController::toggleArm()
{
    if (vehicle_->pilotArmState() > 0) {
        vehicle_->autopilotArm(false);
        notify(tr("Disarm from the gamepad"), kSeverityInfo);
        return;
    }
    if (throttleHeld()) {
        notify(tr("Release the throttle before arming"), kSeverityWarning);
        return;
    }
    vehicle_->autopilotArm(true);
    notify(tr("Arm from the gamepad"), kSeverityInfo);
}

void HelmController::onParam(const QString& name, float value)
{
    if (confirmationBusy_ && name == QLatin1String(kParamFsGcs) && int(value) == kFsGcsStopKeepAuto) {
        writeTimer_.stop();
        setConfirmation(ConfirmNone, false);
        notify(tr("The boat now stops when the link drops"), kSeverityInfo);
        engage();
        return;
    }
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
    if (command == kCmdRequestMessage && state_ == Checking && result != kResultAccepted && rcChannelCount_ < 0) {
        rcChannelCount_ = 0;
        if (checkComplete()) {
            finishCheck();
        }
        return;
    }
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
        safeStop(tr("App in the background — holding position"), tr("App in the background — the transmitter has the boat"));
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
        safeStop(tr("Gamepad stopped responding — holding position"), tr("Gamepad stopped responding — the transmitter has the boat"));
        if (state_ == Off) {
            return;
        }
    }

    helm::Command command;
    if (holdUntilReleased_ && !configuring_ && helm::throttleReleased(padState(), bindings_, tuning_)) {
        holdUntilReleased_ = false;
    }
    if (state_ == Driving && !configuring_ && !holdUntilReleased_ && pendingMode_ < 0 && modeTakesPad(vehicle_->pilotModeState())) {
        command = helm::shape(padState(), bindings_, tuning_);
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
    const bool failsafesOff = int(params_.value(QLatin1String(kParamFsThrottle), 1.0)) == 0
                              && int(params_.value(QLatin1String(kParamFsGcs), 1.0)) == 0;
    if (failsafesOff && rcChannelCount_ < 0) {
        return false;
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
    const bool failsafesOff = has(kParamFsThrottle) && has(kParamFsGcs)
                              && int(value(kParamFsThrottle)) == 0 && int(value(kParamFsGcs)) == 0;
    overrideSeconds_ = has(kParamOverrideTime) ? value(kParamOverrideTime) : 0.0;
    const bool transmitterTakesOver = rcChannelCount_ > 0 && overrideSeconds_ > 0.0;
    if (failsafesOff) {
        notes << tr("Stop on link loss (FS_THR_ENABLE, FS_GCS_ENABLE): off");
        notes << (rcChannelCount_ > 0 ? tr("RC receiver: %1 channels").arg(rcChannelCount_) : tr("RC receiver: none"));
    }
    if (overrideSeconds_ < 0.0) {
        notes << tr("RC_OVERRIDE_TIME is negative: the transmitter never takes over from the gamepad");
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
    if (!notes.isEmpty()) {
        notify(tr("Vehicle check: %1").arg(notes.join(QStringLiteral("; "))), kSeverityInfo);
    }

    if (!refusals.isEmpty()) {
        finish(tr("Gamepad control refused: %1").arg(refusals.join(QStringLiteral("; "))));
        return;
    }

    if (failsafesOff && !(transmitterTakesOver && transmitterConfirmed_)) {
        finish(QString());
        setConfirmation(transmitterTakesOver ? ConfirmTransmitter : ConfirmLinkLossStop, false);
        return;
    }

    setState(Starting);
    sendTimer_.start();
    onSendTick();
    if (modeTakesPad(vehicle_->pilotModeState())) {
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
            if (mode != vehicle_->pilotModeState()) {
                padMissionModes_ &= ~modeBit(mode);
            }
            notify(mode == kModeLoiter ? tr("The boat refused Loiter — no position (GPS/EKF)")
                                       : tr("The vehicle refused %1").arg(DeviceManagerWrapper::modeNameFor(mode)),
                   kSeverityWarning);
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

bool HelmController::handsBackToTransmitter() const
{
    return vehicle_->autopilotOnline() && modeTakesPad(modeBeforeEngage_) && vehicle_->pilotModeState() == modeBeforeEngage_
           && rcChannelCount_ > 0 && overrideSeconds_ > 0.0;
}

void HelmController::safeStop(const QString& reason, const QString& handBackReason)
{
    if (state_ == Off || state_ == Stopping) {
        return;
    }
    if (state_ == Checking) {
        finish(reason);
        return;
    }
    if (handsBackToTransmitter()) {
        vehicle_->autopilotReleaseManualControl();
        finish(handBackReason);
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
        notify(reason, kSeverityWarning);
    }
}

void HelmController::setState(State state)
{
    if (state_ == state) {
        return;
    }
    state_ = state;
    if (state_ != Driving) {
        holdUntilReleased_ = false;
    }
    emit stateChanged();
    emit layoutEditableChanged();
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

void HelmController::notify(const QString& text, int severity)
{
    vehicle_->logAutopilotEvent(severity, text);
}

void HelmController::loadTuning()
{
    QSettings settings(QStringLiteral("KOGGER"), QStringLiteral("KoggerApp"));
    settings.beginGroup(QLatin1String(kSettingsGroup));
    const helm::Tuning defaults;
    tuning_.throttleExpo = std::clamp(settings.value(QStringLiteral("throttleExpo"), defaults.throttleExpo).toDouble(), 0.0, 1.0);
    tuning_.reverseLimit = std::clamp(settings.value(QStringLiteral("reverseLimit"), defaults.reverseLimit).toDouble(), 0.0, 1.0);
    tuning_.steeringLimit = std::clamp(settings.value(QStringLiteral("steeringLimit"), defaults.steeringLimit).toDouble(), 0.0, 1.0);
    tuning_.forwardLimit = std::clamp(settings.value(QStringLiteral("forwardLimit"), defaults.forwardLimit).toDouble(), 0.0, 1.0);
    rateHz_ = std::clamp(settings.value(QStringLiteral("rateHz"), rateHz_).toInt(), kRateMinHz, kRateMaxHz);
    settings.endGroup();
}

void HelmController::saveTuning()
{
    QSettings settings(QStringLiteral("KOGGER"), QStringLiteral("KoggerApp"));
    settings.beginGroup(QLatin1String(kSettingsGroup));
    settings.setValue(QStringLiteral("throttleExpo"), tuning_.throttleExpo);
    settings.setValue(QStringLiteral("reverseLimit"), tuning_.reverseLimit);
    settings.setValue(QStringLiteral("steeringLimit"), tuning_.steeringLimit);
    settings.setValue(QStringLiteral("forwardLimit"), tuning_.forwardLimit);
    settings.setValue(QStringLiteral("rateHz"), rateHz_);
    settings.endGroup();
    emit tuningChanged();
}

void HelmController::loadProfile()
{
    QSettings settings(QStringLiteral("KOGGER"), QStringLiteral("KoggerApp"));
    settings.beginGroup(profileGroup());
    const helm::Tuning defaults;
    tuning_.stickDeadzone = std::clamp(settings.value(QStringLiteral("stickDeadzone"), defaults.stickDeadzone).toDouble(), 0.0, kDeadzoneMax);
    tuning_.triggerDeadzone = std::clamp(settings.value(QStringLiteral("triggerDeadzone"), defaults.triggerDeadzone).toDouble(), 0.0, kDeadzoneMax);
    settings.beginGroup(QLatin1String(kBindingsGroup));
    helm::Bindings loaded = helm::defaultBindings();
    for (int action = 0; action < helm::ActionCount; ++action) {
        loaded[action] = settings.value(QLatin1String(bindingKey(action)), loaded[action]).toInt();
    }
    settings.endGroup();
    settings.endGroup();
    bindings_ = helm::valid(loaded) ? loaded : helm::defaultBindings();
}

void HelmController::saveProfile()
{
    QSettings settings(QStringLiteral("KOGGER"), QStringLiteral("KoggerApp"));
    settings.beginGroup(profileGroup());
    settings.setValue(QStringLiteral("stickDeadzone"), tuning_.stickDeadzone);
    settings.setValue(QStringLiteral("triggerDeadzone"), tuning_.triggerDeadzone);
    settings.beginGroup(QLatin1String(kBindingsGroup));
    for (int action = 0; action < helm::ActionCount; ++action) {
        settings.setValue(QLatin1String(bindingKey(action)), bindings_[action]);
    }
    settings.endGroup();
    settings.endGroup();
}

QString HelmController::profileGroup() const
{
    return QStringLiteral("%1/%2/%3").arg(QLatin1String(kSettingsGroup), QLatin1String(kPadsGroup), padFamily_);
}

void HelmController::saveBindings()
{
    saveProfile();
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
