#pragma once

#include <QJsonObject>
#include <QObject>
#include <QRandomGenerator>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <QVector>

#include "mission_expander.h"
#include "mission_model.h"
#include "mission_transfer.h"

namespace mission {

class MissionPlanController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString      filePath          READ filePath                                    NOTIFY filePathChanged)
    Q_PROPERTY(QString      name              READ name              WRITE setName             NOTIFY planChanged)
    Q_PROPERTY(bool         dirty             READ dirty                                       NOTIFY dirtyChanged)
    Q_PROPERTY(bool         canUndo           READ canUndo                                     NOTIFY undoChanged)
    Q_PROPERTY(bool         canRedo           READ canRedo                                     NOTIFY undoChanged)
    Q_PROPERTY(int          itemCount         READ itemCount                                   NOTIFY planChanged)
    Q_PROPERTY(int          rallyCount        READ rallyCount                                  NOTIFY planChanged)
    Q_PROPERTY(bool         hasHome           READ hasHome                                     NOTIFY planChanged)
    Q_PROPERTY(double       homeLat           READ homeLat                                     NOTIFY planChanged)
    Q_PROPERTY(double       homeLon           READ homeLon                                     NOTIFY planChanged)
    Q_PROPERTY(int          fenceCount        READ fenceCount                                  NOTIFY planChanged)
    Q_PROPERTY(double       cruiseSpeed       READ cruiseSpeed       WRITE setCruiseSpeed      NOTIFY planChanged)
    Q_PROPERTY(int          endAction         READ endAction         WRITE setEndAction        NOTIFY planChanged)
    Q_PROPERTY(QVariantMap  estimates         READ estimates                                   NOTIFY planChanged)
    Q_PROPERTY(QStringList  warnings          READ warnings                                    NOTIFY planChanged)
    Q_PROPERTY(QVariantList issues            READ issues                                      NOTIFY planChanged)
    Q_PROPERTY(int          maxMissionItems   READ maxMissionItems                             CONSTANT)
    Q_PROPERTY(bool         exportable        READ exportable                                  NOTIFY planChanged)
    Q_PROPERTY(QString      exportBlocker     READ exportBlocker                               NOTIFY planChanged)
    Q_PROPERTY(QString      lastError         READ lastError                                   NOTIFY lastErrorChanged)
    Q_PROPERTY(QString      directory         READ directory                                   NOTIFY directoryChanged)
    Q_PROPERTY(QString      directoryOverride READ directoryOverride WRITE setDirectoryOverride NOTIFY directoryChanged)
    Q_PROPERTY(QString      lastFile          READ lastFile                                    NOTIFY lastFileChanged)

public:
    explicit MissionPlanController(QObject* parent = nullptr);

    void setFallbackDirectory(const QString& dir);

    QString     filePath() const { return filePath_; }
    QString     name() const { return plan_.name; }
    bool        dirty() const;
    bool        canUndo() const { return !undo_.isEmpty(); }
    bool        canRedo() const { return !redo_.isEmpty(); }
    int         itemCount() const { return plan_.items.size(); }
    int         rallyCount() const { return plan_.rally.size(); }
    bool        hasHome() const { return plan_.home.has_value(); }
    double      homeLat() const { return plan_.home ? plan_.home->lat : NAN; }
    double      homeLon() const { return plan_.home ? plan_.home->lon : NAN; }
    int         fenceCount() const { return plan_.fence.size(); }
    double      cruiseSpeed() const { return plan_.settings.cruiseSpeed; }
    int         endAction() const { return static_cast<int>(plan_.settings.endAction); }
    QVariantMap estimates() const;
    QStringList warnings() const { return expanded_.warnings(); }
    QVariantList issues() const;
    int         maxMissionItems() const { return kMaxMissionItems; }
    bool        exportable() const { return exportBlocker().isEmpty(); }
    QString     exportBlocker() const;
    QString     lastError() const { return lastError_; }
    QString     directory() const;
    QString     directoryOverride() const { return directoryOverride_; }
    QString     lastFile() const { return lastFile_; }

    const MissionPlan&    plan() const { return plan_; }
    const ExpandResult& expanded() const { return expanded_; }

    void setName(const QString& name);
    void setCruiseSpeed(double speed);
    void setEndAction(int action);
    void setDirectoryOverride(const QString& dir);

    Q_INVOKABLE void    newPlan();
    Q_INVOKABLE bool    openFile(const QString& path);
    Q_INVOKABLE bool    saveFile();
    Q_INVOKABLE bool    saveFileAs(const QString& path);
    Q_INVOKABLE bool    exportPlanFile(const QString& path);
    Q_INVOKABLE bool    exportWplFile(const QString& path);
    Q_INVOKABLE bool    uploadToVehicle();
    Q_INVOKABLE bool    openVehicleMission();
    /** True for a new plan nothing was added to: no items, start point, rally or fence, no file, no unsaved changes. */
    bool isBlank() const;
    /**
     * Shows the vehicle's mission in a blank plan, the QGroundControl rule for an empty Plan view:
     * imported as receiveVehicleMission + openVehicleMission would, but left without unsaved
     * changes. Does nothing and returns false when the plan is not blank or the vehicle has no mission.
     */
    bool adoptVehicleMission(const autopilot::MissionBatches& batches);
    /**
     * True when the plan was opened from the vehicle's lists @p vehicle (still the same lists, compared
     * with sameReadOnVehicle) and its upload has not changed since — renaming or saving keep it. An
     * imported mission does not always expand back to the very same lists (a trailing speed change,
     * an ending written by an older build, a mission made elsewhere), so this is what tells "the plan
     * read from this vehicle" apart from "a different plan".
     */
    bool matchesVehicleOrigin(const autopilot::MissionBatches& vehicle) const;
    Q_INVOKABLE QString defaultFileName() const;
    Q_INVOKABLE QString directoryUrl() const;
    Q_INVOKABLE QString suggestedFilePath() const;
    Q_INVOKABLE void    rememberDirectoryOf(const QString& fileOrUrl);

    Q_INVOKABLE QString planJson() const;
    Q_INVOKABLE bool    loadPlanJson(const QString& json);
    Q_INVOKABLE QString flatJson() const;
    Q_INVOKABLE QString itemJson(const QString& id) const;
    Q_INVOKABLE QVariantMap itemInfo(const QString& id) const;
    Q_INVOKABLE QVariantList generatedPoints(const QString& id) const;
    Q_INVOKABLE QVariantList generatedLines(const QString& id) const;
    Q_INVOKABLE QStringList itemIds() const;
    Q_INVOKABLE QStringList rallyIds() const;
    Q_INVOKABLE QStringList fenceIds() const;
    Q_INVOKABLE int     indexOfItem(const QString& id) const;
    Q_INVOKABLE int     indexOfFence(const QString& id) const;

    Q_INVOKABLE void    setHome(double lat, double lon);
    Q_INVOKABLE void    clearHome();
    Q_INVOKABLE QString addWaypoint(double lat, double lon, int insertIndex = -1);
    Q_INVOKABLE QString addSurvey(const QVariantList& polygon, int insertIndex = -1);
    Q_INVOKABLE QString addCorridor(const QVariantList& axis, int insertIndex = -1);
    Q_INVOKABLE QString addRally(double lat, double lon);
    Q_INVOKABLE QString addFence(const QVariantList& polygon, bool inclusion);
    Q_INVOKABLE bool    removeItem(const QString& id);
    Q_INVOKABLE bool    moveItem(const QString& id, int newIndex);
    Q_INVOKABLE bool    updateItem(const QString& id, const QVariantMap& patch);
    Q_INVOKABLE bool    moveVertex(const QString& id, int index, double lat, double lon);
    Q_INVOKABLE bool    insertVertex(const QString& id, int index, double lat, double lon);
    Q_INVOKABLE bool    removeVertex(const QString& id, int index);

    Q_INVOKABLE void    undo();
    Q_INVOKABLE void    redo();
    Q_INVOKABLE void    beginTransaction();
    Q_INVOKABLE void    endTransaction(bool keep = true);

    Q_INVOKABLE void    setIdSeed(uint seed);
    Q_INVOKABLE void    setFrozenTime(const QString& isoUtc);

public slots:
    void retranslate();
    void receiveVehicleMission(const autopilot::MissionBatches& batches);

signals:
    void planChanged();
    void filePathChanged();
    void dirtyChanged();
    void undoChanged();
    void lastErrorChanged();
    void directoryChanged();
    void lastFileChanged();
    void uploadRequested(const autopilot::MissionBatches& batches);
    void vehicleMissionReceived(bool empty, bool matches, int routePoints, int fenceCount, int rallyCount, int skipped);

private:
    enum class VertexOwner { None, Waypoint, Survey, Corridor, Rally };

    QString     nowIso() const;
    QString     newId();
    QByteArray  snapshot() const;
    void        pushUndo();
    bool        restore(const QByteArray& bytes);
    void        afterChange();
    void        setLastError(const QString& error);
    void        setFilePath(const QString& path);
    void        rememberLastFile(const QString& path);
    void        markClean();
    MissionItem*  findItem(const QString& id);
    RallyItem*  findRally(const QString& id);
    FencePolygon* findFence(const QString& id);
    QVector<GeoPoint>* vertexListById(const QString& id, int* minCount);
    bool        idTaken(const QString& id) const;
    bool        parsePoints(const QVariantList& list, QVector<GeoPoint>* out, int minCount);
    int         clampInsertIndex(int index) const;
    void        applyVehiclePlan(const MissionPlan& plan, const QVector<int>& unreadTypes, bool clean, const autopilot::MissionBatches& origin);

    MissionPlan        plan_;
    ExpandResult     expanded_;
    QString          filePath_;
    QString          appVersion_;
    QString          lastError_;
    QString          fallbackDirectory_;
    QString          directoryOverride_;
    QString          lastFile_;
    QString          frozenTime_;
    QByteArray       cleanSnapshot_;
    bool             lastDirty_ = false;
    std::optional<MissionPlan> vehiclePlan_;
    autopilot::MissionBatches vehicleBatches_;
    autopilot::MissionBatches originRead_;
    autopilot::MissionBatches originPlanned_;
    bool hasOrigin_ = false;
    QVector<int> vehicleUnreadTypes_;
    QVector<int> uploadSkipTypes_;
    int              transactionDepth_ = 0;
    QByteArray       transactionSnapshot_;
    QVector<QByteArray> undo_;
    QVector<QByteArray> redo_;
    QRandomGenerator rng_;
};

} // namespace mission
