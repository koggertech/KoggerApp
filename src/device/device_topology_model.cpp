#include "device_topology_model.h"

#include <algorithm>
#include <limits>
#include <QAbstractItemModel>

#include "device_manager_wrapper.h"
#include "link_manager_wrapper.h"
#include "link_list_model.h"
#include "dev_q_property.h"

namespace {

QVariantMap nodeFor(DevQProperty* dev, DevQProperty* master, bool hasMaster)
{
    QVariantMap node;
    node["device"] = QVariant::fromValue(dev);

    int role = 2;
    if (dev == master)
        role = 0;
    else if (hasMaster)
        role = 1;
    node["role"] = role;

    int port = -1;
    if (hasMaster && dev != master) {
        const int addr = dev->getBusAddress();
        if (addr >= 1 && addr <= 8)
            port = addr;
    }
    node["port"] = port;

    return node;
}

}

DeviceTopologyModel::DeviceTopologyModel(DeviceManagerWrapper* deviceWrapper,
                                         LinkManagerWrapper* linkWrapper,
                                         QObject* parent)
    : QObject(parent),
      deviceWrapper_(deviceWrapper),
      linkWrapper_(linkWrapper),
      rebuildScheduled_(false)
{
    if (deviceWrapper_)
        connect(deviceWrapper_, &DeviceManagerWrapper::devChanged, this, &DeviceTopologyModel::scheduleRebuild);

    if (linkWrapper_) {
        connect(linkWrapper_, &LinkManagerWrapper::modelChanged, this, &DeviceTopologyModel::scheduleRebuild);
        if (LinkListModel* m = linkWrapper_->getModelPtr()) {
            connect(m, &QAbstractItemModel::rowsInserted, this, &DeviceTopologyModel::scheduleRebuild);
            connect(m, &QAbstractItemModel::rowsRemoved,  this, &DeviceTopologyModel::scheduleRebuild);
            connect(m, &QAbstractItemModel::modelReset,   this, &DeviceTopologyModel::scheduleRebuild);
        }
    }

    scheduleRebuild();
}

void DeviceTopologyModel::scheduleRebuild()
{
    if (rebuildScheduled_)
        return;
    rebuildScheduled_ = true;
    QMetaObject::invokeMethod(this, "rebuild", Qt::QueuedConnection);
}

void DeviceTopologyModel::rebuild()
{
    rebuildScheduled_ = false;

    const QList<DevQProperty*> devices = deviceWrapper_ ? deviceWrapper_->getDevList() : QList<DevQProperty*>();

    for (DevQProperty* d : devices) {
        if (!d || watched_.contains(d))
            continue;
        watched_.insert(d);
        connect(d, &DevQProperty::deviceVersionChanged, this, &DeviceTopologyModel::scheduleRebuild);
        connect(d, &QObject::destroyed, this, [this](QObject* obj) {
            watched_.remove(static_cast<DevQProperty*>(obj));
        });
    }

    QList<QUuid> order;
    QHash<QUuid, QList<DevQProperty*>> byLink;
    for (DevQProperty* d : devices) {
        if (!d || !d->isBoardInited())
            continue;
        const QUuid u = d->getLinkUuid();
        if (!byLink.contains(u))
            order.append(u);
        byLink[u].append(d);
    }

    if (LinkListModel* lm = linkWrapper_ ? linkWrapper_->getModelPtr() : nullptr) {
        std::stable_sort(order.begin(), order.end(), [lm](const QUuid& a, const QUuid& b) {
            int ra = lm->rowForUuid(a);
            int rb = lm->rowForUuid(b);
            if (ra < 0) ra = std::numeric_limits<int>::max();
            if (rb < 0) rb = std::numeric_limits<int>::max();
            return ra < rb;
        });
    }

    QVariantList out;
    for (const QUuid& u : order) {
        const QList<DevQProperty*>& members = byLink[u];

        DevQProperty* master = nullptr;
        for (DevQProperty* d : members) {
            if (d->isRecorder()) {
                master = d;
                break;
            }
        }

        const bool hasMaster = (master != nullptr);

        QList<DevQProperty*> childDevs;
        for (DevQProperty* d : members)
            if (d != master)
                childDevs.append(d);
        std::sort(childDevs.begin(), childDevs.end(), [](DevQProperty* a, DevQProperty* b) {
            return a->getBusAddress() < b->getBusAddress();
        });

        QVariantList childNodes;
        QVariantList memberNodes;
        if (master)
            memberNodes.append(nodeFor(master, master, hasMaster));
        for (DevQProperty* d : childDevs) {
            const QVariant node = nodeFor(d, master, hasMaster);
            childNodes.append(node);
            memberNodes.append(node);
        }

        QVariantMap group = buildLinkMeta(u);
        group["hasMaster"] = hasMaster;
        group["master"]    = master ? nodeFor(master, master, hasMaster) : QVariant();
        group["children"]  = childNodes;
        group["members"]   = memberNodes;
        out.append(group);
    }

    groups_ = out;
    emit changed();
}

QVariantMap DeviceTopologyModel::buildLinkMeta(const QUuid& linkUuid) const
{
    QVariantMap meta;
    meta["linkUuid"]        = linkUuid.toString(QUuid::WithoutBraces);
    meta["linkType"]        = -1;
    meta["portName"]        = QString();
    meta["address"]         = QString();
    meta["baudrate"]        = 0;
    meta["sourcePort"]      = 0;
    meta["destinationPort"] = 0;
    meta["linkPresent"]     = false;

    if (!linkWrapper_)
        return meta;

    LinkListModel* m = linkWrapper_->getModelPtr();
    if (!m || !m->containsUuid(linkUuid))
        return meta;

    using R = LinkListModel::Roles;
    meta["linkType"]        = m->valueForUuid(linkUuid, R::LinkType).toInt();
    meta["portName"]        = m->valueForUuid(linkUuid, R::PortName).toString();
    meta["address"]         = m->valueForUuid(linkUuid, R::Address).toString();
    meta["baudrate"]        = m->valueForUuid(linkUuid, R::Baudrate).toInt();
    meta["sourcePort"]      = m->valueForUuid(linkUuid, R::SourcePort).toInt();
    meta["destinationPort"] = m->valueForUuid(linkUuid, R::DestinationPort).toInt();
    meta["linkPresent"]     = true;
    return meta;
}

QVariantMap DeviceTopologyModel::groupForLinkUuid(const QString& linkUuid) const
{
    auto norm = [](const QString& s) {
        QString t = s;
        t.remove('{');
        t.remove('}');
        return t.toLower();
    };

    const QString target = norm(linkUuid);
    for (const QVariant& g : groups_) {
        const QVariantMap m = g.toMap();
        if (norm(m.value("linkUuid").toString()) == target)
            return m;
    }
    return QVariantMap();
}

QVariantMap DeviceTopologyModel::groupForDevice(DevQProperty* device) const
{
    if (!device)
        return QVariantMap();
    return groupForLinkUuid(device->getLinkUuid().toString(QUuid::WithoutBraces));
}
