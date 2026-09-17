#include "control_listener.h"
#include "control_defs.h"

#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonParseError>
#include <QStringList>
#include <QDebug>
#include <utility>
#include <algorithm>

ControlListener::ControlListener(QObject* parent)
    : QObject(parent)
{
}

ControlListener::~ControlListener()
{
    close();
}

void ControlListener::listen(const QHostAddress& address, quint16 port, const QString& token)
{
    if (server_) {
        emit listenFailed(QStringLiteral("already listening"));
        return;
    }

    token_ = token;
    requireToken_ = !address.isLoopback();
    if (requireToken_ && token_.isEmpty()) {
        emit listenFailed(QStringLiteral("non-loopback bind %1 requires a token").arg(address.toString()));
        return;
    }

    server_ = new QTcpServer(this);
    connect(server_, &QTcpServer::newConnection, this, &ControlListener::onNewConnection);

    if (!server_->listen(address, port)) {
        const QString err = server_->errorString();
        server_->deleteLater();
        server_ = nullptr;
        emit listenFailed(QStringLiteral("listen %1:%2 failed: %3").arg(address.toString()).arg(port).arg(err));
        return;
    }

    emit listening(server_->serverAddress(), server_->serverPort());
}

void ControlListener::close()
{
    for (auto it = pending_.begin(); it != pending_.end(); ++it) {
        if (it->watchdog) {
            it->watchdog->stop();
            it->watchdog->deleteLater();
        }
    }
    pending_.clear();

    for (auto it = connections_.begin(); it != connections_.end(); ++it) {
        if (it->socket) {
            it->socket->disconnect(this);
            it->socket->close();
            it->socket->deleteLater();
        }
    }
    connections_.clear();

    if (server_) {
        server_->close();
        server_->deleteLater();
        server_ = nullptr;
    }
}

void ControlListener::onNewConnection()
{
    while (server_ && server_->hasPendingConnections()) {
        QTcpSocket* socket = server_->nextPendingConnection();
        if (!socket) {
            break;
        }
        if (connections_.size() >= kMaxConnections) {
            socket->disconnectFromHost();
            socket->deleteLater();
            continue;
        }
        const int connId = nextConnId_++;
        Connection conn;
        conn.socket = socket;
        conn.authenticated = !requireToken_;
        connections_.insert(connId, conn);

        connect(socket, &QTcpSocket::readyRead,    this, [this, connId]() { onReadyRead(connId); });
        connect(socket, &QTcpSocket::disconnected, this, [this, connId]() { onDisconnected(connId); });

        if (requireToken_) {
            QTimer::singleShot(kAuthTimeoutMs, socket, [this, connId]() {
                auto it = connections_.find(connId);
                if (it != connections_.end() && !it->authenticated && it->socket) {
                    writeError(connId, QJsonValue(), QStringLiteral("unauthorized"), QStringLiteral("token not received in time"));
                    it->socket->disconnectFromHost();
                }
            });
        }
    }
}

void ControlListener::onReadyRead(int connId)
{
    auto it = connections_.find(connId);
    if (it == connections_.end() || !it->socket) {
        return;
    }

    it->buffer.append(it->socket->readAll());

    const int limit = it->authenticated ? kMaxLineBytes : kMaxUnauthBytes;
    if (it->buffer.size() > limit && it->buffer.indexOf('\n') < 0) {
        writeError(connId, QJsonValue(), QStringLiteral("bad-args"), QStringLiteral("line exceeds %1 bytes").arg(limit));
        it->buffer.clear();
        it->socket->disconnectFromHost();
        return;
    }

    int nl = -1;
    while ((nl = it->buffer.indexOf('\n')) >= 0) {
        const QByteArray line = it->buffer.left(nl).trimmed();
        it->buffer.remove(0, nl + 1);
        if (!line.isEmpty()) {
            handleLine(connId, line);
        }
        it = connections_.find(connId);
        if (it == connections_.end()) {
            return;
        }
    }
}

void ControlListener::onDisconnected(int connId)
{
    auto it = connections_.find(connId);
    if (it == connections_.end()) {
        return;
    }
    if (it->socket) {
        it->socket->deleteLater();
    }
    connections_.erase(it);

    QStringList stale;
    for (auto p = pending_.cbegin(); p != pending_.cend(); ++p) {
        if (p->connId == connId) {
            stale.append(p.key());
        }
    }
    for (const QString& key : std::as_const(stale)) {
        dropPending(key);
    }
}

void ControlListener::handleLine(int connId, const QByteArray& line)
{
    auto conn = connections_.find(connId);
    if (conn == connections_.end()) {
        return;
    }

    QJsonParseError parseError{};
    const QJsonDocument doc = QJsonDocument::fromJson(line, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        writeError(connId, QJsonValue(), QStringLiteral("bad-json"), parseError.errorString());
        if (!conn->authenticated) {
            conn->socket->disconnectFromHost();
        }
        return;
    }
    if (!doc.isObject()) {
        writeError(connId, QJsonValue(), QStringLiteral("bad-json"), QStringLiteral("command must be a JSON object"));
        if (!conn->authenticated) {
            conn->socket->disconnectFromHost();
        }
        return;
    }
    const QJsonObject obj = doc.object();

    if (!conn->authenticated) {
        if (obj.value(QStringLiteral("token")).toString() == token_) {
            conn->authenticated = true;
            write(connId, QJsonObject{{QStringLiteral("phase"), QStringLiteral("authenticated")}});
        } else {
            writeError(connId, QJsonValue(), QStringLiteral("unauthorized"), QStringLiteral("token required"));
            conn->socket->disconnectFromHost();
        }
        return;
    }

    const QJsonValue id = obj.value(QStringLiteral("id"));
    if (id.isUndefined() || id.isNull()) {
        writeError(connId, QJsonValue(), QStringLiteral("bad-args"), QStringLiteral("missing id"));
        return;
    }
    if (!obj.contains(QStringLiteral("op"))) {
        writeError(connId, id, QStringLiteral("bad-args"), QStringLiteral("missing op"));
        return;
    }

    if (queuedCount(connId) >= kMaxQueued || inFlightCount(connId) >= kMaxInFlight) {
        write(connId, QJsonObject{{QStringLiteral("id"), id}, {QStringLiteral("phase"), QStringLiteral("busy")}});
        return;
    }

    const QString key = pendingKey(connId, id);
    if (pending_.contains(key)) {
        writeError(connId, id, QStringLiteral("bad-args"), QStringLiteral("duplicate id"));
        return;
    }

    qint64 timeoutMs = obj.value(QStringLiteral("timeout")).toInteger(control::kDefaultTimeoutMs);
    if (timeoutMs <= 0) {
        timeoutMs = control::kDefaultTimeoutMs;
    }
    timeoutMs = std::min<qint64>(timeoutMs, control::kMaxTimeoutMs);

    Pending pend;
    pend.connId = connId;
    pend.watchdog = new QTimer(this);
    pend.watchdog->setSingleShot(true);
    connect(pend.watchdog, &QTimer::timeout, this, [this, key, connId, id]() {
        auto p = pending_.find(key);
        if (p == pending_.end()) {
            return;
        }
        p->timedOut = true;
        write(connId, QJsonObject{{QStringLiteral("id"), id},
                                  {QStringLiteral("phase"), QStringLiteral("timeout")},
                                  {QStringLiteral("taken"), p->taken}});
    });
    pend.watchdog->start(static_cast<int>(timeoutMs + kWatchdogMarginMs));
    pending_.insert(key, pend);

    write(connId, QJsonObject{{QStringLiteral("id"), id}, {QStringLiteral("phase"), QStringLiteral("accepted")}});
    emit commandReceived(connId, obj);
}

void ControlListener::onTaken(int connId, const QJsonValue& id)
{
    auto p = pending_.find(pendingKey(connId, id));
    if (p != pending_.end()) {
        p->taken = true;
    }
}

void ControlListener::sendReply(int connId, const QJsonObject& reply)
{
    const QJsonValue id = reply.value(QStringLiteral("id"));
    const QString key = pendingKey(connId, id);
    QJsonObject out = reply;

    auto p = pending_.find(key);
    if (p != pending_.end()) {
        if (p->timedOut) {
            out.insert(QStringLiteral("late"), true);
        }
        if (out.value(QStringLiteral("phase")).toString() == QLatin1String("done")) {
            dropPending(key);
        }
    }
    write(connId, out);
}

void ControlListener::write(int connId, const QJsonObject& obj)
{
    auto it = connections_.find(connId);
    if (it == connections_.end() || !it->socket) {
        return;
    }
    it->socket->write(QJsonDocument(obj).toJson(QJsonDocument::Compact));
    it->socket->write("\n", 1);
}

void ControlListener::writeError(int connId, const QJsonValue& id, const QString& code, const QString& error)
{
    QJsonObject obj{{QStringLiteral("phase"), QStringLiteral("done")},
                    {QStringLiteral("ok"), false},
                    {QStringLiteral("code"), code},
                    {QStringLiteral("error"), error}};
    if (!id.isUndefined() && !id.isNull()) {
        obj.insert(QStringLiteral("id"), id);
    }
    write(connId, obj);
}

void ControlListener::dropPending(const QString& key)
{
    auto p = pending_.find(key);
    if (p == pending_.end()) {
        return;
    }
    if (p->watchdog) {
        p->watchdog->stop();
        p->watchdog->deleteLater();
    }
    pending_.erase(p);
}

int ControlListener::queuedCount(int connId) const
{
    int n = 0;
    for (auto p = pending_.cbegin(); p != pending_.cend(); ++p) {
        if (p->connId == connId && !p->taken) {
            ++n;
        }
    }
    return n;
}

int ControlListener::inFlightCount(int connId) const
{
    int n = 0;
    for (auto p = pending_.cbegin(); p != pending_.cend(); ++p) {
        if (p->connId == connId) {
            ++n;
        }
    }
    return n;
}

QString ControlListener::pendingKey(int connId, const QJsonValue& id)
{
    return QString::number(connId) + QLatin1Char(':') + QJsonDocument(QJsonArray{id}).toJson(QJsonDocument::Compact);
}
