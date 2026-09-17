#pragma once

#include <QObject>
#include <QList>
#include <QHash>
#include <QHostAddress>
#include <QVariant>
#include <QUdpSocket>
#include <QTimer>
#include <QByteArray>

#include <memory>

class LinkManagerWrapper;

class LinkDiscovery : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool running READ running NOTIFY runningChanged)
    Q_PROPERTY(QVariantList found READ found NOTIFY foundChanged)
    Q_PROPERTY(QVariantList ports READ ports WRITE setPorts NOTIFY portsChanged)
    Q_PROPERTY(int durationMs READ durationMs WRITE setDurationMs NOTIFY durationMsChanged)

public:
    explicit LinkDiscovery(LinkManagerWrapper* linkManager, QObject* parent = nullptr);
    ~LinkDiscovery() override;

    bool running() const;
    QVariantList found() const;
    QVariantList ports() const;
    void setPorts(const QVariantList& ports);
    int durationMs() const;
    void setDurationMs(int ms);

    Q_INVOKABLE void start();
    Q_INVOKABLE void stop();
    Q_INVOKABLE int adoptAll(bool open);

signals:
    void runningChanged();
    void foundChanged();
    void portsChanged();
    void durationMsChanged();
    void finished(int deviceCount);

private:
    struct DeviceInfo {
        uint8_t route = 0;
        int16_t board = -1;
        uint8_t boardMinor = 0;
        uint32_t serialNumber = 0;
        int fwMajor = 0;
        int fwMinor = 0;
        bool hasVersion0 = false;
        bool hasVersion2 = false;
    };

    struct Host {
        QHostAddress address;
        quint16 devicePort = 0;
        quint16 probePort = 0;
        quint16 replyPort = 0;
        QList<DeviceInfo> devices;
    };

    struct Probe {
        std::unique_ptr<QUdpSocket> socket;
        quint16 port = 0;
    };

    void sendProbes();
    void readProbe(Probe* probe);
    void handleFrame(const QHostAddress& sender, quint16 senderPort, quint16 probePort,
                     const uint8_t* frame, int len);
    void finish();
    void closeProbes();
    static QByteArray buildRequest();
    static QList<QHostAddress> broadcastTargets();
    static QString hostLabel(const Host& host);

    LinkManagerWrapper* linkManager_;
    QList<Probe*> probes_;
    QList<Host> hosts_;
    QList<int> ports_;
    int durationMs_;
    int sendsLeft_;
    QTimer sendTimer_;
    QTimer finishTimer_;
    bool running_;
};
