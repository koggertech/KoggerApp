#pragma once

#include <QObject>

// Internet/metered status purely from QNetworkInformation (cross-platform).
// No timers, no polling, no probe requests — we only subscribe to the OS
// backend's change signals and read the current value once on start.
class InternetManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool internetAvailable READ isInternetAvailable NOTIFY internetAvailabilityChanged)
    Q_PROPERTY(bool metered READ isMetered NOTIFY meteredChanged)

public:
    explicit InternetManager(QObject* parent = nullptr);
    bool isInternetAvailable() const;
    bool isMetered() const;

public slots:
    void start();
    void stop();

signals:
    void internetAvailabilityChanged(bool available);
    void meteredChanged(bool metered);

private slots:
    void refreshFromBackend();

private:
    void setInternetAvailable(bool available);
    void setMetered(bool metered);

    bool internetAvailable_;
    bool everAvailabilitySet_;
    bool metered_;
    bool meteredSupported_;
    bool started_;
};
