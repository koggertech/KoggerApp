#include "command_dispatcher.h"

#include "ui_probe.h"

#include <cmath>
#include <climits>
#include <QMetaObject>
#include <QMetaProperty>
#include <QMetaType>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QCoreApplication>
#include <QJsonDocument>
#include <QDebug>
#include <utility>
#include <algorithm>

namespace {

QJsonObject errorReply(const QString& code, const QString& error)
{
    return QJsonObject{{QStringLiteral("ok"), false},
                       {QStringLiteral("code"), code},
                       {QStringLiteral("error"), error}};
}

qint64 nowMs()
{
    return QDateTime::currentMSecsSinceEpoch();
}

} // namespace

CommandDispatcher::CommandDispatcher(QObject* parent)
    : QObject(parent)
{
    pollTimer_.setInterval(kPollMs);
    connect(&pollTimer_, &QTimer::timeout, this, &CommandDispatcher::pollWaiters);
}

void CommandDispatcher::registerObject(const QString& name, QObject* object)
{
    if (name.isEmpty() || !object) {
        return;
    }
    roots_.insert(name, object);
}

void CommandDispatcher::setUiProbe(UiProbe* probe)
{
    uiProbe_ = probe;
}

void CommandDispatcher::setArtifactRoot(const QString& dir)
{
    artifactRoot_ = dir.isEmpty() ? QString() : QDir::cleanPath(QFileInfo(dir).absoluteFilePath());
}

void CommandDispatcher::dispatch(int connId, const QJsonObject& command)
{
    const QJsonValue id = command.value(QStringLiteral("id"));
    emit taken(connId, id);

    const QString op = command.value(QStringLiteral("op")).toString();

    if (op == QLatin1String("describe")) { finish(connId, id, opDescribe(command)); return; }
    if (op == QLatin1String("get"))      { finish(connId, id, opGet(command));      return; }
    if (op == QLatin1String("set"))      { finish(connId, id, opSet(command));      return; }
    if (op == QLatin1String("call"))     { finish(connId, id, opCall(command));     return; }
    if (op == QLatin1String("dump"))     { finish(connId, id, opDump(command));     return; }
    if (op == QLatin1String("wait"))     { opWait(connId, id, command);             return; }
    if (op == QLatin1String("start"))    { opStart(connId, id, command);            return; }
    if (op == QLatin1String("join"))     { opJoin(connId, id, command);             return; }

    Outcome out;
    out.ok = false;
    out.code = QStringLiteral("bad-args");
    out.error = QStringLiteral("unknown op \"%1\"").arg(op);
    finish(connId, id, out);
}

CommandDispatcher::Outcome CommandDispatcher::opDescribe(const QJsonObject& cmd)
{
    Outcome out;
    const QString path = cmd.value(QStringLiteral("path")).toString();

    if (path.isEmpty()) {
        QStringList names = roots_.keys();
        std::sort(names.begin(), names.end());
        QJsonArray roots;
        for (const QString& name : std::as_const(names)) {
            QObject* obj = roots_.value(name);
            if (!obj) {
                continue;
            }
            roots.append(QJsonObject{{QStringLiteral("name"), name},
                                     {QStringLiteral("class"), QString::fromLatin1(obj->metaObject()->className())}});
        }
        out.result = QJsonObject{{QStringLiteral("roots"), roots},
                                 {QStringLiteral("ops"), QJsonArray{"describe", "get", "set", "call", "wait", "start", "join", "dump"}},
                                 {QStringLiteral("until"), QJsonArray{"file-opened", "processing-idle",
                                                                      "{path, eq|ne|gt|lt|ge|le}", "{signal}"}}};
        return out;
    }

    const Resolved r = resolve(path);
    if (!r.object) {
        out.ok = false;
        out.code = QStringLiteral("not-found");
        out.error = r.error;
        return out;
    }
    if (!r.leaf.isEmpty()) {
        out.ok = false;
        out.code = QStringLiteral("not-found");
        out.error = QStringLiteral("\"%1\" is not an object").arg(path);
        return out;
    }
    out.result = describeObject(r.object);
    return out;
}

CommandDispatcher::Outcome CommandDispatcher::opGet(const QJsonObject& cmd)
{
    Outcome out;
    const QString path = cmd.value(QStringLiteral("path")).toString();
    const Resolved r = resolve(path);
    if (!r.object) {
        out.ok = false;
        out.code = QStringLiteral("not-found");
        out.error = r.error;
        return out;
    }
    if (r.leaf.isEmpty()) {
        out.result = describeObject(r.object);
        return out;
    }
    QVariant v = r.object->property(r.leaf.toUtf8().constData());
    if (!v.isValid()) {
        out.ok = false;
        out.code = QStringLiteral("not-found");
        out.error = QStringLiteral("no property \"%1\" on %2").arg(r.leaf, QString::fromLatin1(r.object->metaObject()->className()));
        return out;
    }
    QString error;
    if (!descend(v, r.subPath, error)) {
        out.ok = false;
        out.code = QStringLiteral("not-found");
        out.error = error;
        return out;
    }
    out.result = toJson(v);
    return out;
}

CommandDispatcher::Outcome CommandDispatcher::opSet(const QJsonObject& cmd)
{
    Outcome out;
    const QString path = cmd.value(QStringLiteral("path")).toString();
    const Resolved r = resolve(path);
    if (!r.object || r.leaf.isEmpty() || !r.subPath.isEmpty()) {
        out.ok = false;
        out.code = QStringLiteral("not-found");
        out.error = r.error.isEmpty() ? QStringLiteral("\"%1\" does not name a property").arg(path) : r.error;
        return out;
    }
    const QMetaObject* mo = r.object->metaObject();
    const int idx = mo->indexOfProperty(r.leaf.toUtf8().constData());
    if (idx < 0) {
        out.ok = false;
        out.code = QStringLiteral("not-found");
        out.error = QStringLiteral("no property \"%1\"").arg(r.leaf);
        return out;
    }
    const QMetaProperty prop = mo->property(idx);
    if (!prop.isWritable()) {
        out.ok = false;
        out.code = QStringLiteral("bad-args");
        out.error = QStringLiteral("property \"%1\" is read-only").arg(r.leaf);
        return out;
    }
    bool ok = false;
    const QVariant v = fromJson(cmd.value(QStringLiteral("value")), prop.metaType(), ok);
    if (!ok) {
        out.ok = false;
        out.code = QStringLiteral("bad-args");
        out.error = QStringLiteral("cannot convert value to %1").arg(QString::fromLatin1(prop.metaType().name()));
        return out;
    }
    if (!prop.write(r.object, v)) {
        out.ok = false;
        out.code = QStringLiteral("invoke-failed");
        out.error = QStringLiteral("write to \"%1\" rejected").arg(r.leaf);
        return out;
    }
    out.result = toJson(r.object->property(r.leaf.toUtf8().constData()));
    return out;
}

CommandDispatcher::Outcome CommandDispatcher::opCall(const QJsonObject& cmd)
{
    Outcome out;
    const QString path = cmd.value(QStringLiteral("path")).toString();
    const Resolved r = resolve(path);
    if (!r.object || r.leaf.isEmpty() || !r.subPath.isEmpty()) {
        out.ok = false;
        out.code = QStringLiteral("not-found");
        out.error = r.error.isEmpty() ? QStringLiteral("\"%1\" does not name a method").arg(path) : r.error;
        return out;
    }
    return invoke(r.object, r.leaf, cmd.value(QStringLiteral("args")).toArray());
}

CommandDispatcher::Outcome CommandDispatcher::opDump(const QJsonObject& cmd)
{
    Outcome out;
    if (!uiProbe_) {
        out.ok = false;
        out.code = QStringLiteral("invoke-failed");
        out.error = QStringLiteral("ui probe is not available");
        return out;
    }
    const QString what = cmd.value(QStringLiteral("what")).toString();
    const QString file = cmd.value(QStringLiteral("file")).toString();
    if (file.isEmpty()) {
        out.ok = false;
        out.code = QStringLiteral("bad-args");
        out.error = QStringLiteral("missing file");
        return out;
    }
    const QString abs = absoluteArtifactPath(file);
    if (!artifactRoot_.isEmpty() && !abs.startsWith(artifactRoot_ + QLatin1Char('/'))) {
        out.ok = false;
        out.code = QStringLiteral("bad-args");
        out.error = QStringLiteral("artifact path must be inside %1").arg(artifactRoot_);
        return out;
    }
    const QString dir = QFileInfo(abs).absolutePath();
    if (!QDir().mkpath(dir)) {
        out.ok = false;
        out.code = QStringLiteral("invoke-failed");
        out.error = QStringLiteral("cannot create %1").arg(dir);
        return out;
    }

    bool ok = false;
    if (what == QLatin1String("ui-tree")) {
        ok = uiProbe_->dumpGeometry(abs,
                                    cmd.value(QStringLiteral("objectName")).toString(),
                                    cmd.value(QStringLiteral("root")).toString());
    } else if (what == QLatin1String("shot")) {
        ok = uiProbe_->grabWindow(abs);
    } else {
        out.ok = false;
        out.code = QStringLiteral("bad-args");
        out.error = QStringLiteral("unknown dump \"%1\"").arg(what);
        return out;
    }
    if (!ok) {
        out.ok = false;
        out.code = QStringLiteral("invoke-failed");
        out.error = QStringLiteral("dump %1 failed").arg(what);
        return out;
    }
    out.result = QJsonObject{{QStringLiteral("artifact"), abs}};
    return out;
}

bool CommandDispatcher::opWait(int connId, const QJsonValue& id, const QJsonObject& cmd)
{
    Waiter w;
    w.connId = connId;
    w.id = id;
    w.until = cmd.value(QStringLiteral("until"));
    const int timeout = cmd.value(QStringLiteral("timeout")).toInt(control::kDefaultTimeoutMs);
    w.deadlineMs = nowMs() + (timeout > 0 ? timeout : control::kDefaultTimeoutMs);

    QString error;
    if (isSignalUntil(w.until)) {
        if (!armSignal(w, w.until.toObject(), error)) {
            Outcome out;
            out.ok = false;
            out.code = QStringLiteral("bad-args");
            out.error = error;
            finish(connId, id, out);
            return false;
        }
        waiters_.append(w);
        pollTimer_.start();
        return true;
    }

    bool satisfied = false;
    if (!evaluateUntil(w.until, satisfied, error)) {
        Outcome out;
        out.ok = false;
        out.code = QStringLiteral("bad-args");
        out.error = error;
        finish(connId, id, out);
        return false;
    }
    if (satisfied) {
        finish(connId, id, Outcome{}, QJsonObject{{QStringLiteral("elapsedMs"), 0}});
        return true;
    }
    waiters_.append(w);
    pollTimer_.start();
    return true;
}

bool CommandDispatcher::opStart(int connId, const QJsonValue& id, const QJsonObject& cmd)
{
    const QString path = cmd.value(QStringLiteral("path")).toString();
    const Resolved r = resolve(path);
    if (!r.object || r.leaf.isEmpty() || !r.subPath.isEmpty()) {
        Outcome out;
        out.ok = false;
        out.code = QStringLiteral("not-found");
        out.error = r.error.isEmpty() ? QStringLiteral("\"%1\" does not name a method").arg(path) : r.error;
        finish(connId, id, out);
        return false;
    }

    pruneHandles();
    const QString handle = QStringLiteral("h%1").arg(nextHandle_++);
    handles_.insert(handle, Handle{});

    const QJsonValue until = cmd.value(QStringLiteral("until"));
    Waiter w;
    w.connId = connId;
    w.id = id;
    w.until = until;
    w.ownHandle = handle;
    const int timeout = cmd.value(QStringLiteral("timeout")).toInt(control::kDefaultTimeoutMs);
    w.deadlineMs = nowMs() + (timeout > 0 ? timeout : control::kDefaultTimeoutMs);

    QString error;
    if (isSignalUntil(until) && !armSignal(w, until.toObject(), error)) {
        Outcome out;
        out.ok = false;
        out.code = QStringLiteral("bad-args");
        out.error = error;
        markHandle(handle, out);
        finish(connId, id, out, QJsonObject{{QStringLiteral("handle"), handle}});
        return false;
    }

    const Outcome launched = invoke(r.object, r.leaf, cmd.value(QStringLiteral("args")).toArray());
    if (!launched.ok) {
        if (w.trap) {
            w.trap->deleteLater();
        }
        markHandle(handle, launched);
        finish(connId, id, launched, QJsonObject{{QStringLiteral("handle"), handle}});
        return false;
    }

    if (until.isUndefined() || until.isNull()) {
        markHandle(handle, launched);
        finish(connId, id, launched, QJsonObject{{QStringLiteral("handle"), handle}});
        return true;
    }

    if (w.trap) {
        emit reply(connId, QJsonObject{{QStringLiteral("id"), id},
                                       {QStringLiteral("phase"), QStringLiteral("started")},
                                       {QStringLiteral("handle"), handle}});
        waiters_.append(w);
        pollTimer_.start();
        return true;
    }

    bool satisfied = false;
    if (!evaluateUntil(until, satisfied, error)) {
        Outcome out;
        out.ok = false;
        out.code = QStringLiteral("bad-args");
        out.error = error;
        markHandle(handle, out);
        finish(connId, id, out, QJsonObject{{QStringLiteral("handle"), handle}});
        return false;
    }
    if (satisfied) {
        markHandle(handle, launched);
        finish(connId, id, launched, QJsonObject{{QStringLiteral("handle"), handle}});
        return true;
    }

    emit reply(connId, QJsonObject{{QStringLiteral("id"), id},
                                   {QStringLiteral("phase"), QStringLiteral("started")},
                                   {QStringLiteral("handle"), handle}});
    waiters_.append(w);
    pollTimer_.start();
    return true;
}

bool CommandDispatcher::opJoin(int connId, const QJsonValue& id, const QJsonObject& cmd)
{
    Waiter w;
    w.connId = connId;
    w.id = id;
    const QJsonArray arr = cmd.value(QStringLiteral("handles")).toArray();
    for (const auto& v : arr) {
        w.handles.append(v.toString());
    }
    if (w.handles.isEmpty()) {
        Outcome out;
        out.ok = false;
        out.code = QStringLiteral("bad-args");
        out.error = QStringLiteral("missing handles");
        finish(connId, id, out);
        return false;
    }
    for (const QString& h : std::as_const(w.handles)) {
        if (!handles_.contains(h)) {
            Outcome out;
            out.ok = false;
            out.code = QStringLiteral("not-found");
            out.error = QStringLiteral("unknown handle \"%1\"").arg(h);
            finish(connId, id, out);
            return false;
        }
    }
    const int timeout = cmd.value(QStringLiteral("timeout")).toInt(control::kDefaultTimeoutMs);
    w.deadlineMs = nowMs() + (timeout > 0 ? timeout : control::kDefaultTimeoutMs);
    waiters_.append(w);
    pollTimer_.start();
    pollWaiters();
    return true;
}

CommandDispatcher::Resolved CommandDispatcher::resolve(const QString& path) const
{
    Resolved r;
    const QStringList parts = path.split(QLatin1Char('.'), Qt::SkipEmptyParts);
    if (parts.isEmpty()) {
        r.error = QStringLiteral("empty path");
        return r;
    }

    auto rootIt = roots_.find(parts.first());
    if (rootIt == roots_.end() || !rootIt.value()) {
        r.error = QStringLiteral("unknown root \"%1\"").arg(parts.first());
        return r;
    }

    QObject* cur = rootIt.value();
    for (int i = 1; i < parts.size(); ++i) {
        const QString& seg = parts.at(i);
        const bool last = (i == parts.size() - 1);

        QObject* next = nullptr;
        const QVariant v = cur->property(seg.toUtf8().constData());
        if (v.isValid()) {
            if (v.canConvert<QObject*>()) {
                next = v.value<QObject*>();
            }
        } else {
            next = cur->findChild<QObject*>(seg);
        }

        if (next) {
            cur = next;
            continue;
        }

        r.object = cur;
        r.leaf = seg;
        if (!last) {
            r.subPath = parts.mid(i + 1);
        }
        return r;
    }

    r.object = cur;
    return r;
}

CommandDispatcher::Outcome CommandDispatcher::invoke(QObject* object, const QString& method, const QJsonArray& args)
{
    Outcome out;
    const QMetaObject* mo = object->metaObject();
    const QByteArray name = method.toUtf8();

    QMetaMethod chosen;
    for (int i = 0; i < mo->methodCount(); ++i) {
        const QMetaMethod m = mo->method(i);
        if (m.name() != name) {
            continue;
        }
        if (m.methodType() == QMetaMethod::Signal || m.methodType() == QMetaMethod::Constructor) {
            continue;
        }
        if (m.access() != QMetaMethod::Public) {
            continue;
        }
        if (m.parameterCount() == args.size()) {
            chosen = m;
            break;
        }
    }
    if (!chosen.isValid()) {
        out.ok = false;
        out.code = QStringLiteral("not-found");
        out.error = QStringLiteral("no method \"%1\" with %2 argument(s) on %3")
                        .arg(method).arg(args.size()).arg(QString::fromLatin1(mo->className()));
        return out;
    }
    if (args.size() > kMaxArgs) {
        out.ok = false;
        out.code = QStringLiteral("bad-args");
        out.error = QStringLiteral("too many arguments");
        return out;
    }

    QVector<QVariant> values(args.size());
    for (int i = 0; i < args.size(); ++i) {
        const QMetaType pType = chosen.parameterMetaType(i);
        if (!pType.isValid()) {
            out.ok = false;
            out.code = QStringLiteral("bad-args");
            out.error = QStringLiteral("argument %1: parameter type is not registered with the meta system").arg(i);
            return out;
        }
        bool ok = false;
        values[i] = fromJson(args.at(i), pType, ok);
        if (!ok) {
            out.ok = false;
            out.code = QStringLiteral("bad-args");
            out.error = QStringLiteral("argument %1: cannot convert to %2")
                            .arg(i).arg(QString::fromLatin1(pType.name()));
            return out;
        }
    }

    const QMetaType retType = chosen.returnMetaType();
    void* retStorage = nullptr;
    if (retType.isValid() && retType.id() != QMetaType::Void) {
        retStorage = retType.create();
    }

    void* argv[kMaxArgs + 1] = {nullptr};
    argv[0] = retStorage;
    for (int i = 0; i < args.size(); ++i) {
        argv[i + 1] = chosen.parameterMetaType(i).id() == QMetaType::QVariant
                          ? static_cast<void*>(&values[i])
                          : values[i].data();
    }

    const int rc = QMetaObject::metacall(object, QMetaObject::InvokeMetaMethod, chosen.methodIndex(), argv);
    if (rc >= 0) {
        out.ok = false;
        out.code = QStringLiteral("invoke-failed");
        out.error = QStringLiteral("metacall rejected \"%1\"").arg(method);
    } else if (retStorage) {
        out.result = retType.id() == QMetaType::QVariant
                         ? toJson(*static_cast<QVariant*>(retStorage))
                         : toJson(QVariant(retType, retStorage));
    }

    if (retStorage) {
        retType.destroy(retStorage);
    }
    return out;
}

QJsonObject CommandDispatcher::describeObject(QObject* object) const
{
    const QMetaObject* mo = object->metaObject();
    QJsonObject out;
    out.insert(QStringLiteral("class"), QString::fromLatin1(mo->className()));
    out.insert(QStringLiteral("objectName"), object->objectName());

    QJsonArray props;
    for (int i = 0; i < mo->propertyCount(); ++i) {
        const QMetaProperty p = mo->property(i);
        props.append(QJsonObject{{QStringLiteral("name"), QString::fromLatin1(p.name())},
                                 {QStringLiteral("type"), QString::fromLatin1(p.typeName())},
                                 {QStringLiteral("writable"), p.isWritable()}});
    }
    out.insert(QStringLiteral("properties"), props);

    QJsonArray methods;
    for (int i = 0; i < mo->methodCount(); ++i) {
        const QMetaMethod m = mo->method(i);
        if (m.access() != QMetaMethod::Public) {
            continue;
        }
        QString kind;
        switch (m.methodType()) {
        case QMetaMethod::Signal: kind = QStringLiteral("signal"); break;
        case QMetaMethod::Slot:   kind = QStringLiteral("slot");   break;
        case QMetaMethod::Method: kind = QStringLiteral("invokable"); break;
        default: continue;
        }
        methods.append(QJsonObject{{QStringLiteral("name"), QString::fromLatin1(m.name())},
                                   {QStringLiteral("signature"), QString::fromLatin1(m.methodSignature())},
                                   {QStringLiteral("returns"), QString::fromLatin1(m.typeName())},
                                   {QStringLiteral("kind"), kind}});
    }
    out.insert(QStringLiteral("methods"), methods);

    QJsonArray children;
    for (QObject* c : object->children()) {
        if (c && !c->objectName().isEmpty()) {
            children.append(QJsonObject{{QStringLiteral("objectName"), c->objectName()},
                                        {QStringLiteral("class"), QString::fromLatin1(c->metaObject()->className())}});
        }
    }
    out.insert(QStringLiteral("namedChildren"), children);
    return out;
}

bool CommandDispatcher::evaluateUntil(const QJsonValue& until, bool& satisfied, QString& error) const
{
    if (until.isString()) {
        const QString name = until.toString();
        if (name == QLatin1String("file-opened")) {
            return evaluatePredicate(QJsonObject{{QStringLiteral("path"), QStringLiteral("core.fileOpened")},
                                                 {QStringLiteral("eq"), true}}, satisfied, error);
        }
        if (name == QLatin1String("processing-idle")) {
            return evaluatePredicate(QJsonObject{{QStringLiteral("path"), QStringLiteral("core.processingIdle")},
                                                 {QStringLiteral("eq"), true}}, satisfied, error);
        }
        error = QStringLiteral("unknown until \"%1\"").arg(name);
        return false;
    }
    if (until.isObject()) {
        if (isSignalUntil(until)) {
            error = QStringLiteral("signal until is only valid for wait/start");
            return false;
        }
        return evaluatePredicate(until.toObject(), satisfied, error);
    }
    error = QStringLiteral("until must be a name or a predicate object");
    return false;
}

bool CommandDispatcher::isSignalUntil(const QJsonValue& until)
{
    return until.isObject() && until.toObject().contains(QStringLiteral("signal"));
}

bool CommandDispatcher::armSignal(Waiter& w, const QJsonObject& spec, QString& error)
{
    const QString path = spec.value(QStringLiteral("signal")).toString();
    const Resolved r = resolve(path);
    if (!r.object || r.leaf.isEmpty() || !r.subPath.isEmpty()) {
        error = r.error.isEmpty() ? QStringLiteral("\"%1\" does not name a signal").arg(path) : r.error;
        return false;
    }

    const QMetaObject* mo = r.object->metaObject();
    const QByteArray name = r.leaf.toUtf8();
    QMetaMethod sig;
    for (int i = 0; i < mo->methodCount(); ++i) {
        const QMetaMethod m = mo->method(i);
        if (m.methodType() == QMetaMethod::Signal && m.name() == name) {
            sig = m;
            break;
        }
    }
    if (!sig.isValid()) {
        error = QStringLiteral("no signal \"%1\" on %2").arg(r.leaf, QString::fromLatin1(mo->className()));
        return false;
    }

    auto* trap = new SignalTrap(this);
    const QMetaObject* tmo = trap->metaObject();
    const QMetaMethod slot = tmo->method(tmo->indexOfSlot("fire()"));
    if (!QObject::connect(r.object, sig, trap, slot)) {
        trap->deleteLater();
        error = QStringLiteral("cannot connect to \"%1\"").arg(path);
        return false;
    }
    QObject::connect(r.object, &QObject::destroyed, trap, &QObject::deleteLater);
    w.trap = trap;
    return true;
}

bool CommandDispatcher::descend(QVariant& value, const QStringList& subPath, QString& error)
{
    for (const QString& key : subPath) {
        const int typeId = value.metaType().id();
        if (typeId == QMetaType::QVariantMap || typeId == QMetaType::QVariantHash) {
            const QVariantMap m = value.toMap();
            const auto it = m.constFind(key);
            if (it == m.constEnd()) {
                error = QStringLiteral("no key \"%1\"").arg(key);
                return false;
            }
            value = it.value();
            continue;
        }
        if (typeId == QMetaType::QVariantList || typeId == QMetaType::QStringList) {
            bool ok = false;
            const int idx = key.toInt(&ok);
            const QVariantList l = value.toList();
            if (!ok || idx < 0 || idx >= l.size()) {
                error = QStringLiteral("bad index \"%1\" (size %2)").arg(key).arg(l.size());
                return false;
            }
            value = l.at(idx);
            continue;
        }
        error = QStringLiteral("cannot index \"%1\" into %2").arg(key, QString::fromLatin1(value.typeName()));
        return false;
    }
    return true;
}

bool CommandDispatcher::evaluatePredicate(const QJsonObject& pred, bool& satisfied, QString& error) const
{
    const QString path = pred.value(QStringLiteral("path")).toString();
    const Resolved r = resolve(path);
    if (!r.object || r.leaf.isEmpty()) {
        error = r.error.isEmpty() ? QStringLiteral("\"%1\" does not name a property").arg(path) : r.error;
        return false;
    }
    QVariant actual = r.object->property(r.leaf.toUtf8().constData());
    if (!actual.isValid()) {
        error = QStringLiteral("no property \"%1\"").arg(r.leaf);
        return false;
    }
    if (!descend(actual, r.subPath, error)) {
        return false;
    }

    static const char* ops[] = {"eq", "ne", "gt", "lt", "ge", "le"};
    for (const char* op : ops) {
        const QString key = QString::fromLatin1(op);
        if (pred.contains(key)) {
            if (!compare(actual, pred.value(key), key, satisfied)) {
                error = QStringLiteral("cannot compare %1 with \"%2\"").arg(QString::fromLatin1(actual.typeName()), key);
                return false;
            }
            return true;
        }
    }
    error = QStringLiteral("predicate needs one of eq/ne/gt/lt/ge/le");
    return false;
}

void CommandDispatcher::pollWaiters()
{
    if (waiters_.isEmpty()) {
        pollTimer_.stop();
        return;
    }

    const qint64 now = nowMs();
    QList<Waiter> remaining;
    remaining.reserve(waiters_.size());

    for (const Waiter& w : std::as_const(waiters_)) {
        if (w.trap || (isSignalUntil(w.until) && w.handles.isEmpty())) {
            if (w.trap && w.trap->fired) {
                finishWaiter(w, Outcome{});
                continue;
            }
            if (!w.trap) {
                Outcome out;
                out.ok = false;
                out.code = QStringLiteral("invoke-failed");
                out.error = QStringLiteral("signal source was destroyed");
                finishWaiter(w, out);
                continue;
            }
        } else if (!w.handles.isEmpty()) {
            bool allDone = true;
            bool allOk = true;
            QString firstError;
            for (const QString& h : w.handles) {
                const Handle& hd = handles_.value(h);
                if (!hd.done) {
                    allDone = false;
                    break;
                }
                if (!hd.ok && allOk) {
                    allOk = false;
                    firstError = QStringLiteral("%1: %2").arg(h, hd.error);
                }
            }
            if (allDone) {
                Outcome out;
                out.ok = allOk;
                if (!allOk) {
                    out.code = QStringLiteral("handle-failed");
                    out.error = firstError;
                }
                finishWaiter(w, out);
                continue;
            }
        } else {
            bool satisfied = false;
            QString error;
            if (!evaluateUntil(w.until, satisfied, error)) {
                Outcome out;
                out.ok = false;
                out.code = QStringLiteral("bad-args");
                out.error = error;
                finishWaiter(w, out);
                continue;
            }
            if (satisfied) {
                finishWaiter(w, Outcome{});
                continue;
            }
        }

        if (now >= w.deadlineMs) {
            Outcome out;
            out.ok = false;
            out.code = QStringLiteral("timeout");
            out.error = QStringLiteral("condition not met before deadline");
            finishWaiter(w, out);
            continue;
        }
        remaining.append(w);
    }

    waiters_ = remaining;
    if (waiters_.isEmpty()) {
        pollTimer_.stop();
    }
}

void CommandDispatcher::finish(int connId, const QJsonValue& id, const Outcome& out, const QJsonObject& extra)
{
    QJsonObject r;
    r.insert(QStringLiteral("id"), id);
    r.insert(QStringLiteral("phase"), QStringLiteral("done"));
    r.insert(QStringLiteral("ok"), out.ok);
    if (out.ok) {
        if (!out.result.isUndefined()) {
            r.insert(QStringLiteral("result"), out.result);
        }
    } else {
        r.insert(QStringLiteral("code"), out.code);
        r.insert(QStringLiteral("error"), out.error);
    }
    for (auto it = extra.constBegin(); it != extra.constEnd(); ++it) {
        r.insert(it.key(), it.value());
    }
    emit reply(connId, r);
}

void CommandDispatcher::finishWaiter(const Waiter& w, const Outcome& out)
{
    if (w.trap) {
        w.trap->deleteLater();
    }
    QJsonObject extra;
    if (!w.ownHandle.isEmpty()) {
        markHandle(w.ownHandle, out);
        extra.insert(QStringLiteral("handle"), w.ownHandle);
    }
    finish(w.connId, w.id, out, extra);
}

void CommandDispatcher::markHandle(const QString& handle, const Outcome& out)
{
    auto it = handles_.find(handle);
    if (it == handles_.end()) {
        return;
    }
    it->done = true;
    it->ok = out.ok;
    it->code = out.code;
    it->error = out.error;
}

void CommandDispatcher::pruneHandles()
{
    for (auto it = handles_.begin(); it != handles_.end() && handles_.size() >= kMaxHandles;) {
        if (it->done) {
            it = handles_.erase(it);
        } else {
            ++it;
        }
    }
}

QJsonValue CommandDispatcher::toJson(const QVariant& value)
{
    if (!value.isValid() || value.isNull() || value.metaType().id() == QMetaType::Nullptr) {
        return QJsonValue();
    }
    if (value.canConvert<QObject*>()) {
        QObject* o = value.value<QObject*>();
        if (!o) {
            return QJsonValue();
        }
        return QJsonObject{{QStringLiteral("objectName"), o->objectName()},
                           {QStringLiteral("class"), QString::fromLatin1(o->metaObject()->className())}};
    }
    if (value.metaType().flags().testFlag(QMetaType::IsEnumeration)) {
        return value.toInt();
    }
    QJsonValue direct = QJsonValue::fromVariant(value);
    if (!direct.isUndefined() && !direct.isNull()) {
        return direct;
    }
    QVariant asString = value;
    if (asString.convert(QMetaType(QMetaType::QString))) {
        return asString.toString();
    }
    return QJsonValue(QString::fromLatin1(value.typeName()));
}

QVariant CommandDispatcher::fromJson(const QJsonValue& value, QMetaType target, bool& ok)
{
    QVariant v = value.toVariant();
    if (!target.isValid() || target.id() == QMetaType::QVariant) {
        ok = true;
        return v;
    }
    if (v.metaType() == target) {
        ok = true;
        return v;
    }
    if (target.flags().testFlag(QMetaType::IsEnumeration) && value.isDouble()) {
        const double d = value.toDouble();
        if (d != std::floor(d) || d < INT_MIN || d > INT_MAX) {
            ok = false;
            return QVariant();
        }
        if (target.sizeOf() == sizeof(int)) {
            QVariant e(target);
            *static_cast<int*>(e.data()) = static_cast<int>(d);
            ok = true;
            return e;
        }
    }
    if (v.convert(target)) {
        ok = true;
        return v;
    }
    ok = false;
    return QVariant();
}

bool CommandDispatcher::compare(const QVariant& actual, const QJsonValue& expected, const QString& op, bool& result)
{
    const QVariant exp = expected.toVariant();

    if (op == QLatin1String("eq") || op == QLatin1String("ne")) {
        QVariant a = actual;
        QVariant b = exp;
        if (a.metaType() != b.metaType() && !b.convert(a.metaType())) {
            b = exp;
            if (!a.convert(b.metaType())) {
                return false;
            }
        }
        const bool eq = (a == b);
        result = (op == QLatin1String("eq")) ? eq : !eq;
        return true;
    }

    bool okA = false;
    bool okB = false;
    const double a = actual.toDouble(&okA);
    const double b = exp.toDouble(&okB);
    if (!okA || !okB) {
        return false;
    }
    if (op == QLatin1String("gt")) { result = a >  b; return true; }
    if (op == QLatin1String("lt")) { result = a <  b; return true; }
    if (op == QLatin1String("ge")) { result = a >= b; return true; }
    if (op == QLatin1String("le")) { result = a <= b; return true; }
    return false;
}

QString CommandDispatcher::absoluteArtifactPath(const QString& file) const
{
    const QFileInfo fi(file);
    if (fi.isAbsolute()) {
        return QDir::cleanPath(fi.absoluteFilePath());
    }
    const QDir base = artifactRoot_.isEmpty() ? QDir::current() : QDir(artifactRoot_);
    return QDir::cleanPath(base.absoluteFilePath(file));
}
