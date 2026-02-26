#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QPointer>
#include <QTimer>
#include <QUrl>

class QNetworkReply;
class InternetManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool internetAvailable READ isInternetAvailable NOTIFY internetAvailabilityChanged)

public:
    explicit InternetManager(QObject* parent = nullptr);
    bool isInternetAvailable() const;

public slots:
    void start();
    void stop();

signals:
    void internetAvailabilityChanged(bool available);

private slots:
    void pollInternetAvailability();
    void onProbeFinished();
    void onProbeTimeout();

private:
    void setInternetAvailable(bool available);
    void startProbe();
    void stopProbe();

    bool internetAvailable_;
    bool networkInfoBackendLoaded_;
    bool started_;
    QNetworkAccessManager networkAccessManager_;
    QPointer<QNetworkReply> probeReply_;
    QTimer pollTimer_;
    QTimer probeTimeoutTimer_;

    static const QUrl probeUrl_;
};
