#pragma once

#include <atomic>
#include <cstdint>
#include <memory>

#include <QAbstractListModel>
#include <QFutureWatcher>
#include <QHash>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>
#include <QVector>

#include "param_metadata.h"
#include "param_transfer.h"

namespace autopilot {

class AutopilotParams;

/** The parameters of the current group and search of AutopilotParams, sorted by name. */
class AutopilotParamRows : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Role {
        NameRole = Qt::UserRole + 1,
        ValueTextRole,
        TypeTextRole,
        PendingRole,
        FailedRole,
        DisplayNameRole,
        UnitsRole,
        ValueLabelRole,
        ReadOnlyRole
    };

    explicit AutopilotParamRows(AutopilotParams* owner);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

private:
    friend class AutopilotParams;

    void reset(const QVector<int>& entries);
    void merge(const QVector<int>& entries);
    void entryChanged(int entry);
    void allChanged();

    AutopilotParams* owner_;
    QVector<int> entries_;
};

/** The groups of AutopilotParams as a model: "All" first, prefixes by name, "Other" last. */
class AutopilotParamGroups : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Role {
        KeyRole = Qt::UserRole + 1,
        TitleRole,
        CountRole
    };

    struct Group
    {
        QString key;
        QString title;
        int count = 0;
    };

    explicit AutopilotParamGroups(QObject* parent);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

private:
    friend class AutopilotParams;

    void merge(const QVector<Group>& groups);

    QVector<Group> groups_;
};

/**
 * Every parameter of the latched autopilot component: name, value and MAV_PARAM_TYPE as the
 * vehicle reports them, plus display name, description, units, range and value list from the
 * bundled ArduPilot metadata (ParamMetadata) where it has the parameter. Loaded on request through
 * ParamTransfer; every later PARAM_VALUE (writes of other features, the transmitter) updates it.
 * The metadata is selected on the first load and again when the firmware version changes.
 *
 * Groups are the name up to the first '_' without trailing digits (SERIAL1_BAUD → SERIAL); a
 * group with a single member goes to "Other". Files use the QGroundControl text format
 * (vehicle id, component id, name, value, type, tab separated); reading also takes the Mission
 * Planner "name,value" format. A file is never written blindly: compareFile() lists the
 * differences and applyDiff() writes the selected ones.
 */
class AutopilotParams : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool online READ online NOTIFY stateChanged)
    Q_PROPERTY(bool loading READ loading NOTIFY stateChanged)
    Q_PROPERTY(bool loaded READ loaded NOTIFY stateChanged)
    /** True after the link was lost or the vehicle rebooted since the last complete load. */
    Q_PROPERTY(bool stale READ stale NOTIFY stateChanged)
    /** True when the last load got no answer to PARAM_REQUEST_LIST. */
    Q_PROPERTY(bool loadFailed READ loadFailed NOTIFY stateChanged)
    /** Indices the last load could not get after every retry. */
    Q_PROPERTY(int missing READ missing NOTIFY stateChanged)
    Q_PROPERTY(int received READ received NOTIFY progressChanged)
    Q_PROPERTY(int total READ total NOTIFY progressChanged)
    Q_PROPERTY(int count READ count NOTIFY listChanged)
    Q_PROPERTY(int pendingWrites READ pendingWrites NOTIFY writesChanged)
    /** { key, title, count } per group, "All" first and "Other" last; key "" is all parameters. */
    Q_PROPERTY(QVariantList groups READ groups NOTIFY listChanged)
    /** The same groups as a model that changes row by row, so a view keeps its scroll position while the list loads. */
    Q_PROPERTY(QAbstractItemModel* groupModel READ groupModel CONSTANT)
    Q_PROPERTY(QString currentGroup READ currentGroup WRITE setCurrentGroup NOTIFY filterChanged)
    /** Space separated words, each must occur in the name or display name (case-insensitive); searches all groups. */
    Q_PROPERTY(QString searchText READ searchText WRITE setSearchText NOTIFY filterChanged)
    Q_PROPERTY(QAbstractItemModel* params READ params CONSTANT)
    Q_PROPERTY(int visibleCount READ visibleCount NOTIFY visibleChanged)
    /** { name, current, file, selected } per parameter whose file value differs. */
    Q_PROPERTY(QVariantList diffRows READ diffRows NOTIFY diffChanged)
    Q_PROPERTY(int diffSelected READ diffSelected NOTIFY diffChanged)
    Q_PROPERTY(int diffUnchanged READ diffUnchanged NOTIFY diffChanged)
    /** Names in the file the vehicle does not have; they cannot be written. */
    Q_PROPERTY(QStringList diffMissing READ diffMissing NOTIFY diffChanged)
    /** Names whose file value does not fit the vehicle's type. */
    Q_PROPERTY(QStringList diffInvalid READ diffInvalid NOTIFY diffChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)
    /** Bundled metadata in use, "Rover 4.6"; empty when none applies to the vehicle. */
    Q_PROPERTY(QString metadataSource READ metadataSource NOTIFY metadataChanged)
    /** True when metadataSource is the firmware's own release, false for the nearest older or a fallback. */
    Q_PROPERTY(bool metadataExact READ metadataExact NOTIFY metadataChanged)

public:
    explicit AutopilotParams(QObject* parent = nullptr);
    ~AutopilotParams() override;

    bool online() const { return online_; }
    bool loading() const { return loading_; }
    bool loaded() const { return loaded_; }
    bool stale() const { return stale_; }
    bool loadFailed() const { return loadFailed_; }
    int missing() const { return missing_; }
    int received() const { return received_; }
    int total() const { return total_; }
    int count() const { return int(entries_.size()); }
    int pendingWrites() const { return transfer_.pendingWrites(); }
    QVariantList groups() const;
    QString currentGroup() const { return currentGroup_; }
    void setCurrentGroup(const QString& group);
    QString searchText() const { return searchText_; }
    void setSearchText(const QString& text);
    QAbstractItemModel* params() { return &rows_; }
    QAbstractItemModel* groupModel() { return &groupRows_; }
    int visibleCount() const { return rows_.rowCount(); }
    QVariantList diffRows() const;
    int diffSelected() const;
    int diffUnchanged() const { return diffUnchanged_; }
    QStringList diffMissing() const { return diffMissing_; }
    QStringList diffInvalid() const { return diffInvalid_; }
    QString lastError() const { return lastError_; }
    QString metadataSource() const { return metadata_.source(); }
    bool metadataExact() const { return metadata_.exact(); }

    /** Clears the list and reads it again from scratch; ignored while no autopilot is online. */
    Q_INVOKABLE void refresh();
    /** refresh() unless a complete, current list is already here or a load runs. */
    Q_INVOKABLE void ensureLoaded();
    /**
     * Reads only @p names by name, without the whole list, and loads the metadata; the answers
     * join the list like any PARAM_VALUE. Emits requestFinished() once all answered or gave up.
     * Ignored while no autopilot is online.
     */
    Q_INVOKABLE void request(const QStringList& names);
    /** True when the vehicle has reported @p name; @p value receives it. */
    bool value(const QString& name, float* value) const;
    /** The bundled metadata of @p name, or nullptr. */
    const ParamInfo* metadataOf(const QString& name) const { return metadata_.find(name); }
    /** Empty when @p text is a value parameter @p name can take, else the reason. */
    Q_INVOKABLE QString validate(const QString& name, const QString& text) const;
    /** Queues the write of @p text; false (with lastError) when validate() refuses it. */
    Q_INVOKABLE bool setValue(const QString& name, const QString& text);
    Q_INVOKABLE QString valueText(const QString& name) const;
    /**
     * { name, valueText, typeText, valueLabel, displayName, description, units, hasRange, low, high, increment,
     * values: [{ value, label }], bits: [{ bit, label }], rebootRequired, readOnly }; the metadata
     * fields are empty or false when the bundled metadata has no entry for @p name.
     */
    Q_INVOKABLE QVariantMap info(const QString& name) const;
    /** Non-blocking: empty, or a note that @p text is outside the documented range. */
    Q_INVOKABLE QString rangeWarning(const QString& name, const QString& text) const;
    Q_INVOKABLE bool saveToFile(const QString& pathOrUrl);
    /** Compares a parameter file with the vehicle and fills the diff properties; all differences start selected. */
    Q_INVOKABLE bool compareFile(const QString& pathOrUrl);
    Q_INVOKABLE void setDiffSelected(int row, bool selected);
    Q_INVOKABLE void setAllDiffSelected(bool selected);
    /** Queues the writes of the selected differences and clears the diff; returns how many. */
    Q_INVOKABLE int applyDiff();
    Q_INVOKABLE void clearDiff();
    /** Asks for an autopilot reboot (MAV_CMD_PREFLIGHT_REBOOT_SHUTDOWN); the list becomes stale. */
    Q_INVOKABLE void reboot();

public slots:
    /** The latched vehicle; another vehicle clears the list, going offline makes it stale and aborts the transfer. */
    void setVehicle(bool online, int systemId, int componentId);
    /**
     * HEARTBEAT autopilot and type and the AUTOPILOT_VERSION text. An unknown autopilot (-1) keeps
     * the metadata; while @p versionPending the choice waits for the version instead of loading the
     * newest release first. The metadata loads on a worker thread.
     */
    void setFirmware(int autopilotKind, int vehicleType, const QString& version, bool versionPending);
    /** Feeds the payload of one PARAM_VALUE of the autopilot component. */
    void handleParamValue(const QByteArray& payload);

signals:
    void sendMessage(quint32 msgId, const QByteArray& payload, int v1Length);
    void rebootRequested();
    void stateChanged();
    void progressChanged();
    void listChanged();
    void writesChanged();
    void filterChanged();
    void visibleChanged();
    void diffChanged();
    void lastErrorChanged();
    void metadataChanged();
    /** A write was not confirmed; @p actual is the value the vehicle reports now (may still change). */
    void writeFailed(const QString& name, const QString& actual);
    void writeConfirmed(const QString& name, const QString& value);
    /** A PARAM_VALUE for @p name arrived (any source: a load, a read, a write echo, another ground station). */
    void paramChanged(const QString& name);
    /** The end of request(); @p missing never answered. */
    void requestFinished(const QStringList& missing);

private:
    friend class AutopilotParamRows;

    struct Entry
    {
        QString name;
        float value = 0.0f;
        uint8_t type = 0;
        int index = -1;
        QString group;
        bool pending = false;
        bool failed = false;
    };

    struct DiffRow
    {
        QString name;
        QString current;
        QString file;
        float value = 0.0f;
        uint8_t type = 0;
        bool selected = true;
    };

    void onValueReceived(const QString& name, float value, int type, int index, int count);
    void onProgress(int received, int total);
    void onDownloadFinished(bool ok, int result, int missing);
    void onWriteFinished(const QString& name, bool ok, int result);

    void clearAll();
    void scheduleRebuild();
    void rebuild();
    void rebuildGroups();
    QVector<int> visibleEntries() const;
    void refilterFromTop();
    void refilterInPlace();
    bool matchesFilter(const Entry& e) const;
    bool parseValue(const Entry& e, const QString& text, float* out, QString* error) const;
    void setLastError(const QString& error);
    void setLoading(bool loading);
    void updateMetadata();
    void onMetadataLoaded();
    void cancelMetadataLoad();

    static QString groupOf(const QString& name);
    static QString formatValue(float value, uint8_t type);
    static QString typeText(uint8_t type);
    static QString localPath(const QString& pathOrUrl);

    ParamTransfer transfer_{ this };
    AutopilotParamRows rows_{ this };
    AutopilotParamGroups groupRows_{ this };
    QVector<Entry> entries_;
    QVector<int> order_;
    QHash<QString, int> byName_;
    QString currentGroup_;
    QString searchText_;
    QTimer rebuildTimer_{ this };
    ParamMetadata metadata_;
    QFutureWatcher<ParamMetadata> metadataWatcher_;
    std::shared_ptr<std::atomic_bool> metadataCancel_;
    QString metadataKey_;
    bool metadataWanted_ = false;
    int autopilotKind_ = -1;
    int vehicleType_ = -1;
    QString firmwareVersion_;
    bool versionPending_ = false;

    bool online_ = false;
    int systemId_ = -1;
    int componentId_ = -1;
    bool loading_ = false;
    bool loaded_ = false;
    bool stale_ = false;
    bool loadFailed_ = false;
    int missing_ = 0;
    int received_ = 0;
    int total_ = 0;

    QVector<DiffRow> diff_;
    int diffUnchanged_ = 0;
    QStringList diffMissing_;
    QStringList diffInvalid_;
    QString lastError_;
};

} // namespace autopilot
