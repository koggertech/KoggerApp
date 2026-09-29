#pragma once

#include <QHash>

#include "mission_generators.h"
#include "mission_model.h"

namespace mission {

enum class IssueLevel : int {
    Warning = 1,
    Error = 2
};

struct PlanIssue {
    QString itemId;
    QString text;
    IssueLevel level = IssueLevel::Warning;
};

struct ExpandResult {
    FlatMission mission;
    QHash<QString, GeneratedPath> generated;
    QVector<PlanIssue> issues;
    MissionEstimates estimates;

    QStringList warnings() const;
};

ExpandResult expandPlan(const MissionPlan& plan);
MissionEstimates estimateMission(const FlatMission& mission, const PlanSettings& settings);

} // namespace mission
