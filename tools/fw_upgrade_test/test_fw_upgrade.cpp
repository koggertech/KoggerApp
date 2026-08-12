// Firmware-upgrade regression test for the host side of the KP1 upgrade exchange.
//
// Drives the real DevDriver / IDBinUpdate state machine against a scriptable Recorder
// (fake_recorder.h) over an in-process wire that has the same shape as the production
// path: DevDriver::binFrameOut -> bytes -> device, device -> FrameParser ->
// DevDriver::protoComplete. No hardware, no link layer, no UI.
//
// Scenarios and what each one is for are in docs/KoggerApp-Docs/fw-upgrade-host.md.
//
//   test_fw_upgrade.exe <firmware.ufw> [scenario ...]

#include <cstdio>
#include <functional>

#include <QByteArray>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFile>
#include <QList>
#include <QString>
#include <QStringList>
#include <QTimer>

#include "core.h"                 // resolves to tools/fw_upgrade_test/shim/core.h
#include "device/dev_driver.h"
#include "fake_recorder.h"

Core core;

namespace {

using namespace FwTest;

constexpr uint8_t  kRecorderRoute = 2;   // a Recorder answers on its own bus address, not 0
constexpr int      kPacketPayload = 96;  // IDBinUpdate::_packetSize

// ---------------------------------------------------------------- reporting

// FWTEST_TRACE=<scenario> dumps that scenario's wire exchange. Worth leaving in: the
// scenarios are timing machines, and "which frame arrived when, carrying which mark bit"
// is the only question worth asking when one behaves unlike its isolated run.
bool gTrace = false;
QElapsedTimer gTraceClock;

void trace(const char* dir, const WireFrame& f)
{
    if (!gTrace) return;
    std::printf("    %6lld %s id=0x%02X type=%d ver=%d mark=%d resp=%d len=%lld\n",
                qlonglong(gTraceClock.elapsed()), dir, int(f.id), int(f.type), int(f.ver),
                f.mark ? 1 : 0, f.resp ? 1 : 0, qlonglong(f.payload.size()));
}

struct Report {
    QString scenario;
    int checks = 0;
    int failures = 0;
    QStringList notes;

    void check(bool ok, const QString& what, const QString& detail = QString())
    {
        ++checks;
        if (ok) {
            std::printf("    ok   %s\n", qUtf8Printable(what));
        } else {
            ++failures;
            std::printf("  FAIL   %s%s\n", qUtf8Printable(what),
                        detail.isEmpty() ? "" : qUtf8Printable(QString("  [%1]").arg(detail)));
        }
    }

    void note(const QString& text)
    {
        notes.append(text);
        std::printf("    ..   %s\n", qUtf8Printable(text));
    }
};

// Runs the event loop until `until` holds or the deadline passes. Never busy-waits: the
// device answers on zero-delay timers, so everything here depends on the loop turning.
bool spin(int msTimeout, const std::function<bool()>& until)
{
    if (until && until()) return true;

    QEventLoop loop;
    QTimer poll;
    QTimer deadline;
    bool satisfied = false;

    poll.setInterval(2);
    deadline.setSingleShot(true);

    QObject::connect(&poll, &QTimer::timeout, &loop, [&]() {
        if (!until || until()) { satisfied = true; loop.quit(); }
    });
    QObject::connect(&deadline, &QTimer::timeout, &loop, [&]() { loop.quit(); });

    poll.start();
    deadline.start(msTimeout);
    loop.exec();

    return satisfied || (until && until());
}

// ---------------------------------------------------------------- the rig

struct Observed {
    int startUpgrading = 0;
    int doneUpgrading  = 0;
    int doneUpgradingDM = 0;
    int lastProgress = -999;
    QList<int> progress;

    int  updateFrames = 0;          // ID_UPDATE SETTING frames the host put on the wire
    int  bootV0Frames = 0;          // reboot-into-bootloader
    int  bootV1Frames = 0;          // run-the-new-firmware
    bool sawSetupRequest = false;   // proves DevDriver reached m_state.connect == true

    // The mark bit gates the whole upgrade state machine, so how many device frames carry
    // it is the first thing worth knowing when a scenario behaves unexpectedly.
    int  deviceFrames = 0;
    int  deviceFramesMarked = 0;

    // ID_MARK SETTING is emitted from exactly one place: the process() branch taken when
    // m_state.mark is clear, which also calls restartState(). Counting it says how often
    // the driver decided the device had gone quiet.
    int  markSettings = 0;

    QList<quint16> packetNumbers;
    QByteArray fwAsSent;            // payload bytes in the order the host emitted them

    void reset() { *this = Observed(); }
};

class Rig
{
public:
    Rig(FakeRecorder::Policy policy)
        : device_(kRecorderRoute, policy)
    {
        dev_.setBusAddress(kRecorderRoute);

        QObject::connect(&dev_, &DevDriver::binFrameOut, &ctx_,
                         [this](Parsers::ProtoBinOut out) { onHostFrame(out); });
        QObject::connect(&dev_, &DevDriver::startUpgradingFirmware, &ctx_,
                         [this]() { ++obs_.startUpgrading; });
        QObject::connect(&dev_, &DevDriver::upgradingFirmwareDone, &ctx_,
                         [this]() { ++obs_.doneUpgrading; });
        QObject::connect(&dev_, &DevDriver::upgradingFirmwareDoneDM, &ctx_,
                         [this]() { ++obs_.doneUpgradingDM; });
        QObject::connect(&dev_, &DevDriver::upgradeProgressChanged, &ctx_,
                         [this](int p) { obs_.lastProgress = p; obs_.progress.append(p); });

        device_.setToHost([this](const QByteArray& bytes) { onDeviceFrame(bytes); });
    }

    DevDriver&    dev()    { return dev_; }
    FakeRecorder& device() { return device_; }
    Observed&     obs()    { return obs_; }

    // Brings the device up exactly the way DeviceManager does, then waits until the
    // driver is genuinely connected. That matters: DevDriver::reboot() -- the first thing
    // sendUpdateFW does -- silently returns when m_state.connect is false, so a test that
    // flashed too early would exercise a path the button never takes.
    bool bringUp(int msTimeout = 12000)
    {
        dev_.startConnection(true);
        if (!spin(msTimeout, [this]() { return obs_.sawSetupRequest; })) {
            return false;
        }

        // Then wait for the link to go quiet before handing back. The setup codecs re-ask
        // every 1500 ms, so there is almost always an answer in flight; if UPGRADE is
        // pressed while an ID_VERSION reply is on the wire, that reply lands after
        // reboot()'s idVersion->reset() and re-identifies the device from stale evidence,
        // and the driver walks into the packet phase without the bootloader ever having
        // spoken. Real, and worth knowing about -- but it is not what most of these
        // scenarios are measuring, so they start from an idle link instead of racing it.
        return spin(msTimeout, [this]() { return quietForMs() > 400; });
    }

    qint64 quietForMs() const
    {
        return sinceLastDeviceFrame_.isValid() ? sinceLastDeviceFrame_.elapsed() : 0;
    }

private:
    void onHostFrame(Parsers::ProtoBinOut out)
    {
        const QByteArray bytes(reinterpret_cast<const char*>(out.frame()), out.frameLen());

        WireFrame f;
        if (decode(bytes, &f)) {
            if (f.id == ID_UPDATE && f.type == SETTING && f.payload.size() >= 2) {
                ++obs_.updateFrames;
                obs_.packetNumbers.append(rd16(f.payload, 0));
                obs_.fwAsSent.append(f.payload.mid(2));
            }
            if (f.id == ID_BOOT && f.type == SETTING) {
                if (f.ver == v0) ++obs_.bootV0Frames;
                if (f.ver == v1) ++obs_.bootV1Frames;
            }
            if (f.id == ID_DATASET && f.type == GETTING) obs_.sawSetupRequest = true;
            if (f.id == ID_MARK && f.type == SETTING) ++obs_.markSettings;
            if (f.id != ID_UPDATE) trace("host->dev", f);
        }

        QTimer::singleShot(0, &ctx_, [this, bytes]() { device_.fromHost(bytes); });
    }

    // Mirrors Link::toParser: bytes in, complete frames out, straight into the driver.
    void onDeviceFrame(const QByteArray& bytes)
    {
        sinceLastDeviceFrame_.start();

        WireFrame df;
        if (decode(bytes, &df)) {
            ++obs_.deviceFrames;
            if (df.mark) ++obs_.deviceFramesMarked;
            if (df.id != ID_UPDATE) trace("dev->host", df);
        }

        inbox_ = bytes;
        parser_.setContext(reinterpret_cast<uint8_t*>(inbox_.data()), uint32_t(inbox_.size()));
        while (parser_.availContext() > 0) {
            parser_.process();
            if (parser_.isComplete()) {
                dev_.protoComplete(parser_);
            }
        }
    }

    QObject      ctx_;
    DevDriver    dev_;
    FakeRecorder device_;
    Parsers::FrameParser parser_;
    QByteArray   inbox_;
    QElapsedTimer sinceLastDeviceFrame_;
    Observed     obs_;
};

QString rigState(Rig& rig)
{
    return QString("reboots %1, runFW %2, inBootloader %3, windowExpired %4, "
                   "device frames %5 (%6 marked), host ID_UPDATE %7, status %8")
        .arg(rig.device().rebootCount())
        .arg(rig.device().runFwCount())
        .arg(rig.device().inBootloader() ? "y" : "n")
        .arg(rig.device().bootWindowExpired() ? "y" : "n")
        .arg(rig.obs().deviceFrames)
        .arg(rig.obs().deviceFramesMarked)
        .arg(rig.obs().updateFrames)
        .arg(rig.dev().upgradeFWStatus())
        + QString(", host ID_MARK %1").arg(rig.obs().markSettings);
}

bool packetNumbersAreContiguous(const QList<quint16>& numbers, QString* detail)
{
    for (int i = 0; i < numbers.size(); ++i) {
        const quint16 want = quint16(i + 1);
        if (numbers.at(i) != want) {
            *detail = QString("index %1: expected packet %2, got %3").arg(i).arg(want).arg(numbers.at(i));
            return false;
        }
    }
    return true;
}

// ---------------------------------------------------------------- scenarios

// The exchange as it is supposed to go. Also the only scenario that proves the wire
// format itself: every firmware byte has to arrive at the device unchanged and in order.
void scenarioHappyPath(const QByteArray& fw, Report& r)
{
    FakeRecorder::Policy policy;
    Rig rig(policy);

    r.check(rig.bringUp(), "device identifies and the driver reaches connected state");
    core.clear();

    rig.dev().sendUpdateFW(fw);

    const bool finished = spin(120000, [&]() { return rig.obs().doneUpgrading > 0; });

    const int expectedPackets = int((fw.size() + kPacketPayload - 1) / kPacketPayload);
    r.note(QString("packets sent %1, expected %2, device stored %3 of %4 bytes")
               .arg(rig.obs().updateFrames).arg(expectedPackets)
               .arg(rig.device().received().size()).arg(fw.size()));

    r.check(finished, "upgrade finishes");
    r.check(rig.obs().startUpgrading == 1, "startUpgradingFirmware emitted exactly once",
            QString("got %1").arg(rig.obs().startUpgrading));
    r.check(rig.obs().doneUpgrading == 1, "upgradingFirmwareDone emitted exactly once",
            QString("got %1").arg(rig.obs().doneUpgrading));
    r.check(rig.obs().doneUpgradingDM == 1, "upgradingFirmwareDoneDM emitted exactly once",
            QString("got %1").arg(rig.obs().doneUpgradingDM));
    r.check(rig.dev().upgradeFWStatus() == DevDriver::successUpgrade, "status is successUpgrade",
            QString("got %1").arg(rig.dev().upgradeFWStatus()));
    r.check(!rig.dev().isUpdatingFw(), "driver leaves the updating state");
    r.check(rig.device().rebootCount() == 1, "device was rebooted into the bootloader once",
            QString("got %1").arg(rig.device().rebootCount()));
    r.check(rig.device().runFwCount() == 1, "device was told to run the new firmware once",
            QString("got %1").arg(rig.device().runFwCount()));
    r.check(rig.obs().bootV1Frames == 1, "exactly one ID_BOOT v1 on the wire",
            QString("got %1").arg(rig.obs().bootV1Frames));
    r.check(rig.obs().updateFrames == expectedPackets, "one ID_UPDATE frame per 96-byte chunk",
            QString("got %1, expected %2").arg(rig.obs().updateFrames).arg(expectedPackets));

    QString detail;
    r.check(packetNumbersAreContiguous(rig.obs().packetNumbers, &detail),
            "packet numbers run 1..N with no gap or repeat", detail);
    r.check(rig.obs().fwAsSent == fw, "bytes put on the wire equal the firmware file",
            QString("sent %1 bytes").arg(rig.obs().fwAsSent.size()));
    r.check(rig.device().received() == fw, "bytes the device stored equal the firmware file",
            QString("stored %1 bytes").arg(rig.device().received().size()));
}

// The port comes back inside the bootloader's window. Nothing in the driver has to be
// clever about this -- it just has to keep asking until somebody answers -- so this
// scenario is what separates a driver defect from a link defect. If it passes and the real
// app still fails, the fault is in the link layer, not here.
void scenarioReenumerationRecovers(const QByteArray& fw, Report& r)
{
    FakeRecorder::Policy policy;
    policy.unreachableForMs = 1500;   // typical STM32 CDC re-enumeration on Windows
    policy.bootWindowMs     = 5000;
    Rig rig(policy);

    r.check(rig.bringUp(), "device identifies and the driver reaches connected state");
    core.clear();

    rig.dev().sendUpdateFW(fw);

    const bool finished = spin(120000, [&]() { return rig.obs().doneUpgrading > 0; });

    r.note(QString("packets sent %1, device stored %2 of %3 bytes")
               .arg(rig.obs().updateFrames).arg(rig.device().received().size()).arg(fw.size()));

    r.check(!rig.device().bootWindowExpired(),
            "the first packet landed inside the bootloader's 5 s window");
    r.check(finished, "upgrade finishes after the device re-enumerates");
    r.check(rig.device().received() == fw, "bytes the device stored equal the firmware file");
}

// The port comes back after the window has closed. The bootloader has already jumped into
// the firmware that is still flashed, so the thing that answers is an ordinary application
// with no ID_UPDATE. The host has to notice and stop -- otherwise it flashes into a void
// and leaves the link marked upgrading for the rest of the session.
void scenarioWindowMissed(const QByteArray& fw, Report& r)
{
    FakeRecorder::Policy policy;
    policy.unreachableForMs = 6000;
    policy.bootWindowMs     = 5000;
    Rig rig(policy);

    r.check(rig.bringUp(), "device identifies and the driver reaches connected state");
    core.clear();

    rig.dev().sendUpdateFW(fw);

    const bool gaveUp = spin(25000, [&]() { return rig.obs().doneUpgrading > 0; });

    r.note(QString("boot window expired: %1, device still in bootloader: %2, "
                   "ID_UPDATE frames sent into it: %3")
               .arg(rig.device().bootWindowExpired() ? "yes" : "no")
               .arg(rig.device().inBootloader() ? "yes" : "no")
               .arg(rig.obs().updateFrames));

    r.check(rig.device().bootWindowExpired(), "the bootloader window did lapse (scenario is set up right)");
    r.check(gaveUp, "host abandons the upgrade once the bootloader window has lapsed");
    r.check(rig.dev().upgradeFWStatus() == DevDriver::failUpgrade, "status is failUpgrade",
            QString("got %1").arg(rig.dev().upgradeFWStatus()));
    r.check(!rig.dev().isUpdatingFw(), "driver leaves the updating state");
}

// The port never comes back under the same handle. Same requirement as above and the
// simplest possible shape of it.
void scenarioBootloaderUnreachable(const QByteArray& fw, Report& r)
{
    FakeRecorder::Policy policy;
    policy.unreachableForMs = -1;
    Rig rig(policy);

    r.check(rig.bringUp(), "device identifies and the driver reaches connected state");
    core.clear();

    rig.dev().sendUpdateFW(fw);

    // Well past _timeoutUpgradeAnswerTime, which sendUpdateFW sets to 5000 ms.
    const bool gaveUp = spin(20000, [&]() { return rig.obs().doneUpgrading > 0; });

    r.note(rigState(rig) + QString(", console: %1").arg(core.lines().join(" | ")));

    r.check(gaveUp, "host abandons the upgrade when the bootloader never answers");
    r.check(rig.dev().upgradeFWStatus() == DevDriver::failUpgrade, "status is failUpgrade",
            QString("got %1").arg(rig.dev().upgradeFWStatus()));
    r.check(!rig.dev().isUpdatingFw(), "driver leaves the updating state");
    r.check(core.countContaining("aborted") > 0, "the give-up is reported to the console");
}

// The bootloader answers the mark but never identifies itself, so boardVersion() stays
// BoardNone and process() can never promote in_boot to in_update. Same requirement as
// above: the host owns the timeout, not the user.
void scenarioBootloaderNeverIdentifies(const QByteArray& fw, Report& r)
{
    FakeRecorder::Policy policy;
    policy.bootAnswersVersion = false;
    Rig rig(policy);

    r.check(rig.bringUp(), "device identifies and the driver reaches connected state");
    core.clear();

    rig.dev().sendUpdateFW(fw);

    const bool gaveUp = spin(20000, [&]() { return rig.obs().doneUpgrading > 0; });

    r.note(rigState(rig));

    r.check(gaveUp, "host abandons the upgrade when the bootloader never identifies");
    r.check(rig.dev().upgradeFWStatus() == DevDriver::failUpgrade, "status is failUpgrade",
            QString("got %1").arg(rig.dev().upgradeFWStatus()));
    r.check(!rig.dev().isUpdatingFw(), "driver leaves the updating state");
}

// The link dies partway through the transfer. The host is entitled to retry, but the
// retries have to be bounded and it has to fail in the end -- an unbounded resend keeps
// the link marked "upgrading" forever, which is what makes the port unrecoverable.
void scenarioStallsMidTransfer(const QByteArray& fw, Report& r)
{
    const int stopAfter = 100;

    FakeRecorder::Policy policy;
    policy.silentAfterPacket = stopAfter;
    Rig rig(policy);

    r.check(rig.bringUp(), "device identifies and the driver reaches connected state");
    core.clear();

    rig.dev().sendUpdateFW(fw);

    spin(3000, [&]() { return rig.device().updatePackets() >= stopAfter; });
    const int framesAtStall = rig.obs().updateFrames;

    const bool gaveUp = spin(20000, [&]() { return rig.obs().doneUpgrading > 0; });
    const int retransmits = rig.obs().updateFrames - framesAtStall;

    r.note(QString("packets before the stall: %1, retransmits after it: %2, timeout lines: %3")
               .arg(framesAtStall).arg(retransmits).arg(core.countContaining("timeout")));

    r.check(gaveUp, "host abandons the upgrade after the device stops answering");
    r.check(retransmits > 0, "host retries at least once before giving up",
            QString("got %1").arg(retransmits));
    r.check(retransmits <= 10, "retries are bounded",
            QString("got %1 resends of the same packet").arg(retransmits));
    r.check(rig.dev().upgradeFWStatus() == DevDriver::failUpgrade, "status is failUpgrade",
            QString("got %1").arg(rig.dev().upgradeFWStatus()));
}

// Characterisation, not a defect claim: this pins how hard the upgrade depends on the mark
// bit. DevDriver::protoComplete reassigns m_state.mark from every frame it receives, and a
// tick that sees it clear calls restartState(), which wipes in_update -- so a bootloader
// whose answers carry no mark can never be reached at all, and the 5 s window lapses while
// the host is still asking. Whether any bootloader behaves that way has not been checked;
// the firmware was not read. If this scenario ever starts failing, the coupling changed and
// somebody should decide whether that was on purpose.
void scenarioMarkDependency(const QByteArray& fw, Report& r)
{
    FakeRecorder::Policy policy;
    policy.bootSetsMark = false;
    policy.bootWindowMs = 60000;   // hold the device in the bootloader so only the mark
                                   // bit is under test, not the fallback timer
    Rig rig(policy);

    r.check(rig.bringUp(), "device identifies and the driver reaches connected state");
    core.clear();

    rig.dev().sendUpdateFW(fw);
    const bool gaveUp = spin(20000, [&]() { return rig.obs().doneUpgrading > 0; });

    r.note(rigState(rig));

    r.check(rig.obs().updateFrames == 0,
            "no packet is ever sent to a bootloader that does not echo the mark bit",
            QString("got %1").arg(rig.obs().updateFrames));
    r.check(gaveUp, "the handshake deadline still releases the link");
    r.check(rig.dev().upgradeFWStatus() == DevDriver::failUpgrade, "status is failUpgrade",
            QString("got %1").arg(rig.dev().upgradeFWStatus()));
}

// Old bootloaders answer an ID_UPDATE with a bare ACK instead of a progress report;
// m_bootloaderLagacyMode is the host's name for that mode. It has to still work.
void scenarioLegacyAckOnly(const QByteArray& fw, Report& r)
{
    FakeRecorder::Policy policy;
    policy.legacyAckOnly = true;
    Rig rig(policy);

    r.check(rig.bringUp(), "device identifies and the driver reaches connected state");
    core.clear();

    rig.dev().sendUpdateFW(fw);

    const bool finished = spin(120000, [&]() { return rig.obs().doneUpgrading > 0; });

    r.note(QString("packets sent %1, device stored %2 of %3 bytes")
               .arg(rig.obs().updateFrames).arg(rig.device().received().size()).arg(fw.size()));

    r.check(finished, "upgrade finishes in legacy ACK mode");
    r.check(rig.device().received() == fw, "bytes the device stored equal the firmware file");
}

struct Scenario {
    const char* name;
    void (*run)(const QByteArray&, Report&);
};

const Scenario kScenarios[] = {
    { "happy-path",              scenarioHappyPath },
    { "legacy-ack-only",         scenarioLegacyAckOnly },
    { "reenumeration-recovers",  scenarioReenumerationRecovers },
    { "window-missed",           scenarioWindowMissed },
    { "bootloader-unreachable",  scenarioBootloaderUnreachable },
    { "bootloader-mute",         scenarioBootloaderNeverIdentifies },
    { "stall-mid-transfer",      scenarioStallsMidTransfer },
    { "mark-dependency",         scenarioMarkDependency },
};

} // namespace

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

    const QStringList args = QCoreApplication::arguments();
    if (args.size() < 2) {
        std::printf("usage: %s <firmware.ufw> [scenario ...]\n", argv[0]);
        for (const Scenario& s : kScenarios) std::printf("  %s\n", s.name);
        return 2;
    }

    QFile file(args.at(1));
    if (!file.open(QIODevice::ReadOnly)) {
        std::printf("cannot open firmware file: %s\n", qUtf8Printable(args.at(1)));
        return 2;
    }
    const QByteArray fw = file.readAll();
    file.close();

    const QStringList wanted = args.mid(2);

    std::printf("firmware: %s (%lld bytes, %lld packets of %d)\n",
                qUtf8Printable(args.at(1)), qlonglong(fw.size()),
                qlonglong((fw.size() + kPacketPayload - 1) / kPacketPayload), kPacketPayload);

    int totalChecks = 0;
    int totalFailures = 0;
    QStringList failedScenarios;

    for (const Scenario& s : kScenarios) {
        if (!wanted.isEmpty() && !wanted.contains(QString::fromLatin1(s.name))) continue;

        std::printf("\n== %s ==\n", s.name);
        core.clear();
        gTrace = (qEnvironmentVariable("FWTEST_TRACE") == QString::fromLatin1(s.name));
        if (gTrace) gTraceClock.start();

        Report r;
        r.scenario = QString::fromLatin1(s.name);
        QElapsedTimer t;
        t.start();
        s.run(fw, r);
        std::printf("   (%lld ms, %d checks, %d failed)\n", qlonglong(t.elapsed()), r.checks, r.failures);

        totalChecks += r.checks;
        totalFailures += r.failures;
        if (r.failures) failedScenarios.append(r.scenario);
    }

    std::printf("\n---------------------------------------------\n");
    std::printf("%d checks, %d failed\n", totalChecks, totalFailures);
    if (!failedScenarios.isEmpty()) {
        std::printf("failing scenarios: %s\n", qUtf8Printable(failedScenarios.join(", ")));
    }

    return totalFailures == 0 ? 0 : 1;
}
