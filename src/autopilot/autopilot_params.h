#pragma once

#include <cstdint>

#include <QAbstractListModel>
#include <QHash>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QVariantList>
#include <QVector>

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
        FailedRole
    };

    explicit AutopilotParamRows(AutopilotParams* owner);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

private:
    friend class AutopilotParams;

    void reset(const QVector<int>& entries);
    void entryChanged(int entry);

    AutopilotParams* owner_;
    QVector<int> entries_;
};

/**
 * Every parameter of the latched autopilot component, as the vehicle reports it: name, value and
 * MAV_PARAM_TYPE, nothing more (no descriptions, units or ranges). Loaded on request through
 * ParamTransfer; every later PARAM_VALUE (writes of other features, the transmitter) updates it.
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
    Q_PROPERTY(QString currentGroup READ currentGroup WRITE setCurrentGroup NOTIFY filterChanged)
    /** Space separated words, each must occur in the name (case-insensitive); searches all groups. */
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

public:
    explicit AutopilotParams(QObject* parent = nullptr);

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
    QVariantList groups() const { return groups_; }
    QString currentGroup() const { return currentGroup_; }
    void setCurrentGroup(const QString& group);
    QString searchText() const { return searchText_; }
    void setSearchText(const QString& text);
    QAbstractItemModel* params() { return &rows_; }
    int visibleCount() const { return rows_.rowCount(); }
    QVariantList diffRows() const;
    int diffSelected() const;
    int diffUnchanged() const { return diffUnchanged_; }
    QStringList diffMissing() const { return diffMissing_; }
    QStringList diffInvalid() const { return diffInvalid_; }
    QString lastError() const { return lastError_; }

    /** Reads the whole list again; ignored while no autopilot is online. */
    Q_INVOKABLE void refresh();
    /** refresh() unless a complete, current list is already here or a load runs. */
    Q_INVOKABLE void ensureLoaded();
    /** Empty when @p text is a value parameter @p name can take, else the reason. */
    Q_INVOKABLE QString validate(const QString& name, const QString& text) const;
    /** Queues the write of @p text; false (with lastError) when validate() refuses it. */
    Q_INVOKABLE bool setValue(const QString& name, const QString& text);
    Q_INVOKABLE QString valueText(const QString& name) const;
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
    /** A write was not confirmed; @p actual is the value the vehicle reports now (may still change). */
    void writeFailed(const QString& name, const QString& actual);
    void writeConfirmed(const QString& name, const QString& value);

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
    void applyFilter();
    bool matchesFilter(const Entry& e) const;
    bool parseValue(const Entry& e, const QString& text, float* out, QString* error) const;
    void setLastError(const QString& error);
    void setLoading(bool loading);

    static QString groupOf(const QString& name);
    static QString formatValue(float value, uint8_t type);
    static QString typeText(uint8_t type);
    static QString localPath(const QString& pathOrUrl);

    ParamTransfer transfer_{ this };
    AutopilotParamRows rows_{ this };
    QVector<Entry> entries_;
    QVector<int> order_;
    QHash<QString, int> byName_;
    QVariantList groups_;
    QString currentGroup_;
    QString searchText_;
    QTimer rebuildTimer_{ this };

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
