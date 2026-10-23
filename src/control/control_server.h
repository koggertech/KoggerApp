#pragma once

#include <QObject>
#include <QHostAddress>
#include <QString>
#include <memory>

class QThread;
class ControlListener;
class CommandDispatcher;
class UiProbe;

class ControlServer : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool listening READ listening NOTIFY listeningChanged)
    Q_PROPERTY(int  port      READ port      NOTIFY listeningChanged)
public:
    explicit ControlServer(QObject* parent = nullptr);
    ~ControlServer() override;

    void registerObject(const QString& name, QObject* object);
    void setUiProbe(UiProbe* probe);
    void setArtifactRoot(const QString& dir);

    bool start(const QHostAddress& address, quint16 port, const QString& token);
    void stop();

    bool listening() const { return port_ != 0; }
    int  port() const { return port_; }

    static constexpr quint16 kBasePort = 47800;

signals:
    void listeningChanged();

private:
    std::unique_ptr<QThread>   thread_;
    ControlListener*           listener_ = nullptr;
    CommandDispatcher*         dispatcher_ = nullptr;
    quint16                    port_ = 0;
};
