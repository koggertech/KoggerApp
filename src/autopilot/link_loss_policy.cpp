#include "link_loss_policy.h"

#include <cmath>

#include <QVariantMap>

#include "autopilot_params.h"
#include "param_metadata.h"
#include "param_transfer.h"

namespace autopilot {

namespace {

constexpr int kMavAutopilotArdupilot = 3;
constexpr int kMavTypeGroundRover = 10;
constexpr int kMavTypeSurfaceBoat = 11;

constexpr int kModeHold = 4;
constexpr int kModeAuto = 10;
constexpr int kModeRtl = 11;

constexpr int kActionNothing = 0;
constexpr int kActionRtl = 1;
constexpr int kActionHold = 2;
constexpr int kActionSmartRtlOrRtl = 3;
constexpr int kActionSmartRtlOrHold = 4;
constexpr int kActionTerminate = 5;
constexpr int kActionLoiterOrHold = 6;

const QString kFsAction = QStringLiteral("FS_ACTION");
const QString kGcsEnable = QStringLiteral("FS_GCS_ENABLE");
const QString kGcsTimeout = QStringLiteral("FS_GCS_TIMEOUT");
const QString kFsTimeout = QStringLiteral("FS_TIMEOUT");
const QString kThrEnable = QStringLiteral("FS_THR_ENABLE");
const QString kFsOptions = QStringLiteral("FS_OPTIONS");
const QString kSrtlPoints = QStringLiteral("SRTL_POINTS");

const QStringList& paramNames()
{
    static const QStringList names{ kFsAction, kGcsEnable, kGcsTimeout, kFsTimeout, kThrEnable, kFsOptions, kSrtlPoints };
    return names;
}

} // namespace

LinkLossPolicy::LinkLossPolicy(AutopilotParams* params, QObject* parent)
    : QObject(parent)
    , params_(params)
{
    confirmTimer_.setSingleShot(true);
    connect(&confirmTimer_, &QTimer::timeout, this, &LinkLossPolicy::onConfirmTimeout);
    connect(params_, &AutopilotParams::paramChanged, this, &LinkLossPolicy::onParamChanged);
    connect(params_, &AutopilotParams::requestFinished, this, &LinkLossPolicy::onRequestFinished);
    connect(params_, &AutopilotParams::writeFailed, this, &LinkLossPolicy::onWriteFailed);
    connect(params_, &AutopilotParams::metadataChanged, this, &LinkLossPolicy::changed);
}

int LinkLossPolicy::choice() const
{
    if (!known()) {
        return ChoiceUnknown;
    }
    if (gcsEnable_ == kGcsOff) {
        return ChoiceNothing;
    }
    switch (fsAction_) {
    case kActionHold:            return ChoiceHold;
    case kActionLoiterOrHold:    return ChoiceLoiter;
    case kActionRtl:             return ChoiceRtl;
    case kActionSmartRtlOrRtl:   return ChoiceSmartRtl;
    default:                     return ChoiceOther;
    }
}

QString LinkLossPolicy::choiceLabel() const
{
    switch (choice()) {
    case ChoiceUnknown:  return {};
    case ChoiceNothing:  return tr("Nothing");
    case ChoiceHold:     return QStringLiteral("Hold");
    case ChoiceLoiter:   return QStringLiteral("Loiter");
    case ChoiceRtl:      return QStringLiteral("RTL");
    case ChoiceSmartRtl: return QStringLiteral("SmartRTL");
    default:             return actionLabel(fsAction_);
    }
}

QVariantList LinkLossPolicy::options() const
{
    const auto option = [](int choice, const QString& label, bool enabled, const QString& note) {
        return QVariantMap{ { QStringLiteral("choice"), choice },
                            { QStringLiteral("label"), label },
                            { QStringLiteral("enabled"), enabled },
                            { QStringLiteral("note"), note } };
    };
    QVariantList out;
    out.append(option(ChoiceNothing, tr("Nothing"), true, tr("The boat keeps its mode and its last command")));
    out.append(option(ChoiceHold, QStringLiteral("Hold"), true, tr("The motors stop and the boat drifts")));
    const bool loiter = offersLoiter();
    QString loiterNote;
    if (loiter || fsAction_ == kActionLoiterOrHold) {
        loiterNote = tr("The boat holds its position; without a position fix it stops (Hold)");
    } else if (params_->metadataOf(kFsAction)) {
        loiterNote = tr("Loiter on link loss needs ArduRover 4.7 or newer");
    } else {
        loiterNote = tr("Unknown whether this firmware has Loiter on link loss");
    }
    out.append(option(ChoiceLoiter, QStringLiteral("Loiter"), loiter, loiterNote));
    out.append(option(ChoiceRtl, QStringLiteral("RTL"), true,
                      tr("The boat returns to the nearest rally point or home and holds its position there")));
    const bool smartRtl = srtlPoints_ != 0;
    out.append(option(ChoiceSmartRtl, QStringLiteral("SmartRTL"), smartRtl,
                      smartRtl ? tr("The boat returns along its own track since arming and holds its position; without a track it does RTL")
                               : tr("SmartRTL is off on the boat (SRTL_POINTS = 0)")));
    return out;
}

double LinkLossPolicy::delaySeconds() const
{
    if (gcsTimeout_ < 0.0 || fsTimeout_ < 0.0) {
        return -1.0;
    }
    return gcsTimeout_ + fsTimeout_;
}

int LinkLossPolicy::outcome() const
{
    if (!available_ || !known()) {
        return OutcomeUnknown;
    }
    if (gcsEnable_ == kGcsOff) {
        return OutcomeNothing;
    }
    if (mode_ == kModeRtl || (mode_ == kModeHold && !activeInHold())) {
        return OutcomeAlready;
    }
    if (mode_ == kModeAuto && gcsEnable_ == kGcsContinueMission) {
        return OutcomeMission;
    }
    if (fsAction_ == kActionNothing) {
        return OutcomeNothing;
    }
    return OutcomeAction;
}

QString LinkLossPolicy::outcomeText() const
{
    switch (outcome()) {
    case OutcomeNothing: return QStringLiteral("→ —");
    case OutcomeMission: return QStringLiteral("→ Auto");
    case OutcomeAlready: return QStringLiteral("→ %1").arg(mode_ == kModeRtl ? QStringLiteral("RTL") : QStringLiteral("Hold"));
    case OutcomeAction:  return QStringLiteral("→ %1").arg(actionWord(fsAction_));
    default:             return QStringLiteral("…");
    }
}

bool LinkLossPolicy::apply(int choice, bool continueMission)
{
    const auto refuse = [this](const QString& reason) {
        error_ = reason;
        emit changed();
        return false;
    };
    if (!available_) {
        return refuse(tr("No autopilot"));
    }
    if (busy_) {
        return false;
    }
    if (!known()) {
        return refuse(tr("The link-loss parameters have not been read yet"));
    }
    if (choice == ChoiceLoiter && !offersLoiter()) {
        return refuse(tr("This firmware has no Loiter link-loss action"));
    }
    if (choice == ChoiceSmartRtl && srtlPoints_ == 0) {
        return refuse(tr("SmartRTL is off on the boat (SRTL_POINTS = 0)"));
    }
    const int action = actionFor(choice);
    if (choice != ChoiceNothing && choice != ChoiceOther && action < 0) {
        return false;
    }
    continuePreferred_ = continueMission;
    targetAction_ = action;
    targetGcs_ = choice == ChoiceNothing ? kGcsOff : (continueMission ? kGcsContinueMission : kGcsOn);

    bool writing = false;
    if (targetAction_ >= 0 && targetAction_ != fsAction_) {
        if (!params_->setValue(kFsAction, QString::number(targetAction_))) {
            targetAction_ = targetGcs_ = -1;
            return refuse(params_->lastError());
        }
        writing = true;
        gcsDeferred_ = targetGcs_ != gcsEnable_;
    } else if (targetGcs_ != gcsEnable_) {
        if (!writeGcsEnable()) {
            return false;
        }
        writing = true;
    }
    error_.clear();
    busy_ = writing;
    if (writing) {
        startConfirmTimer();
    } else {
        targetAction_ = targetGcs_ = -1;
    }
    emit changed();
    return true;
}

void LinkLossPolicy::reread()
{
    if (!available_) {
        return;
    }
    reading_ = true;
    error_.clear();
    emit changed();
    params_->request(paramNames());
}

void LinkLossPolicy::setVehicle(bool online, int systemId, int componentId, int autopilotKind, int vehicleType, int mode,
                                bool proxyTraffic)
{
    const bool available = online && autopilotKind == kMavAutopilotArdupilot
                           && (vehicleType == kMavTypeGroundRover || vehicleType == kMavTypeSurfaceBoat);
    const bool otherVehicle = available && available_ && (systemId != systemId_ || componentId != componentId_);
    const bool same = available == available_ && mode == mode_ && proxyTraffic == proxyTraffic_;
    mode_ = mode;
    proxyTraffic_ = proxyTraffic;
    if (online) {
        systemId_ = systemId;
        componentId_ = componentId;
    }
    if (otherVehicle) {
        busy_ = false;
        gcsDeferred_ = false;
        confirmTimer_.stop();
        targetAction_ = -1;
        targetGcs_ = -1;
        clearValues();
        reread();
        return;
    }
    if (available == available_) {
        if (!same) {
            emit changed();
        }
        return;
    }
    available_ = available;
    if (available_) {
        clearValues();
        reread();
        return;
    }
    if (busy_) {
        finishBusy(tr("The link was lost before the boat confirmed the change"));
    }
    reading_ = false;
    clearValues();
    emit changed();
}

void LinkLossPolicy::onParamChanged(const QString& name)
{
    if (!available_ || !paramNames().contains(name)) {
        return;
    }
    readValue(name);
    checkConfirmed();
    emit changed();
}

void LinkLossPolicy::onRequestFinished(const QStringList& missing)
{
    Q_UNUSED(missing)
    if (!reading_) {
        return;
    }
    reading_ = false;
    if (available_ && !known()) {
        error_ = tr("The boat did not answer FS_GCS_ENABLE / FS_ACTION");
    }
    emit changed();
}

void LinkLossPolicy::onWriteFailed(const QString& name, const QString& actual)
{
    if (!busy_ || (name != kFsAction && name != kGcsEnable)) {
        return;
    }
    finishBusy(tr("Not confirmed: the boat reports %1 = %2").arg(name, actual));
}

void LinkLossPolicy::onConfirmTimeout()
{
    if (busy_) {
        finishBusy(tr("Not confirmed by the boat"));
    }
}

void LinkLossPolicy::readValue(const QString& name)
{
    float v = 0.0f;
    if (!params_->value(name, &v)) {
        return;
    }
    const int whole = int(std::lround(v));
    if (name == kFsAction) {
        fsAction_ = whole;
    } else if (name == kGcsEnable) {
        gcsEnable_ = whole;
    } else if (name == kThrEnable) {
        thrEnable_ = whole;
    } else if (name == kFsOptions) {
        fsOptions_ = whole;
    } else if (name == kSrtlPoints) {
        srtlPoints_ = whole;
    } else if (name == kGcsTimeout) {
        gcsTimeout_ = double(v);
    } else if (name == kFsTimeout) {
        fsTimeout_ = double(v);
    }
}

void LinkLossPolicy::checkConfirmed()
{
    if (!busy_) {
        return;
    }
    if (gcsDeferred_) {
        if (fsAction_ != targetAction_) {
            return;
        }
        gcsDeferred_ = false;
        if (gcsEnable_ != targetGcs_) {
            if (writeGcsEnable()) {
                startConfirmTimer();
            }
            return;
        }
    }
    if ((targetAction_ < 0 || fsAction_ == targetAction_) && gcsEnable_ == targetGcs_) {
        finishBusy(QString());
    }
}

bool LinkLossPolicy::writeGcsEnable()
{
    if (params_->setValue(kGcsEnable, QString::number(targetGcs_))) {
        return true;
    }
    finishBusy(params_->lastError());
    return false;
}

void LinkLossPolicy::finishBusy(const QString& error)
{
    busy_ = false;
    gcsDeferred_ = false;
    confirmTimer_.stop();
    targetAction_ = -1;
    targetGcs_ = -1;
    error_ = error;
    emit changed();
}

void LinkLossPolicy::startConfirmTimer()
{
    const int perWriteMs = ParamTransfer::kWriteAttempts * ParamTransfer::kWriteTimeoutMs;
    confirmTimer_.start(params_->pendingWrites() * perWriteMs + kConfirmMarginMs);
}

void LinkLossPolicy::clearValues()
{
    fsAction_ = -1;
    gcsEnable_ = -1;
    thrEnable_ = -1;
    fsOptions_ = -1;
    srtlPoints_ = -1;
    gcsTimeout_ = -1.0;
    fsTimeout_ = -1.0;
    error_.clear();
}

bool LinkLossPolicy::offersLoiter() const
{
    const ParamInfo* info = params_->metadataOf(kFsAction);
    if (!info) {
        return false;
    }
    for (const auto& v : info->values) {
        if (int(std::lround(v.first)) == kActionLoiterOrHold) {
            return true;
        }
    }
    return false;
}

int LinkLossPolicy::actionFor(int choice)
{
    switch (choice) {
    case ChoiceHold:     return kActionHold;
    case ChoiceLoiter:   return kActionLoiterOrHold;
    case ChoiceRtl:      return kActionRtl;
    case ChoiceSmartRtl: return kActionSmartRtlOrRtl;
    default:             return -1;
    }
}

QString LinkLossPolicy::actionLabel(int fsAction) const
{
    const ParamInfo* info = params_->metadataOf(kFsAction);
    const QString label = info ? ParamMetadata::valueLabel(*info, float(fsAction)) : QString();
    return label.isEmpty() ? QStringLiteral("FS_ACTION = %1").arg(fsAction) : label;
}

QString LinkLossPolicy::actionWord(int fsAction) const
{
    switch (fsAction) {
    case kActionRtl:            return QStringLiteral("RTL");
    case kActionHold:           return QStringLiteral("Hold");
    case kActionSmartRtlOrRtl:
    case kActionSmartRtlOrHold: return QStringLiteral("SmartRTL");
    case kActionTerminate:      return tr("Disarm");
    case kActionLoiterOrHold:   return QStringLiteral("Loiter");
    default:                    return actionLabel(fsAction);
    }
}

} // namespace autopilot
