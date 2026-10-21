#pragma once

#include <QJsonObject>
#include <QString>

#include "mission_model.h"

namespace mission {
namespace Export {

QJsonObject toPlanJson(const FlatMission& mission, const PlanSettings& settings, const QString& appVersion);
QString toWpl(const FlatMission& mission);

bool savePlanFile(const QString& path, const FlatMission& mission, const PlanSettings& settings, const QString& appVersion, QString* outError = nullptr);
bool saveWplFile(const QString& path, const FlatMission& mission, QString* outError = nullptr);

} // namespace Export
} // namespace mission
