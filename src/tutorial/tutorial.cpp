#include "tutorial.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QJsonValue>
#include <QQuickItem>
#include <QQuickWindow>
#include <QSet>
#include <QSettings>
#include <QtDebug>

#include <cmath>
#include <limits>
#include <span>

namespace {

enum class Kind : uint8_t { Bool, Number, String, Object, Array, Any };

struct FieldSpec {
    const char* name;
    Kind kind;
    const char* choices = nullptr;
    double minimum = -std::numeric_limits<double>::infinity();
    double maximum = std::numeric_limits<double>::infinity();
};

const FieldSpec kScenarioFields[] = {
    {"id", Kind::String},
    {"version", Kind::Number, nullptr, 1.0},
    {"devOnly", Kind::Bool},
    {"title", Kind::String},
    {"defaults", Kind::Object},
    {"steps", Kind::Array},
};

const FieldSpec kDefaultsFields[] = {
    {"enter", Kind::Array},
    {"exit", Kind::Array},
    {"advance", Kind::String, "target|anywhere|condition|button|timer"},
    {"onMissing", Kind::String, "center|skip|abort"},
    {"delayMs", Kind::Number, nullptr, 0.0},
    {"timeoutMs", Kind::Number, nullptr, 0.0},
    {"autoAdvanceMs", Kind::Number, nullptr, 0.0},
    {"dim", Kind::Object},
    {"spotlight", Kind::Object},
    {"arrow", Kind::Object},
    {"bubble", Kind::Object},
    {"buttons", Kind::Object},
};

const FieldSpec kStepFields[] = {
    {"id", Kind::String},
    {"checkpoint", Kind::Bool},
    {"anchor", Kind::String},
    {"title", Kind::String},
    {"text", Kind::String},
    {"comment", Kind::String},
    {"enter", Kind::Array},
    {"exit", Kind::Array},
    {"until", Kind::Object},
    {"next", Kind::String},
    {"advance", Kind::String, "target|anywhere|condition|button|timer"},
    {"onMissing", Kind::String, "center|skip|abort"},
    {"delayMs", Kind::Number, nullptr, 0.0},
    {"timeoutMs", Kind::Number, nullptr, 0.0},
    {"autoAdvanceMs", Kind::Number, nullptr, 0.0},
    {"dim", Kind::Object},
    {"spotlight", Kind::Object},
    {"arrow", Kind::Object},
    {"bubble", Kind::Object},
    {"buttons", Kind::Object},
};

const FieldSpec kDimFields[] = {
    {"enabled", Kind::Bool},
    {"opacity", Kind::Number, nullptr, 0.0, 1.0},
    {"blockInput", Kind::Bool},
};

const FieldSpec kSpotlightFields[] = {
    {"enabled", Kind::Bool},
    {"shape", Kind::String, "roundRect|circle|rect"},
    {"padding", Kind::Number, nullptr, 0.0},
    {"radius", Kind::Number, nullptr, 0.0},
    {"pulse", Kind::Bool},
    {"passThrough", Kind::Bool},
    {"anchors", Kind::Array},
    {"merge", Kind::String, "auto|all|none"},
    {"mergeGap", Kind::Number, nullptr, 0.0},
};

const FieldSpec kArrowFields[] = {
    {"enabled", Kind::Bool},
    {"direction", Kind::String, "auto|up|down|left|right"},
    {"length", Kind::Number, nullptr, 0.0},
    {"offset", Kind::Number},
    {"bounce", Kind::Bool},
};

const FieldSpec kBubbleFields[] = {
    {"enabled", Kind::Bool},
    {"placement", Kind::String, "auto|top|bottom|left|right|center"},
    {"maxWidth", Kind::Number, nullptr, 1.0},
    {"tail", Kind::Bool},
    {"counter", Kind::Bool},
    {"icon", Kind::String},
    {"hint", Kind::Bool},
};

const FieldSpec kButtonsFields[] = {
    {"close", Kind::Bool},
    {"next", Kind::Bool},
    {"back", Kind::Bool},
    {"skip", Kind::Bool},
    {"nextLabel", Kind::String},
    {"backLabel", Kind::String},
    {"skipLabel", Kind::String},
};

const FieldSpec kUntilFields[] = {
    {"on", Kind::String, "store|core"},
    {"property", Kind::String},
    {"equals", Kind::Any},
};

const FieldSpec kActionFields[] = {
    {"on", Kind::String, "store|core"},
    {"call", Kind::String},
    {"args", Kind::Array},
    {"set", Kind::String},
    {"value", Kind::Any},
};

std::span<const FieldSpec> blockFields(const QString& name)
{
    if (name == QLatin1String("dim"))
        return kDimFields;
    if (name == QLatin1String("spotlight"))
        return kSpotlightFields;
    if (name == QLatin1String("arrow"))
        return kArrowFields;
    if (name == QLatin1String("bubble"))
        return kBubbleFields;
    if (name == QLatin1String("buttons"))
        return kButtonsFields;
    return {};
}

bool kindMatches(const QJsonValue& value, Kind kind)
{
    switch (kind) {
    case Kind::Bool:   return value.isBool();
    case Kind::Number: return value.isDouble();
    case Kind::String: return value.isString();
    case Kind::Object: return value.isObject();
    case Kind::Array:  return value.isArray();
    case Kind::Any:    return !value.isUndefined();
    }
    return false;
}

const char* kindName(Kind kind)
{
    switch (kind) {
    case Kind::Bool:   return "bool";
    case Kind::Number: return "number";
    case Kind::String: return "string";
    case Kind::Object: return "object";
    case Kind::Array:  return "array";
    case Kind::Any:    return "value";
    }
    return "value";
}

void validateFields(const QJsonObject& object, std::span<const FieldSpec> fields,
                    const QString& where, QStringList& errors)
{
    for (auto it = object.constBegin(); it != object.constEnd(); ++it) {
        const FieldSpec* spec = nullptr;
        for (const FieldSpec& f : fields) {
            if (it.key() == QLatin1String(f.name)) {
                spec = &f;
                break;
            }
        }
        if (!spec) {
            errors << QStringLiteral("%1: unknown key '%2'").arg(where, it.key());
            continue;
        }
        const QJsonValue value = it.value();
        if (!kindMatches(value, spec->kind)) {
            errors << QStringLiteral("%1.%2: expected %3").arg(where, it.key(), QLatin1String(kindName(spec->kind)));
            continue;
        }
        if (spec->choices) {
            const QStringList choices = QString::fromLatin1(spec->choices).split(QLatin1Char('|'));
            if (!choices.contains(value.toString()))
                errors << QStringLiteral("%1.%2: '%3' is not one of %4")
                              .arg(where, it.key(), value.toString(), choices.join(QLatin1String(", ")));
        }
        if (spec->kind == Kind::Number) {
            const double v = value.toDouble();
            if (v < spec->minimum || v > spec->maximum || !std::isfinite(v))
                errors << QStringLiteral("%1.%2: %3 is out of range").arg(where, it.key()).arg(v);
        }
    }
}

void validateActions(const QJsonArray& actions, const QString& where, QStringList& errors)
{
    for (int i = 0; i < actions.size(); ++i) {
        const QString at = QStringLiteral("%1[%2]").arg(where).arg(i);
        if (!actions.at(i).isObject()) {
            errors << QStringLiteral("%1: expected object").arg(at);
            continue;
        }
        const QJsonObject action = actions.at(i).toObject();
        validateFields(action, kActionFields, at, errors);
        const bool isCall = action.contains(QLatin1String("call"));
        const bool isSet = action.contains(QLatin1String("set"));
        if (isCall == isSet)
            errors << QStringLiteral("%1: needs exactly one of 'call' or 'set'").arg(at);
        if (isCall && action.contains(QLatin1String("value")))
            errors << QStringLiteral("%1: 'value' belongs to 'set'").arg(at);
        if (isSet && action.contains(QLatin1String("args")))
            errors << QStringLiteral("%1: 'args' belongs to 'call'").arg(at);
        if (isSet && !action.contains(QLatin1String("value")))
            errors << QStringLiteral("%1: 'set' needs 'value'").arg(at);
    }
}

void validateStepLike(const QJsonObject& object, std::span<const FieldSpec> fields,
                      const QString& where, QStringList& errors)
{
    validateFields(object, fields, where, errors);
    for (auto it = object.constBegin(); it != object.constEnd(); ++it) {
        const QString key = it.key();
        const QString at = where + QLatin1Char('.') + key;
        if (it.value().isObject()) {
            const auto block = blockFields(key);
            if (!block.empty())
                validateFields(it.value().toObject(), block, at, errors);
        }
        if (key == QLatin1String("until") && it.value().isObject()) {
            const QJsonObject until = it.value().toObject();
            validateFields(until, kUntilFields, at, errors);
            if (!until.contains(QLatin1String("property")) || !until.contains(QLatin1String("equals")))
                errors << QStringLiteral("%1: needs 'property' and 'equals'").arg(at);
        }
        if ((key == QLatin1String("enter") || key == QLatin1String("exit")) && it.value().isArray())
            validateActions(it.value().toArray(), at, errors);
    }
    const QJsonValue spotlight = object.value(QLatin1String("spotlight"));
    if (spotlight.isObject()) {
        const QJsonArray extra = spotlight.toObject().value(QLatin1String("anchors")).toArray();
        for (const auto& v : extra) {
            if (!v.isString() || v.toString().isEmpty())
                errors << QStringLiteral("%1.spotlight.anchors: expected non-empty strings").arg(where);
        }
    }
}

QJsonObject builtinStep()
{
    return QJsonObject{
        {QStringLiteral("anchor"), QString()},
        {QStringLiteral("title"), QString()},
        {QStringLiteral("text"), QString()},
        {QStringLiteral("checkpoint"), false},
        {QStringLiteral("enter"), QJsonArray()},
        {QStringLiteral("exit"), QJsonArray()},
        {QStringLiteral("next"), QString()},
        {QStringLiteral("advance"), QStringLiteral("target")},
        {QStringLiteral("onMissing"), QStringLiteral("center")},
        {QStringLiteral("delayMs"), 0},
        {QStringLiteral("timeoutMs"), 3000},
        {QStringLiteral("autoAdvanceMs"), 0},
        {QStringLiteral("dim"), QJsonObject{
            {QStringLiteral("enabled"), true},
            {QStringLiteral("opacity"), 0.72},
            {QStringLiteral("blockInput"), true}}},
        {QStringLiteral("spotlight"), QJsonObject{
            {QStringLiteral("enabled"), true},
            {QStringLiteral("shape"), QStringLiteral("roundRect")},
            {QStringLiteral("padding"), 6},
            {QStringLiteral("radius"), 8},
            {QStringLiteral("pulse"), true},
            {QStringLiteral("anchors"), QJsonArray()},
            {QStringLiteral("merge"), QStringLiteral("auto")},
            {QStringLiteral("mergeGap"), 32}}},
        {QStringLiteral("arrow"), QJsonObject{
            {QStringLiteral("enabled"), true},
            {QStringLiteral("direction"), QStringLiteral("auto")},
            {QStringLiteral("length"), 44},
            {QStringLiteral("offset"), 6},
            {QStringLiteral("bounce"), true}}},
        {QStringLiteral("bubble"), QJsonObject{
            {QStringLiteral("enabled"), true},
            {QStringLiteral("placement"), QStringLiteral("auto")},
            {QStringLiteral("maxWidth"), 360},
            {QStringLiteral("tail"), true},
            {QStringLiteral("counter"), true},
            {QStringLiteral("icon"), QString()}}},
        {QStringLiteral("buttons"), QJsonObject{
            {QStringLiteral("close"), true},
            {QStringLiteral("next"), false},
            {QStringLiteral("back"), false},
            {QStringLiteral("skip"), false},
            {QStringLiteral("nextLabel"), QString()},
            {QStringLiteral("backLabel"), QString()},
            {QStringLiteral("skipLabel"), QString()}}},
    };
}

QJsonObject deepMerge(QJsonObject base, const QJsonObject& over)
{
    for (auto it = over.constBegin(); it != over.constEnd(); ++it) {
        const QJsonValue current = base.value(it.key());
        if (current.isObject() && it.value().isObject())
            base.insert(it.key(), deepMerge(current.toObject(), it.value().toObject()));
        else
            base.insert(it.key(), it.value());
    }
    return base;
}

QJsonObject mergedStep(const QJsonObject& defaults, const QJsonObject& raw)
{
    QJsonObject merged = deepMerge(deepMerge(builtinStep(), defaults), raw);
    merged.remove(QStringLiteral("comment"));

    const QString advance = merged.value(QLatin1String("advance")).toString();

    QJsonObject spotlight = merged.value(QLatin1String("spotlight")).toObject();
    if (!spotlight.contains(QLatin1String("passThrough")))
        spotlight.insert(QStringLiteral("passThrough"),
                         advance == QLatin1String("target") || advance == QLatin1String("condition"));
    merged.insert(QStringLiteral("spotlight"), spotlight);

    QJsonObject bubble = merged.value(QLatin1String("bubble")).toObject();
    if (!bubble.contains(QLatin1String("hint")))
        bubble.insert(QStringLiteral("hint"), advance == QLatin1String("anywhere"));
    merged.insert(QStringLiteral("bubble"), bubble);

    return merged;
}

QString settingsGroup(const QString& id)
{
    return QStringLiteral("main/tutorial/") + id;
}

}

Tutorial::Tutorial(QObject* parent)
    : QObject(parent)
{
}

void Tutorial::loadFromDirectory(const QString& directory)
{
    const QDir dir(directory);
    const QStringList files = dir.entryList({QStringLiteral("*.json")}, QDir::Files, QDir::Name);
    QSet<QString> ids;
    for (const QString& name : files) {
        Scenario scenario;
        QStringList errors;
        const QString path = dir.filePath(name);
        if (!parseScenario(path, scenario, errors)) {
            for (const QString& e : std::as_const(errors)) {
                const QString message = name + QLatin1String(": ") + e;
                qWarning().noquote() << "Tutorial:" << message;
                loadErrors_ << message;
            }
            continue;
        }
        if (ids.contains(scenario.id)) {
            const QString message = name + QLatin1String(": duplicate scenario id '") + scenario.id + QLatin1Char('\'');
            qWarning().noquote() << "Tutorial:" << message;
            loadErrors_ << message;
            continue;
        }
        ids.insert(scenario.id);
        progress_.insert(scenario.id, readStoredProgress(scenario));
        scenarios_.append(scenario);
    }
    emit scenariosChanged();
}

void Tutorial::setWindow(QQuickWindow* window)
{
    window_ = window;
}

bool Tutorial::parseScenario(const QString& filePath, Scenario& out, QStringList& errors) const
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        errors << QStringLiteral("cannot open");
        return false;
    }
    QJsonParseError parseError{};
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        errors << QStringLiteral("JSON error at %1: %2").arg(parseError.offset).arg(parseError.errorString());
        return false;
    }
    if (!doc.isObject()) {
        errors << QStringLiteral("root must be an object");
        return false;
    }

    const QJsonObject root = doc.object();
    validateFields(root, kScenarioFields, QStringLiteral("scenario"), errors);
    for (const char* key : {"id", "version", "title", "steps"}) {
        if (!root.contains(QLatin1String(key)))
            errors << QStringLiteral("scenario: missing '%1'").arg(QLatin1String(key));
    }
    if (!errors.isEmpty())
        return false;

    out.id = root.value(QLatin1String("id")).toString();
    const QString baseName = QFileInfo(filePath).completeBaseName();
    if (out.id != baseName)
        errors << QStringLiteral("scenario: id '%1' must match file name '%2'").arg(out.id, baseName);

    const double version = root.value(QLatin1String("version")).toDouble();
    if (version != std::floor(version))
        errors << QStringLiteral("scenario.version: must be an integer");
    out.version = static_cast<int>(version);
    out.devOnly = root.value(QLatin1String("devOnly")).toBool(false);
    out.title = root.value(QLatin1String("title")).toString();
    out.defaults = root.value(QLatin1String("defaults")).toObject();
    validateStepLike(out.defaults, kDefaultsFields, QStringLiteral("defaults"), errors);

    const QJsonArray steps = root.value(QLatin1String("steps")).toArray();
    if (steps.isEmpty())
        errors << QStringLiteral("scenario.steps: must not be empty");

    QSet<QString> seen;
    for (int i = 0; i < steps.size(); ++i) {
        const QString where = QStringLiteral("steps[%1]").arg(i);
        if (!steps.at(i).isObject()) {
            errors << QStringLiteral("%1: expected object").arg(where);
            continue;
        }
        QJsonObject step = steps.at(i).toObject();
        validateStepLike(step, kStepFields, where, errors);
        QString id = step.value(QLatin1String("id")).toString();
        if (id.isEmpty()) {
            id = QStringLiteral("#%1").arg(i);
            step.insert(QStringLiteral("id"), id);
        }
        if (seen.contains(id))
            errors << QStringLiteral("%1: duplicate step id '%2'").arg(where, id);
        seen.insert(id);
        out.steps.append(step);
        out.stepIds.append(id);
    }

    for (int i = 0; i < out.steps.size(); ++i) {
        const QString where = QStringLiteral("steps[%1]").arg(i);
        const QJsonObject merged = mergedStep(out.defaults, out.steps.at(i));
        const QString next = merged.value(QLatin1String("next")).toString();
        if (!next.isEmpty() && !out.stepIds.contains(next))
            errors << QStringLiteral("%1.next: unknown step '%2'").arg(where, next);
        const QString advance = merged.value(QLatin1String("advance")).toString();
        if (advance == QLatin1String("condition") && !merged.contains(QLatin1String("until")))
            errors << QStringLiteral("%1: advance 'condition' needs 'until'").arg(where);
        if (advance == QLatin1String("timer") && merged.value(QLatin1String("autoAdvanceMs")).toDouble() <= 0.0)
            errors << QStringLiteral("%1: advance 'timer' needs autoAdvanceMs > 0").arg(where);
    }

    return errors.isEmpty();
}

int Tutorial::indexOfScenario(const QString& id) const
{
    for (int i = 0; i < scenarios_.size(); ++i) {
        if (scenarios_.at(i).id == id)
            return i;
    }
    return -1;
}

QVariantList Tutorial::scenarios() const
{
    QVariantList list;
    list.reserve(scenarios_.size());
    for (const Scenario& s : scenarios_) {
        const Progress progress = loadProgress(s);
        list.append(QVariantMap{
            {QStringLiteral("id"), s.id},
            {QStringLiteral("title"), translate(s, s.title)},
            {QStringLiteral("version"), s.version},
            {QStringLiteral("devOnly"), s.devOnly},
            {QStringLiteral("stepCount"), s.steps.size()},
            {QStringLiteral("completed"), progress.completed},
            {QStringLiteral("resumeStep"), progress.checkpoint.isEmpty() ? -1 : s.stepIds.indexOf(progress.checkpoint)},
        });
    }
    return list;
}

QString Tutorial::scenarioId() const
{
    return active() ? scenarios_.at(activeIndex_).id : QString();
}

int Tutorial::stepCount() const
{
    return active() ? static_cast<int>(scenarios_.at(activeIndex_).steps.size()) : 0;
}

bool Tutorial::start(const QString& id)
{
    const int index = indexOfScenario(id);
    if (index < 0 || active())
        return false;
    return beginScenario(index, 0);
}

bool Tutorial::resume(const QString& id)
{
    const int index = indexOfScenario(id);
    if (index < 0 || active())
        return false;
    const Scenario& s = scenarios_.at(index);
    const int step = s.stepIds.indexOf(loadProgress(s).checkpoint);
    return beginScenario(index, step < 0 ? 0 : step);
}

bool Tutorial::beginScenario(int scenarioIndex, int stepIndex)
{
    activeIndex_ = scenarioIndex;
    stepIndex_ = -1;
    history_.clear();
    emit activeChanged();
    enterStep(stepIndex, false);
    return true;
}

void Tutorial::stop()
{
    if (active())
        endScenario(false);
}

void Tutorial::finish()
{
    if (active())
        endScenario(true);
}

void Tutorial::advance()
{
    if (active())
        moveForward(true);
}

void Tutorial::skipStep()
{
    if (active())
        moveForward(false);
}

void Tutorial::moveForward(bool pushHistory)
{
    const QString next = step_.value(QStringLiteral("next")).toString();
    const int target = next.isEmpty() ? stepIndex_ + 1 : scenarios_.at(activeIndex_).stepIds.indexOf(next);
    if (target >= 0 && target < stepCount())
        enterStep(target, pushHistory);
    else
        endScenario(true);
}

void Tutorial::back()
{
    if (!active() || history_.isEmpty())
        return;
    enterStep(history_.takeLast(), false);
}

bool Tutorial::goTo(const QString& stepId)
{
    if (!active())
        return false;
    const int index = scenarios_.at(activeIndex_).stepIds.indexOf(stepId);
    if (index < 0)
        return false;
    enterStep(index, true);
    return true;
}

void Tutorial::enterStep(int index, bool pushHistory)
{
    if (pushHistory && stepIndex_ >= 0)
        history_.append(stepIndex_);
    stepIndex_ = index;
    ++stepSerial_;
    resetTarget();
    rebuildStep();

    const Scenario& s = scenarios_.at(activeIndex_);
    if (index == 0 || step_.value(QStringLiteral("checkpoint")).toBool()) {
        saveCheckpoint(s, s.stepIds.at(index));
        emit scenariosChanged();
    }

    setPhase(QStringLiteral("preparing"));
    emit stepChanged();
    emit stepSerialChanged();
}

void Tutorial::rebuildStep()
{
    if (!active() || stepIndex_ < 0) {
        step_.clear();
        return;
    }
    const Scenario& s = scenarios_.at(activeIndex_);
    QJsonObject merged = mergedStep(s.defaults, s.steps.at(stepIndex_));

    merged.insert(QStringLiteral("title"), translate(s, merged.value(QLatin1String("title")).toString()));
    merged.insert(QStringLiteral("text"), translate(s, merged.value(QLatin1String("text")).toString()));

    QJsonObject buttons = merged.value(QLatin1String("buttons")).toObject();
    for (const char* key : {"nextLabel", "backLabel", "skipLabel"})
        buttons.insert(QLatin1String(key), translate(s, buttons.value(QLatin1String(key)).toString()));
    merged.insert(QStringLiteral("buttons"), buttons);

    QJsonArray targets;
    const QString primary = merged.value(QLatin1String("anchor")).toString();
    if (!primary.isEmpty())
        targets.append(primary);
    const QJsonArray extraAnchors = merged.value(QLatin1String("spotlight")).toObject().value(QLatin1String("anchors")).toArray();
    for (const auto& v : extraAnchors) {
        if (!targets.contains(v))
            targets.append(v);
    }
    merged.insert(QStringLiteral("targets"), targets);
    merged.insert(QStringLiteral("index"), stepIndex_);
    merged.insert(QStringLiteral("count"), static_cast<int>(s.steps.size()));

    step_ = merged.toVariantMap();
}

void Tutorial::endScenario(bool completed)
{
    const QString id = scenarioId();
    if (completed)
        saveCompleted(scenarios_.at(activeIndex_));
    activeIndex_ = -1;
    stepIndex_ = -1;
    ++stepSerial_;
    history_.clear();
    step_.clear();
    resetTarget();
    setPhase(QStringLiteral("idle"));
    emit stepSerialChanged();
    emit stepChanged();
    emit activeChanged();
    emit scenariosChanged();
    emit finished(id, completed);
}

void Tutorial::setPhase(const QString& phase)
{
    if (phase_ == phase)
        return;
    phase_ = phase;
    emit phaseChanged();
}

void Tutorial::resetTarget()
{
    if (!targetFound_ && targetRect_.isNull())
        return;
    targetFound_ = false;
    targetRect_ = QRectF();
    emit targetChanged();
}

void Tutorial::stepReady(int serial)
{
    if (active() && serial == stepSerial_)
        setPhase(QStringLiteral("showing"));
}

void Tutorial::stepSuspended(int serial)
{
    if (active() && serial == stepSerial_)
        setPhase(QStringLiteral("preparing"));
}

void Tutorial::stepFailed(int serial, const QString& reason)
{
    if (!active() || serial != stepSerial_)
        return;
    const QString id = scenarioId();
    qWarning().noquote() << "Tutorial:" << id << "step" << stepIndex_ << "failed:" << reason;
    endScenario(false);
    emit failed(id, reason);
}

void Tutorial::reportTarget(int serial, bool found, const QRectF& rect)
{
    if (!active() || serial != stepSerial_)
        return;
    const QRectF effective = found ? rect : QRectF();
    if (targetFound_ == found && targetRect_ == effective)
        return;
    targetFound_ = found;
    targetRect_ = effective;
    emit targetChanged();
}

void Tutorial::registerAnchor(const QString& key, QQuickItem* item)
{
    if (key.isEmpty() || !item)
        return;
    auto& list = anchors_[key];
    list.removeIf([](const QPointer<QQuickItem>& p) { return p.isNull(); });
    if (!list.contains(item))
        list.append(item);
    emit anchorsChanged(key);
}

void Tutorial::unregisterAnchor(const QString& key, QQuickItem* item)
{
    auto it = anchors_.find(key);
    if (it == anchors_.end())
        return;
    it->removeIf([item](const QPointer<QQuickItem>& p) { return p.isNull() || p == item; });
    if (it->isEmpty())
        anchors_.erase(it);
    emit anchorsChanged(key);
}

QQuickItem* Tutorial::anchor(const QString& key) const
{
    const auto it = anchors_.constFind(key);
    if (it == anchors_.constEnd())
        return nullptr;
    for (const QPointer<QQuickItem>& p : *it) {
        if (!p || !p->isVisible() || p->width() <= 0.0 || p->height() <= 0.0)
            continue;
        if (window_ && p->window() != window_)
            continue;
        return p.data();
    }
    return nullptr;
}

bool Tutorial::hasAnchor(const QString& key) const
{
    const auto it = anchors_.constFind(key);
    if (it == anchors_.constEnd())
        return false;
    for (const QPointer<QQuickItem>& p : *it) {
        if (p)
            return true;
    }
    return false;
}

void Tutorial::resetProgress(const QString& id)
{
    QSettings settings;
    if (id.isEmpty()) {
        settings.remove(QStringLiteral("main/tutorial"));
        progress_.clear();
    } else {
        settings.remove(settingsGroup(id));
        progress_.remove(id);
    }
    emit scenariosChanged();
}

void Tutorial::reloadProgress()
{
    progress_.clear();
    for (const Scenario& s : std::as_const(scenarios_))
        progress_.insert(s.id, readStoredProgress(s));
    emit scenariosChanged();
}

void Tutorial::retranslate()
{
    if (active()) {
        rebuildStep();
        emit stepChanged();
    }
    emit scenariosChanged();
}

QString Tutorial::translate(const Scenario& scenario, const QString& source) const
{
    if (source.isEmpty())
        return source;
    const QByteArray context = QByteArrayLiteral("tutorial_") + scenario.id.toUtf8();
    const QByteArray text = source.toUtf8();
    return QCoreApplication::translate(context.constData(), text.constData());
}

Tutorial::Progress Tutorial::loadProgress(const Scenario& scenario) const
{
    return progress_.value(scenario.id);
}

Tutorial::Progress Tutorial::readStoredProgress(const Scenario& scenario) const
{
    QSettings settings;
    settings.beginGroup(settingsGroup(scenario.id));
    Progress progress;
    if (settings.value(QStringLiteral("version"), 0).toInt() != scenario.version)
        return progress;
    progress.completed = settings.value(QStringLiteral("completed"), false).toBool();
    progress.checkpoint = settings.value(QStringLiteral("checkpoint")).toString();
    return progress;
}

void Tutorial::saveCheckpoint(const Scenario& scenario, const QString& stepId)
{
    Progress& progress = progress_[scenario.id];
    progress.checkpoint = stepId;
    QSettings settings;
    settings.beginGroup(settingsGroup(scenario.id));
    if (settings.value(QStringLiteral("version"), 0).toInt() != scenario.version)
        settings.remove(QString());
    settings.setValue(QStringLiteral("version"), scenario.version);
    settings.setValue(QStringLiteral("checkpoint"), stepId);
    if (progress.completed)
        settings.setValue(QStringLiteral("completed"), true);
}

void Tutorial::saveCompleted(const Scenario& scenario)
{
    Progress& progress = progress_[scenario.id];
    progress.completed = true;
    progress.checkpoint.clear();
    QSettings settings;
    settings.beginGroup(settingsGroup(scenario.id));
    settings.setValue(QStringLiteral("version"), scenario.version);
    settings.setValue(QStringLiteral("completed"), true);
    settings.remove(QStringLiteral("checkpoint"));
}
