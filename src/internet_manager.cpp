#include "internet_manager.h"

#include <QNetworkInformation>

InternetManager::InternetManager(QObject* parent) :
    QObject(parent),
    internetAvailable_(false),
    everAvailabilitySet_(false),
    metered_(false),
    meteredSupported_(false),
    started_(false)
{
}

bool InternetManager::isInternetAvailable() const
{
    return internetAvailable_;
}

bool InternetManager::isMetered() const
{
    return metered_;
}

void InternetManager::start()
{
    if (started_) {
        return;
    }

    started_ = true;

    if (QNetworkInformation::loadDefaultBackend()) {
        if (QNetworkInformation* networkInfo = QNetworkInformation::instance(); networkInfo) {
            QObject::connect(networkInfo, &QNetworkInformation::reachabilityChanged, this, &InternetManager::refreshFromBackend, Qt::UniqueConnection);
            meteredSupported_ = networkInfo->supportedFeatures().testFlag(QNetworkInformation::Feature::Metered);
            if (meteredSupported_) {
                QObject::connect(networkInfo, &QNetworkInformation::isMeteredChanged, this, &InternetManager::refreshFromBackend, Qt::UniqueConnection);
            }
        }
    }

    refreshFromBackend();
}

void InternetManager::stop()
{
    started_ = false;
}

void InternetManager::refreshFromBackend()
{
    if (!started_) {
        return;
    }

    const QNetworkInformation* networkInfo = QNetworkInformation::instance();

    if (meteredSupported_ && networkInfo) {
        setMetered(networkInfo->isMetered());
    }

    // Pure event-driven reachability. When the backend can't determine it
    // (Unknown, or no backend at all) assume available so we never falsely
    // block — the tile downloader fails gracefully if there is truly no net.
    bool available = true;
    if (networkInfo) {
        const auto reachability = networkInfo->reachability();
        if (reachability != QNetworkInformation::Reachability::Unknown) {
            available = (reachability == QNetworkInformation::Reachability::Online);
        }
    }
    setInternetAvailable(available);
}

void InternetManager::setInternetAvailable(bool available)
{
    if (everAvailabilitySet_ && internetAvailable_ == available) {
        return;
    }

    everAvailabilitySet_ = true;
    internetAvailable_ = available;
    emit internetAvailabilityChanged(internetAvailable_);
}

void InternetManager::setMetered(bool metered)
{
    if (metered_ == metered) {
        return;
    }

    metered_ = metered;
    emit meteredChanged(metered_);
}
