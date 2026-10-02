#pragma once

#include <cmath>

#include <QElapsedTimer>
#include <QObject>
#include <QString>
#include <QTimer>
#include <QVector>

#include "mission_model.h"
#include "mission_transfer.h"

namespace mission {

class MissionPlanController;

/** Vehicle state the run tracker follows, filled by Core from DeviceManagerWrapper on every change. */
struct RunTelemetry {
    bool online = false;
    bool transferActive = false;
    bool transferSilent = false;
    int flightMode = -1;
    int currentSeq = -1;
    int total = -1;
    int state = 0;
    quint32 missionId = 0;
    quint32 missionReports = 0;
    double latitude = NAN;
    double longitude = NAN;
    double groundSpeed = NAN;
    double homeLat = NAN;
    double homeLon = NAN;
};

/** One item of the vehicle's route list (index = MAVLink seq) as the run shows it. */
struct RunItem {
    enum class Kind { Command, Other, Start, Waypoint, Survey, Corridor, ReturnToLaunch, Hold };

    int seq = 0;
    int command = 0;
    Kind kind = Kind::Command;
    GeoPoint pos;
    bool navigation = false;
    double plannedSpeed = NAN;
    int ordinal = 0;
    int segment = -1;
    int segmentCount = 0;
    QString sourceId;
};

/**
 * Follows the mission that is on the vehicle (GUI thread, owned by Core, QML "missionRun").
 *
 * The snapshot of the vehicle's lists comes from our successful upload (items then carry their
 * plan origin: start point, waypoint N, survey / corridor N with the line), from any download,
 * and from a silent read 1 s after the vehicle comes online (QGroundControl reads on connect). A
 * downloaded route is linked to the open plan when it equals what that plan would upload. The
 * snapshot is marked stale when MISSION_CURRENT's total or mission_id change after the first
 * report seen with it, and is then read again silently once no transfer runs (at most every 10 s,
 * three failed reads per connection). A silent read fills a blank plan (adoptVehicleMission).
 * Offline, everything is cleared.
 *
 * Progress is measured along the positioned navigation items from seq 1 (RTL counts at the home
 * slot position): remaining = vehicle → current target + the legs after it; time from the planned
 * DO_CHANGE_SPEED speeds when all remaining legs have one, otherwise from the ground speed.
 */
class MissionRunTracker : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool    known             READ known             NOTIFY snapshotChanged)
    Q_PROPERTY(bool    stale             READ stale             NOTIFY snapshotChanged)
    Q_PROPERTY(bool    vehicleEmpty      READ vehicleEmpty      NOTIFY snapshotChanged)
    Q_PROPERTY(int     itemCount         READ itemCount         NOTIFY snapshotChanged)
    Q_PROPERTY(int     navCount          READ navCount          NOTIFY snapshotChanged)
    Q_PROPERTY(bool    linkedToPlan      READ linkedToPlan      NOTIFY snapshotChanged)
    Q_PROPERTY(bool    reading           READ reading           NOTIFY readingChanged)
    Q_PROPERTY(bool    matchesPlan       READ matchesPlan       NOTIFY matchChanged)
    Q_PROPERTY(int     currentNav        READ currentNav        NOTIFY progressChanged)
    Q_PROPERTY(int     targetSeq         READ targetSeq         NOTIFY progressChanged)
    Q_PROPERTY(QString currentLabel      READ currentLabel      NOTIFY progressChanged)
    Q_PROPERTY(double  progress          READ progress          NOTIFY progressChanged)
    Q_PROPERTY(double  remainingDistance READ remainingDistance NOTIFY progressChanged)
    Q_PROPERTY(double  remainingSeconds  READ remainingSeconds  NOTIFY progressChanged)
    Q_PROPERTY(bool    running           READ running           NOTIFY progressChanged)
    Q_PROPERTY(bool    complete          READ complete          NOTIFY progressChanged)
    Q_PROPERTY(bool    actionable        READ actionable        NOTIFY snapshotChanged)
    /** Read the vehicle's lists by itself (on connect, when they change on the vehicle, after a failed read); refresh() always reads. */
    Q_PROPERTY(bool    readAutomatically READ readAutomatically WRITE setReadAutomatically NOTIFY readAutomaticallyChanged)

public:
    explicit MissionRunTracker(MissionPlanController* plan, QObject* parent = nullptr);

    bool    known() const { return known_; }
    bool    stale() const { return stale_; }
    bool    vehicleEmpty() const { return vehicleEmpty_; }
    int     itemCount() const { return int(items_.size()); }
    int     navCount() const { return int(navSeqs_.size()); }
    bool    linkedToPlan() const { return linked_; }
    bool    reading() const { return telemetry_.transferActive && telemetry_.transferSilent; }
    bool    matchesPlan() const { return matchesPlan_; }
    int     currentNav() const { return currentNav_; }
    int     targetSeq() const { return targetSeq_; }
    QString currentLabel() const { return currentLabel_; }
    double  progress() const { return progress_; }
    double  remainingDistance() const { return remainingDistance_; }
    double  remainingSeconds() const { return remainingSeconds_; }
    bool    running() const { return running_; }
    bool    complete() const { return complete_; }
    /** The route is past its last navigation item (MISSION_STATE COMPLETE or current seq beyond it). */
    bool    finished() const { return finished_; }
    /** A jump can be sent: the snapshot is known, not stale and not being read again. */
    bool    actionable() const { return known_ && !stale_ && !reading(); }
    bool    readAutomatically() const { return readAutomatically_; }
    void    setReadAutomatically(bool read);

    const autopilot::MissionBatches& snapshot() const { return snapshot_; }
    const QVector<RunItem>& items() const { return items_; }
    /** Seqs of the positioned navigation items the progress runs along, and their positions. */
    const QVector<int>& navSeqs() const { return navSeqs_; }
    const QVector<GeoPoint>& navPositions() const { return navPos_; }
    /** 1-based place of @p seq among navSeqs(), 0 when it is not one of them. */
    int navIndexOf(int seq) const;
    /**
     * The seq to make current for "go to @p seq": the DO_CHANGE_SPEED right before it when there
     * is one, so the leg keeps its planned speed; otherwise @p seq itself. -1 while not actionable(),
     * so a jump is never computed from a snapshot the vehicle no longer holds.
     */
    Q_INVOKABLE int jumpSeqFor(int seq) const;
    /** The navigation item after the current target, -1 when the target is the last one, unknown, or not actionable(). */
    Q_INVOKABLE int nextTargetSeq() const;
    /** labelFor the route item @p seq, empty when there is none. */
    Q_INVOKABLE QString labelForSeq(int seq) const;
    GeoPoint vehiclePosition() const { return GeoPoint(telemetry_.latitude, telemetry_.longitude); }
    GeoPoint vehicleHome() const { return GeoPoint(telemetry_.homeLat, telemetry_.homeLon); }
    /** Display name of @p item: "Start point", "Waypoint 3", "Survey 2 · line 4/12", … */
    QString labelFor(const RunItem& item) const;

    /** Hands the snapshot to the plan editor as if it had just been read (MissionPlanController::receiveVehicleMission). */
    Q_INVOKABLE bool openInEditor();
    /** Reads the vehicle's lists again silently. */
    Q_INVOKABLE void refresh();

    void setTelemetry(const RunTelemetry& telemetry);

public slots:
    void onUploadRequested(const autopilot::MissionBatches& batches);
    void onUploadFinished(bool ok, int result, int missionType);
    void onVehicleMissionRead(const autopilot::MissionBatches& batches, bool ok, bool silent);
    void retranslate();

signals:
    void snapshotChanged();
    void readingChanged();
    void matchChanged();
    void progressChanged();
    void vehicleMoved();
    void readAutomaticallyChanged();
    void readRequested();

private:
    QVector<RunItem> buildItems(const autopilot::MissionBatches& batches, bool link) const;
    void setSnapshot(const autopilot::MissionBatches& batches, const QVector<RunItem>& items, bool linked);
    void clearSnapshot();
    void rebuildGeometry();
    void updateMatch();
    void updateProgress(bool force);
    void checkStale();
    void wantRead(int delayMs);
    void autoRead(int delayMs);
    void onReadTimer();
    void onPlanChanged();

    MissionPlanController* plan_ = nullptr;
    RunTelemetry telemetry_;
    autopilot::MissionBatches snapshot_;
    QVector<RunItem> items_;
    bool known_ = false;
    bool stale_ = false;
    bool vehicleEmpty_ = false;
    bool linked_ = false;
    bool matchesPlan_ = false;

    QVector<int> navSeqs_;
    QVector<GeoPoint> navPos_;
    QVector<double> legLength_;
    QVector<double> legSpeed_;
    QVector<double> suffixLength_;
    QVector<double> suffixTime_;
    double totalLength_ = 0.0;

    int currentNav_ = 0;
    int targetSeq_ = -1;
    QString currentLabel_;
    double progress_ = -1.0;
    double remainingDistance_ = NAN;
    double remainingSeconds_ = NAN;
    bool running_ = false;
    bool complete_ = false;
    bool finished_ = false;

    bool hasPendingUpload_ = false;
    autopilot::MissionBatches pendingUpload_;
    QVector<RunItem> pendingItems_;

    bool baselineSet_ = false;
    quint32 snapshotReports_ = 0;
    int baselineTotal_ = -1;
    quint32 baselineMissionId_ = 0;

    bool readWanted_ = false;
    bool readAutomatically_ = true;
    int failedReads_ = 0;
    QTimer readTimer_{ this };
    QElapsedTimer lastRead_;
    GeoPoint drawnVehicle_;
    GeoPoint drawnHome_;
};

} // namespace mission
