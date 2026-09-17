#pragma once

#include <QObject>
#include <QHash>
#include <QList>
#include <QPointer>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonArray>
#include <QString>
#include <QTimer>
#include <QVariant>
#include <QMetaMethod>
#include <QMetaType>

#include "control_defs.h"

class UiProbe;

class SignalTrap : public QObject
{
    Q_OBJECT
public:
    explicit SignalTrap(QObject* parent = nullptr) : QObject(parent) {}
    bool fired = false;

public slots:
    void fire() { fired = true; }
};

class CommandDispatcher : public QObject
{
    Q_OBJECT
public:
    explicit CommandDispatcher(QObject* parent = nullptr);

    void registerObject(const QString& name, QObject* object);
    void setUiProbe(UiProbe* probe);
    void setArtifactRoot(const QString& dir);

public slots:
    void dispatch(int connId, const QJsonObject& command);

signals:
    void taken(int connId, const QJsonValue& id);
    void reply(int connId, const QJsonObject& reply);

private:
    struct Resolved {
        QObject*    object = nullptr;
        QString     leaf;
        QStringList subPath;
        QString     error;
    };
    struct Handle {
        bool    done = false;
        bool    ok = true;
        QString code;
        QString error;
    };
    struct Waiter {
        int                 connId = 0;
        QJsonValue          id;
        QJsonValue          until;
        QStringList         handles;
        QString             ownHandle;
        QPointer<SignalTrap> trap;
        qint64              deadlineMs = 0;
    };
    struct Outcome {
        bool       ok = true;
        QString    code;
        QString    error;
        QJsonValue result;
    };

    Outcome opDescribe(const QJsonObject& cmd);
    Outcome opGet(const QJsonObject& cmd);
    Outcome opSet(const QJsonObject& cmd);
    Outcome opCall(const QJsonObject& cmd);
    Outcome opDump(const QJsonObject& cmd);
    bool    opWait(int connId, const QJsonValue& id, const QJsonObject& cmd);
    bool    opStart(int connId, const QJsonValue& id, const QJsonObject& cmd);
    bool    opJoin(int connId, const QJsonValue& id, const QJsonObject& cmd);

    Resolved resolve(const QString& path) const;
    Outcome  invoke(QObject* object, const QString& method, const QJsonArray& args);
    QJsonObject describeObject(QObject* object) const;

    bool evaluateUntil(const QJsonValue& until, bool& satisfied, QString& error) const;
    bool evaluatePredicate(const QJsonObject& pred, bool& satisfied, QString& error) const;
    bool armSignal(Waiter& w, const QJsonObject& spec, QString& error);
    static bool isSignalUntil(const QJsonValue& until);
    static bool descend(QVariant& value, const QStringList& subPath, QString& error);
    void pollWaiters();
    void finish(int connId, const QJsonValue& id, const Outcome& out, const QJsonObject& extra = {});
    void finishWaiter(const Waiter& w, const Outcome& out);
    void markHandle(const QString& handle, const Outcome& out);
    void pruneHandles();
    QString absoluteArtifactPath(const QString& file) const;

    static QJsonValue toJson(const QVariant& value);
    static QVariant   fromJson(const QJsonValue& value, QMetaType target, bool& ok);
    static bool       compare(const QVariant& actual, const QJsonValue& expected, const QString& op, bool& result);

    QHash<QString, QPointer<QObject>> roots_;
    QPointer<UiProbe>                 uiProbe_;
    QHash<QString, Handle>            handles_;
    QList<Waiter>                     waiters_;
    QTimer                            pollTimer_;
    int                               nextHandle_ = 1;
    QString                           artifactRoot_;

    static constexpr int kPollMs = 50;
    static constexpr int kMaxArgs = 8;
    static constexpr int kMaxHandles = 256;
};
