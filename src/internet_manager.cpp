#include "internet_manager.h"

#include <QNetworkInformation>
#include <QNetworkReply>
#include <QNetworkRequest>

const QUrl InternetManager::probeUrl_ = QUrl(QStringLiteral("8.8.8.8")); // fallback, if reachability == Unknown "http://cp.cloudflare.com/generate_204"

InternetManager::InternetManager(QObject* parent) :
    QObject(parent),
    internetAvailable_(false),
    networkInfoBackendLoaded_(false),
    started_(false),
    networkAccessManager_(this)
{
    pollTimer_.setParent(this);
    pollTimer_.setInterval(1000);
    pollTimer_.setSingleShot(false);
    QObject::connect(&pollTimer_, &QTimer::timeout, this, &InternetManager::pollInternetAvailability, Qt::AutoConnection);

    probeTimeoutTimer_.setParent(this);
    probeTimeoutTimer_.setInterval(2000);
    probeTimeoutTimer_.setSingleShot(true);
    QObject::connect(&probeTimeoutTimer_, &QTimer::timeout, this, &InternetManager::onProbeTimeout, Qt::AutoConnection);
}

bool InternetManager::isInternetAvailable() const
{
    return internetAvailable_;
}

void InternetManager::start()
{
    if (started_) {
        return;
    }

    started_ = true;
    networkInfoBackendLoaded_ = QNetworkInformation::loadDefaultBackend();

    if (networkInfoBackendLoaded_) {
        if (QNetworkInformation* networkInfo = QNetworkInformation::instance(); networkInfo) {
            QObject::connect(networkInfo, &QNetworkInformation::reachabilityChanged, this, &InternetManager::pollInternetAvailability, Qt::UniqueConnection);
        }
    }

    pollTimer_.start();
    QMetaObject::invokeMethod(this, "pollInternetAvailability", Qt::QueuedConnection);
}

void InternetManager::stop()
{
    started_ = false;
    pollTimer_.stop();
    probeTimeoutTimer_.stop();
    stopProbe();
}

void InternetManager::pollInternetAvailability()
{
    if (!started_) {
        return;
    }

    if (networkInfoBackendLoaded_) {
        if (const QNetworkInformation* networkInfo = QNetworkInformation::instance(); networkInfo) {
            const auto reachability = networkInfo->reachability();
            if (reachability != QNetworkInformation::Reachability::Unknown) {
                setInternetAvailable(reachability == QNetworkInformation::Reachability::Online);
                return;
            }
        }
    }

    startProbe();
}

void InternetManager::onProbeFinished()
{
    if (!probeReply_) {
        return;
    }

    QNetworkReply* reply = probeReply_;
    probeReply_.clear();
    probeTimeoutTimer_.stop();

    bool available = false;
    const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (reply->error() == QNetworkReply::NoError && statusCode >= 200 && statusCode < 400) {
        available = true;
    }

    reply->deleteLater();
    setInternetAvailable(available);
}

void InternetManager::onProbeTimeout()
{
    stopProbe();
    setInternetAvailable(false);
}

void InternetManager::setInternetAvailable(bool available)
{
    if (internetAvailable_ == available) {
        return;
    }

    internetAvailable_ = available;
    emit internetAvailabilityChanged(internetAvailable_);
}

void InternetManager::startProbe()
{
    if (probeReply_) {
        return;
    }

    QNetworkRequest request(probeUrl_);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setTransferTimeout(1500);
    request.setRawHeader("User-Agent", "QtNetwork/1.0");

    probeReply_ = networkAccessManager_.get(request);
    QObject::connect(probeReply_, &QNetworkReply::finished, this, &InternetManager::onProbeFinished, Qt::AutoConnection);
    probeTimeoutTimer_.start();
}

void InternetManager::stopProbe()
{
    if (!probeReply_) {
        return;
    }

    probeReply_->abort();
    probeReply_->deleteLater();
    probeReply_.clear();
}
