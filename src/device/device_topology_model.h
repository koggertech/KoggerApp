#pragma once

#include <QObject>
#include <QVariantList>
#include <QVariantMap>
#include <QUuid>
#include <QSet>

class DeviceManagerWrapper;
class LinkManagerWrapper;
class LinkListModel;
class DevQProperty;

class DeviceTopologyModel : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QVariantList groups READ groups NOTIFY changed)

public:
    explicit DeviceTopologyModel(DeviceManagerWrapper* deviceWrapper,
                                 LinkManagerWrapper* linkWrapper,
                                 QObject* parent = nullptr);

    QVariantList groups() const { return groups_; }

    Q_INVOKABLE QVariantMap groupForLinkUuid(const QString& linkUuid) const;
    Q_INVOKABLE QVariantMap groupForDevice(DevQProperty* device) const;

signals:
    void changed();

private slots:
    void scheduleRebuild();
    void rebuild();

private:
    QVariantMap buildLinkMeta(const QUuid& linkUuid) const;

    DeviceManagerWrapper* deviceWrapper_;
    LinkManagerWrapper*   linkWrapper_;
    QVariantList          groups_;
    QSet<DevQProperty*>   watched_;
    bool                  rebuildScheduled_;
};
