#pragma once

#include <QJsonObject>
#include <QString>

#include "mission_model.h"

namespace mission {

struct LoadResult {
    bool ok = false;
    QString error;
    MissionPlan plan;
};

namespace FileIo {

QJsonObject toJson(const MissionPlan& plan, const QString& appVersion);
bool fromJson(const QJsonObject& root, MissionPlan* outPlan, QString* outError);
LoadResult loadFromFile(const QString& path);
bool saveToFile(const QString& path, const MissionPlan& plan, const QString& appVersion, QString* outError = nullptr);

QJsonObject itemToJson(const MissionItem& item);
bool itemFromJson(const QJsonObject& obj, MissionItem* outItem, QString* outError);
QJsonObject rallyToJson(const RallyItem& item);
QJsonObject fenceToJson(const FencePolygon& fence);
bool fenceFromJson(const QJsonObject& obj, FencePolygon* out, QString* outError);
bool knownItemKey(const QString& type, const QString& key);
bool isOpaqueUri(const QString& pathOrUri);

} // namespace FileIo
} // namespace mission
