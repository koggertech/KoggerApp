#pragma once

#include <QObject>
#include <QHash>
#include <QHostAddress>
#include <QJsonObject>
#include <QJsonValue>
#include <QByteArray>
#include <QString>

class QTcpServer;
class QTcpSocket;
class QTimer;

class ControlListener : public QObject
{
    Q_OBJECT
public:
    explicit ControlListener(QObject* parent = nullptr);
    ~ControlListener() override;

public slots:
    void listen(const QHostAddress& address, quint16 port, const QString& token);
    void close();
    void onTaken(int connId, const QJsonValue& id);
    void sendReply(int connId, const QJsonObject& reply);

signals:
    void listening(const QHostAddress& address, quint16 port);
    void listenFailed(const QString& error);
    void commandReceived(int connId, const QJsonObject& command);

private:
    struct Connection {
        QTcpSocket* socket = nullptr;
        QByteArray  buffer;
        bool        authenticated = false;
    };
    struct Pending {
        int     connId = 0;
        QTimer* watchdog = nullptr;
        bool    taken = false;
        bool    timedOut = false;
    };

    void onNewConnection();
    void onReadyRead(int connId);
    void onDisconnected(int connId);
    void handleLine(int connId, const QByteArray& line);
    void write(int connId, const QJsonObject& obj);
    void writeError(int connId, const QJsonValue& id, const QString& code, const QString& error);
    void dropPending(const QString& key);
    int  queuedCount(int connId) const;
    int  inFlightCount(int connId) const;

    static QString pendingKey(int connId, const QJsonValue& id);

    QTcpServer*             server_ = nullptr;
    QString                 token_;
    bool                    requireToken_ = false;
    QHash<int, Connection>  connections_;
    QHash<QString, Pending> pending_;
    int                     nextConnId_ = 1;

    static constexpr int kWatchdogMarginMs = 5000;
    static constexpr int kMaxQueued        = 2;
    static constexpr int kMaxInFlight      = 32;
    static constexpr int kMaxConnections   = 8;
    static constexpr int kMaxLineBytes     = 1 << 20;
    static constexpr int kMaxUnauthBytes   = 4096;
    static constexpr int kAuthTimeoutMs    = 5000;
};
