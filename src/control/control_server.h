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
public:
    explicit ControlServer(QObject* parent = nullptr);
    ~ControlServer() override;

    void registerObject(const QString& name, QObject* object);
    void setUiProbe(UiProbe* probe);
    void setArtifactRoot(const QString& dir);

    bool start(const QHostAddress& address, quint16 port, const QString& token);
    void stop();

    static constexpr quint16 kBasePort = 47800;

private:
    std::unique_ptr<QThread>   thread_;
    ControlListener*           listener_ = nullptr;
    CommandDispatcher*         dispatcher_ = nullptr;
};
