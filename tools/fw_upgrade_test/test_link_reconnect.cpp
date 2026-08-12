// Link-layer half of the firmware-upgrade regression test.
//
// The driver test (test_fw_upgrade.cpp) covers the exchange once host and bootloader can
// talk. This one covers the gap in between, which on an STM32 USB VCP is not optional: the
// MCU resets on ID_BOOT, the USB device detaches, COM11 disappears from Windows for the
// length of a re-enumeration, and the bootloader on the other side is counting down a 5 s
// window for the first ID_UPDATE. Miss it and the device runs the old firmware again.
//
// So the requirement is a deadline, not just "reconnects eventually": the link has to be
// open again early enough that the driver's handshake and first packet still fit inside
// the window.
//
// No hardware. LinkManager's view of the world is scripted through the getCurrentSerialList
// seam, and no port is ever really opened -- an open of a name that owns no device fails,
// which is the point. Open ATTEMPTS are what is counted, with the time each one happened.
//
//   test_link_reconnect.exe

#include <cstdio>

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QList>
#include <QSerialPortInfo>
#include <QStandardPaths>
#include <QString>
#include <QTimer>

#include "core.h"                 // resolves to tools/fw_upgrade_test/shim/core.h
#include "link/link_manager.h"
#include "notifications.h"

Core core;
Notifications notifications;

// src/notifications.cpp is deliberately not compiled into this binary: it reaches Core
// through a quoted #include, and a quoted include is resolved against the including file's
// own directory first -- which would pull in the real src/core.h and with it the whole
// application. The three methods are trivial and toasts are not what this test observes.
Notifications::Notifications(QObject* parent) : QObject(parent) {}
void Notifications::info(const QString& text, const QString& actionPath)
{ emit messageRequested(0, text, QString(), actionPath); }
void Notifications::warning(const QString& text, const QString& tag, const QString& actionPath)
{ emit messageRequested(1, text, tag, actionPath); }
void Notifications::dismiss(const QString& tag)
{ emit dismissRequested(tag); }

namespace {

// Deliberately not a name Windows can assign. The scenario needs every openAsSerial to
// fail, and a real port name would mean the test grabs whatever device is plugged into it
// -- including the Recorder this whole exercise is about.
const char* kPort = "COM_TEST_UPGRADE";

// Measured on the bench: the bootloader runs the existing firmware if no upgrade packet
// arrives within this long after the reboot command.
constexpr int kBootWindowMs = 5000;

// How long the port stays gone while Windows re-enumerates the VCP.
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

// LinkManager with the OS view replaced by a script. Everything else -- the 500 ms tick,
// addNewLinks, deleteMissingLinks, openAutoConnections -- is the production code.
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

} // namespace

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

    // Keep the run away from the user's pinned_links.xml: the first tick imports it, and an
    // imported UDP link would open a real socket.
    QStandardPaths::setTestModeEnabled(true);

    ScriptedLinkManager manager;

    std::printf("== usb-vcp-dropout ==\n");
    std::printf("    ..   re-enumeration %d ms, bootloader window %d ms\n",
                kReenumerationMs, kBootWindowMs);

    // 1. The port is there and auto-discovery has picked it up.
    manager.setPortPresent(true);
    manager.createAndStartTimer();
    pump(700);

    Link* link = manager.getLinkPtr(QUuid::createUuidV3(QUuid{}, QString::fromLatin1(kPort)));
    check(link != nullptr, "auto-discovery created a link for the port");
    if (!link) return 1;

    // A failed openAsSerial still emits connectionStatusChanged, so this counts attempts,
    // not successes -- which is what can be observed without real hardware on the far end.
    QElapsedTimer sinceUpgradeStart;
    QList<qint64> attemptsAt;
    QObject::connect(link, &Link::connectionStatusChanged, &app, [&](QUuid) {
        if (sinceUpgradeStart.isValid()) attemptsAt.append(sinceUpgradeStart.elapsed());
    });

    // 2. The user presses UPGRADE. This is what DevQProperty::startUpgradingFirmware does,
    //    and it is also the moment the bootloader's window starts.
    sinceUpgradeStart.start();
    link->onStartUpgradingFirmware();
    check(link->getIsUpgradingState(), "link enters the upgrading state");
    check(link->getAutoConnOnce(), "link is armed to reconnect");

    // 3. The MCU resets and the USB device detaches: the port is gone.
    manager.setPortPresent(false);
    pump(kReenumerationMs);

    check(manager.getLinkPtr(link->getUuid()) != nullptr,
          "link is not deleted while the port is away and an upgrade is in flight");
    const int attemptsWhileAbsent = attemptsAt.size();

    // 4. Re-enumeration finishes with time to spare inside the window.
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
