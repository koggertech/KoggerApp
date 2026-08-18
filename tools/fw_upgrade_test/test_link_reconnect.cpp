
#include <cstdio>

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QList>
#include <QSerialPortInfo>
#include <QStandardPaths>
#include <QString>
#include <QTimer>

#include "core.h"
#include "link/link_manager.h"
#include "notifications.h"

Core core;
Notifications notifications;

Notifications::Notifications(QObject* parent) : QObject(parent) {}
void Notifications::info(const QString& text, const QString& actionPath)
{ emit messageRequested(0, text, QString(), actionPath); }
void Notifications::warning(const QString& text, const QString& tag, const QString& actionPath)
{ emit messageRequested(1, text, tag, actionPath); }
void Notifications::dismiss(const QString& tag)
{ emit dismissRequested(tag); }

namespace {

const char* kPort = "COM_TEST_UPGRADE";

constexpr int kBootWindowMs = 5000;

constexpr int kReenumerationMs = 1500;

int gChecks = 0;
int gFailures = 0;

void check(bool ok, const QString& what, const QString& detail = QString())
{
    ++gChecks;
    if (ok) {
        std::printf("    ok   %s\n", qUtf8Printable(what));
    } else {
        ++gFailures;
        std::printf("  FAIL   %s%s\n", qUtf8Printable(what),
                    detail.isEmpty() ? "" : qUtf8Printable(QString("  [%1]").arg(detail)));
    }
}

class ScriptedLinkManager : public LinkManager
{
public:
    void setPortPresent(bool present) { present_ = present; }

protected:
    QStringList currentSerialPortNames() const override
    {
        QStringList out;
        if (present_) out.append(QString::fromLatin1(kPort));
        return out;
    }

private:
    bool present_ = true;
};

void pump(int ms)
{
    QEventLoop loop;
    QTimer::singleShot(ms, &loop, &QEventLoop::quit);
    loop.exec();
}

}

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

    QStandardPaths::setTestModeEnabled(true);

    ScriptedLinkManager manager;

    std::printf("== usb-vcp-dropout ==\n");
    std::printf("    ..   re-enumeration %d ms, bootloader window %d ms\n",
                kReenumerationMs, kBootWindowMs);

    manager.setPortPresent(true);
    manager.createAndStartTimer();
    pump(700);

    Link* link = manager.getLinkPtr(QUuid::createUuidV3(QUuid{}, QString::fromLatin1(kPort)));
    check(link != nullptr, "auto-discovery created a link for the port");
    if (!link) return 1;

    QElapsedTimer sinceUpgradeStart;
    QList<qint64> attemptsAt;
    QObject::connect(link, &Link::connectionStatusChanged, &app, [&](QUuid) {
        if (sinceUpgradeStart.isValid()) attemptsAt.append(sinceUpgradeStart.elapsed());
    });

    sinceUpgradeStart.start();
    link->onStartUpgradingFirmware();
    check(link->getIsUpgradingState(), "link enters the upgrading state");
    check(link->getAutoConnOnce(), "link is armed to reconnect");

    manager.setPortPresent(false);
    pump(kReenumerationMs);

    check(manager.getLinkPtr(link->getUuid()) != nullptr,
          "link is not deleted while the port is away and an upgrade is in flight");
    const int attemptsWhileAbsent = attemptsAt.size();

    manager.setPortPresent(true);
    pump(kBootWindowMs - kReenumerationMs);

    qint64 firstAfterReturn = -1;
    for (qint64 t : attemptsAt) {
        if (t >= kReenumerationMs) { firstAfterReturn = t; break; }
    }

    QString timeline;
    for (qint64 t : attemptsAt) timeline += QString::number(t) + "ms ";
    std::printf("    ..   open attempts at: %s(port returned at %d ms)\n",
                timeline.isEmpty() ? "none " : qUtf8Printable(timeline), kReenumerationMs);

    check(firstAfterReturn >= 0,
          "manager reopens the link once the port comes back during an upgrade",
          QString("no open was attempted after the port returned; %1 attempt(s) were spent "
                  "while it was still absent and could not have succeeded").arg(attemptsWhileAbsent));

    check(firstAfterReturn >= 0 && firstAfterReturn < kBootWindowMs,
          QString("and does it inside the bootloader's %1 ms window").arg(kBootWindowMs),
          firstAfterReturn < 0 ? QStringLiteral("never reopened")
                               : QString("first attempt at %1 ms").arg(firstAfterReturn));

    check(link->getIsUpgradingState(),
          "link is still marked upgrading, so it was never quietly dropped");

    manager.stopTimer();

    std::printf("\n---------------------------------------------\n");
    std::printf("%d checks, %d failed\n", gChecks, gFailures);
    return gFailures == 0 ? 0 : 1;
}
