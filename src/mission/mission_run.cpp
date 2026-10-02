#include "mission_run.h"

#include <algorithm>
#include <type_traits>

#include <QHash>

#include "autopilot_messages.h"
#include "mission_geometry.h"
#include "mission_plan_controller.h"
#include "mission_vehicle.h"

namespace mission {

namespace {

constexpr int kConnectReadDelayMs = 1000;
constexpr int kStaleReadDelayMs = 2000;
constexpr int kRetryDelayMs = 1000;
constexpr qint64 kMinReadIntervalMs = 10000;
constexpr int kMaxFailedReads = 3;
constexpr double kMinGroundSpeed = 0.3;
constexpr int kNavCommandFirst = 16;
constexpr int kNavCommandLast = 95;
constexpr int kRoverModeAuto = 10;
constexpr double kDistanceStep = 0.5;
constexpr double kSecondsStep = 1.0;
constexpr double kProgressStep = 0.001;
constexpr double kVehicleMoveStepM = 0.5;

struct PlanOrigin {
    RunItem::Kind kind = RunItem::Kind::Other;
    int ordinal = 0;
    int segmentCount = 0;
};

const autopilot::MissionBatch* routeOf(const autopilot::MissionBatches& batches)
{
    for (const auto& b : batches) {
        if (b.missionType == MavMissionTypeMission) {
            return &b;
        }
    }
    return nullptr;
}

bool hasType(const autopilot::MissionBatches& batches, int missionType)
{
    return std::any_of(batches.cbegin(), batches.cend(), [missionType](const autopilot::MissionBatch& b) { return b.missionType == missionType; });
}

GeoPoint positionOf(const autopilot::MissionItem& m)
{
    if (m.x == 0 && m.y == 0) {
        return GeoPoint();
    }
    return GeoPoint(double(m.x) / 1.0e7, double(m.y) / 1.0e7);
}

bool changedBy(double a, double b, double step)
{
    if (std::isfinite(a) != std::isfinite(b)) {
        return true;
    }
    return std::isfinite(a) && std::fabs(a - b) >= step;
}

} // namespace

MissionRunTracker::MissionRunTracker(MissionPlanController* plan, QObject* parent)
    : QObject(parent),
      plan_(plan)
{
    readTimer_.setSingleShot(true);
    connect(&readTimer_, &QTimer::timeout, this, &MissionRunTracker::onReadTimer);
    if (plan_) {
        connect(plan_, &MissionPlanController::planChanged, this, &MissionRunTracker::onPlanChanged);
    }
}

QString MissionRunTracker::labelFor(const RunItem& item) const
{
    if (item.seq == 0) {
        return tr("Home");
    }
    const bool hasLine = item.segment >= 0 && item.segmentCount > 0;
    switch (item.kind) {
    case RunItem::Kind::Start:
        return tr("Start point");
    case RunItem::Kind::Waypoint:
        return tr("Waypoint %1").arg(item.ordinal);
    case RunItem::Kind::Survey:
        return hasLine ? tr("Survey %1 · line %2/%3").arg(item.ordinal).arg(item.segment + 1).arg(item.segmentCount)
                       : tr("Survey %1").arg(item.ordinal);
    case RunItem::Kind::Corridor:
        return hasLine ? tr("Corridor %1 · line %2/%3").arg(item.ordinal).arg(item.segment + 1).arg(item.segmentCount)
                       : tr("Corridor %1").arg(item.ordinal);
    case RunItem::Kind::ReturnToLaunch:
        return tr("Return to launch");
    case RunItem::Kind::Hold:
        return tr("Hold position");
    case RunItem::Kind::Command:
    case RunItem::Kind::Other:
        break;
    }
    return tr("Item %1").arg(item.seq);
}

int MissionRunTracker::navIndexOf(int seq) const
{
    const int k = int(navSeqs_.indexOf(seq));
    return k >= 0 ? k + 1 : 0;
}

int MissionRunTracker::jumpSeqFor(int seq) const
{
    if (!actionable() || seq < 1 || seq >= items_.size()) {
        return -1;
    }
    if (seq > 1 && seq < items_.size() && items_.at(seq - 1).command == int(MavCmd::DoChangeSpeed)) {
        return seq - 1;
    }
    return seq;
}

int MissionRunTracker::nextTargetSeq() const
{
    if (!actionable() || finished_) {
        return -1;
    }
    const int k = navIndexOf(targetSeq_);
    return (k > 0 && k < navSeqs_.size()) ? navSeqs_.at(k) : -1;
}

QString MissionRunTracker::labelForSeq(int seq) const
{
    return (seq >= 0 && seq < items_.size()) ? labelFor(items_.at(seq)) : QString();
}

bool MissionRunTracker::openInEditor()
{
    if (!known_ || !plan_) {
        return false;
    }
    plan_->receiveVehicleMission(snapshot_);
    return true;
}

void MissionRunTracker::setReadAutomatically(bool read)
{
    if (readAutomatically_ == read) {
        return;
    }
    readAutomatically_ = read;
    emit readAutomaticallyChanged();
    if (!readAutomatically_) {
        readWanted_ = false;
        readTimer_.stop();
    } else if (telemetry_.online && (!known_ || stale_)) {
        failedReads_ = 0;
        wantRead(kRetryDelayMs);
    }
}

void MissionRunTracker::refresh()
{
    failedReads_ = 0;
    lastRead_.invalidate();
    readTimer_.stop();
    wantRead(0);
}

void MissionRunTracker::setTelemetry(const RunTelemetry& telemetry)
{
    const bool wasOnline = telemetry_.online;
    const bool wasReading = reading();
    const bool wasTransfer = telemetry_.transferActive;
    telemetry_ = telemetry;
    if (!telemetry_.online) {
        if (wasOnline) {
            readWanted_ = false;
            readTimer_.stop();
            failedReads_ = 0;
            hasPendingUpload_ = false;
            clearSnapshot();
        }
        if (wasReading != reading()) {
            emit readingChanged();
        }
        return;
    }
    if (!wasOnline) {
        failedReads_ = 0;
        lastRead_.invalidate();
        autoRead(kConnectReadDelayMs);
    }
    if (wasReading != reading()) {
        emit readingChanged();
        emit snapshotChanged();
    }
    if (wasTransfer && !telemetry_.transferActive && readWanted_ && !readTimer_.isActive()) {
        readTimer_.start(kRetryDelayMs);
    }
    checkStale();
    updateProgress(false);
    const GeoPoint vehicle = vehiclePosition();
    const GeoPoint home = vehicleHome();
    const bool vehicleChanged = vehicle.isValid() != drawnVehicle_.isValid()
                                || (vehicle.isValid() && geoDistance(vehicle, drawnVehicle_) >= kVehicleMoveStepM);
    const bool homeChanged = home.isValid() != drawnHome_.isValid()
                             || (home.isValid() && geoDistance(home, drawnHome_) >= kVehicleMoveStepM);
    if (vehicleChanged || homeChanged) {
        drawnVehicle_ = vehicle;
        drawnHome_ = home;
        emit vehicleMoved();
    }
}

void MissionRunTracker::onUploadRequested(const autopilot::MissionBatches& batches)
{
    pendingUpload_ = batches;
    pendingItems_ = buildItems(batches, true);
    hasPendingUpload_ = true;
}

void MissionRunTracker::onUploadFinished(bool ok, int result, int missionType)
{
    Q_UNUSED(missionType);
    if (!hasPendingUpload_) {
        return;
    }
    hasPendingUpload_ = false;
    autopilot::MissionBatches uploaded;
    uploaded.swap(pendingUpload_);
    QVector<RunItem> items;
    items.swap(pendingItems_);
    if (result == autopilot::MissionTransfer::ResultBusy) {
        return;
    }
    if (!ok) {
        if (telemetry_.online) {
            if (known_ && !stale_) {
                stale_ = true;
                emit snapshotChanged();
            }
            autoRead(kStaleReadDelayMs);
        }
        return;
    }
    for (int type : { int(MavMissionTypeFence), int(MavMissionTypeRally) }) {
        if (hasType(uploaded, type)) {
            continue;
        }
        for (const auto& b : std::as_const(snapshot_)) {
            if (b.missionType == type) {
                uploaded.append(b);
            }
        }
    }
    setSnapshot(uploaded, items, true);
}

void MissionRunTracker::onVehicleMissionRead(const autopilot::MissionBatches& batches, bool ok, bool silent)
{
    Q_UNUSED(ok);
    if (!routeOf(batches)) {
        if (telemetry_.online && (stale_ || !known_)) {
            ++failedReads_;
            autoRead(kRetryDelayMs);
        }
        return;
    }
    failedReads_ = 0;
    const bool linked = plan_ && sameRouteOnVehicle(batches, toVehicleUpload(plan_->expanded().mission));
    setSnapshot(batches, buildItems(batches, linked), linked);
    if (silent && plan_ && !vehicleEmpty_) {
        plan_->adoptVehicleMission(batches);
    }
}

void MissionRunTracker::retranslate()
{
    updateProgress(true);
}

QVector<RunItem> MissionRunTracker::buildItems(const autopilot::MissionBatches& batches, bool link) const
{
    QVector<RunItem> out;
    const autopilot::MissionBatch* route = routeOf(batches);
    if (!route) {
        return out;
    }
    QHash<QString, PlanOrigin> origins;
    const FlatMission* flat = nullptr;
    if (link && plan_) {
        flat = &plan_->expanded().mission;
        const auto& generated = plan_->expanded().generated;
        int ordinal = 0;
        for (const auto& item : plan_->plan().items) {
            ++ordinal;
            std::visit([&](const auto& it) {
                using T = std::decay_t<decltype(it)>;
                PlanOrigin o;
                o.ordinal = ordinal;
                if constexpr (std::is_same_v<T, WaypointItem>) {
                    o.kind = RunItem::Kind::Waypoint;
                } else if constexpr (std::is_same_v<T, SurveyItem>) {
                    o.kind = RunItem::Kind::Survey;
                } else {
                    o.kind = RunItem::Kind::Corridor;
                }
                const auto g = generated.constFind(it.id);
                if (g != generated.constEnd()) {
                    o.segmentCount = int(g->lines.size());
                }
                origins.insert(it.id, o);
            }, item);
        }
    }

    double speed = NAN;
    int navOrdinal = 0;
    bool startSeen = false;
    out.reserve(route->items.size());
    for (int seq = 0; seq < route->items.size(); ++seq) {
        const autopilot::MissionItem& m = route->items.at(seq);
        RunItem r;
        r.seq = seq;
        r.command = m.command;
        r.pos = positionOf(m);
        if (m.command == int(MavCmd::DoChangeSpeed) && m.param2 > 0.0f) {
            speed = m.param2;
        }
        r.plannedSpeed = speed;
        r.navigation = seq > 0 && m.command >= kNavCommandFirst && m.command <= kNavCommandLast;
        const FlatItem* f = (flat && seq > 0 && seq - 1 < flat->items.size()) ? &flat->items.at(seq - 1) : nullptr;
        if (f) {
            r.sourceId = f->sourceId;
        }
        if (r.navigation) {
            ++navOrdinal;
            const auto origin = f ? origins.constFind(f->sourceId) : origins.constEnd();
            if (f && f->sourceId == kHomeItemId) {
                r.kind = RunItem::Kind::Start;
            } else if (!f && !startSeen && m.command == int(MavCmd::NavWaypoint)) {
                r.kind = RunItem::Kind::Start;
                startSeen = true;
                --navOrdinal;
            } else if (origin != origins.constEnd()) {
                r.kind = origin->kind;
                r.ordinal = origin->ordinal;
                r.segment = f->segment;
                r.segmentCount = origin->segmentCount;
            } else if (m.command == int(MavCmd::NavWaypoint)) {
                r.kind = RunItem::Kind::Waypoint;
                r.ordinal = navOrdinal;
            } else if (m.command == int(MavCmd::NavReturnToLaunch)) {
                r.kind = RunItem::Kind::ReturnToLaunch;
            } else if (m.command == int(MavCmd::NavLoiterUnlim)) {
                r.kind = RunItem::Kind::Hold;
            } else {
                r.kind = RunItem::Kind::Other;
            }
        }
        out.append(r);
    }
    return out;
}

void MissionRunTracker::setSnapshot(const autopilot::MissionBatches& batches, const QVector<RunItem>& items, bool linked)
{
    snapshot_ = batches;
    items_ = items;
    linked_ = linked;
    known_ = true;
    stale_ = false;
    readWanted_ = false;
    readTimer_.stop();
    const autopilot::MissionBatch* route = routeOf(snapshot_);
    vehicleEmpty_ = !route || route->items.size() <= 1;
    baselineSet_ = false;
    snapshotReports_ = telemetry_.missionReports;
    rebuildGeometry();
    updateMatch();
    emit snapshotChanged();
    updateProgress(true);
}

void MissionRunTracker::clearSnapshot()
{
    const bool had = known_ || !items_.isEmpty();
    snapshot_.clear();
    items_.clear();
    linked_ = false;
    known_ = false;
    stale_ = false;
    vehicleEmpty_ = false;
    baselineSet_ = false;
    rebuildGeometry();
    updateMatch();
    if (had) {
        emit snapshotChanged();
    }
    updateProgress(true);
}

void MissionRunTracker::rebuildGeometry()
{
    navSeqs_.clear();
    navPos_.clear();
    legLength_.clear();
    legSpeed_.clear();
    const GeoPoint home = items_.isEmpty() ? GeoPoint() : items_.first().pos;
    for (const auto& r : std::as_const(items_)) {
        if (!r.navigation) {
            continue;
        }
        const GeoPoint p = r.kind == RunItem::Kind::ReturnToLaunch ? home : r.pos;
        if (!p.isValid()) {
            continue;
        }
        legLength_.append(navPos_.isEmpty() ? 0.0 : geoDistance(navPos_.last(), p));
        navSeqs_.append(r.seq);
        navPos_.append(p);
        legSpeed_.append(r.plannedSpeed);
    }
    const int n = int(navSeqs_.size());
    suffixLength_.fill(0.0, n);
    suffixTime_.fill(0.0, n);
    for (int k = n - 2; k >= 0; --k) {
        suffixLength_[k] = suffixLength_[k + 1] + legLength_[k + 1];
        const double v = legSpeed_[k + 1];
        suffixTime_[k] = (std::isfinite(suffixTime_[k + 1]) && v > 0.0) ? suffixTime_[k + 1] + legLength_[k + 1] / v : NAN;
    }
    totalLength_ = n > 0 ? suffixLength_.first() : 0.0;
}

void MissionRunTracker::updateMatch()
{
    bool matches = false;
    if (known_ && !vehicleEmpty_ && plan_ && plan_->exportable()) {
        matches = sameReadOnVehicle(snapshot_, toVehicleUpload(plan_->expanded().mission)) || plan_->matchesVehicleOrigin(snapshot_);
    }
    if (matches != matchesPlan_) {
        matchesPlan_ = matches;
        emit matchChanged();
    }
}

void MissionRunTracker::onPlanChanged()
{
    updateMatch();
    if (!known_ || linked_ || !plan_ || !sameRouteOnVehicle(snapshot_, toVehicleUpload(plan_->expanded().mission))) {
        return;
    }
    items_ = buildItems(snapshot_, true);
    linked_ = true;
    rebuildGeometry();
    emit snapshotChanged();
    updateProgress(true);
}

void MissionRunTracker::updateProgress(bool force)
{
    int nav = 0;
    int target = -1;
    QString label;
    double progress = -1.0;
    double remaining = NAN;
    double seconds = NAN;
    const bool running = known_ && (telemetry_.state == MavMissionStateActive
                                    || (telemetry_.state == MavMissionStateUnknown && telemetry_.flightMode == kRoverModeAuto && telemetry_.currentSeq >= 1));
    const bool complete = known_ && telemetry_.state == MavMissionStateComplete;
    bool finished = false;

    if (known_ && !navSeqs_.isEmpty() && telemetry_.currentSeq >= 0) {
        const int last = int(navSeqs_.size()) - 1;
        const int k = int(std::lower_bound(navSeqs_.cbegin(), navSeqs_.cend(), telemetry_.currentSeq) - navSeqs_.cbegin());
        if (complete || k > last) {
            finished = true;
            nav = last + 1;
            target = navSeqs_.at(last);
            remaining = 0.0;
            seconds = 0.0;
            progress = 1.0;
        } else {
            const GeoPoint vehicle(telemetry_.latitude, telemetry_.longitude);
            const double toTarget = vehicle.isValid() ? geoDistance(vehicle, navPos_.at(k)) : legLength_.at(k);
            remaining = toTarget + suffixLength_.at(k);
            progress = totalLength_ > 0.0 ? std::clamp(1.0 - remaining / totalLength_, 0.0, 1.0) : -1.0;
            const double v = legSpeed_.at(k);
            if (v > 0.0 && std::isfinite(suffixTime_.at(k))) {
                seconds = toTarget / v + suffixTime_.at(k);
            } else if (telemetry_.groundSpeed > kMinGroundSpeed) {
                seconds = remaining / telemetry_.groundSpeed;
            }
            nav = k + 1;
            target = navSeqs_.at(k);
        }
        label = labelFor(items_.at(target));
    }

    const bool changed = force || nav != currentNav_ || target != targetSeq_ || label != currentLabel_
                         || running != running_ || complete != complete_ || finished != finished_
                         || changedBy(progress, progress_, kProgressStep)
                         || changedBy(remaining, remainingDistance_, kDistanceStep)
                         || changedBy(seconds, remainingSeconds_, kSecondsStep);
    if (!changed) {
        return;
    }
    currentNav_ = nav;
    targetSeq_ = target;
    currentLabel_ = label;
    progress_ = progress;
    remainingDistance_ = remaining;
    remainingSeconds_ = seconds;
    running_ = running;
    complete_ = complete;
    finished_ = finished;
    emit progressChanged();
}

void MissionRunTracker::checkStale()
{
    if (!known_ || telemetry_.transferActive) {
        return;
    }
    if (!baselineSet_) {
        if (telemetry_.missionReports != snapshotReports_) {
            baselineTotal_ = telemetry_.total;
            baselineMissionId_ = telemetry_.missionId;
            baselineSet_ = true;
        }
        return;
    }
    const bool totalChanged = telemetry_.total != baselineTotal_ && (telemetry_.total >= 0 || baselineTotal_ >= 0);
    const bool idChanged = telemetry_.missionId != 0 && baselineMissionId_ != 0 && telemetry_.missionId != baselineMissionId_;
    if ((totalChanged || idChanged) && !stale_) {
        stale_ = true;
        emit snapshotChanged();
        autoRead(kStaleReadDelayMs);
    }
}

void MissionRunTracker::wantRead(int delayMs)
{
    readWanted_ = true;
    if (!readTimer_.isActive()) {
        readTimer_.start(delayMs);
    }
}

void MissionRunTracker::autoRead(int delayMs)
{
    if (readAutomatically_) {
        wantRead(delayMs);
    }
}

void MissionRunTracker::onReadTimer()
{
    if (!readWanted_ || !telemetry_.online || telemetry_.transferActive) {
        return;
    }
    if (failedReads_ >= kMaxFailedReads) {
        readWanted_ = false;
        return;
    }
    if (lastRead_.isValid() && lastRead_.elapsed() < kMinReadIntervalMs) {
        readTimer_.start(int(kMinReadIntervalMs - lastRead_.elapsed()));
        return;
    }
    readWanted_ = false;
    lastRead_.start();
    emit readRequested();
}

} // namespace mission
