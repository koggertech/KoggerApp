
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

#include "core.h"
#include "device/dev_driver.h"
#include "fake_recorder.h"

Core core;

namespace {

using namespace FwTest;

constexpr uint8_t  kRecorderRoute = 2;
constexpr int      kPacketPayload = 96;

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

struct Observed {
    int startUpgrading = 0;
    int doneUpgrading  = 0;
    int doneUpgradingDM = 0;
    int lastProgress = -999;
    QList<int> progress;

    int  updateFrames = 0;
    int  bootV0Frames = 0;
    int  bootV1Frames = 0;
    bool sawSetupRequest = false;

    int  deviceFrames = 0;
    int  deviceFramesMarked = 0;

    int  markSettings = 0;

    QList<quint16> packetNumbers;
    QByteArray fwAsSent;

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

    bool bringUp(int msTimeout = 12000)
    {
        dev_.startConnection(true);
        if (!spin(msTimeout, [this]() { return obs_.sawSetupRequest; })) {
            return false;
        }

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

void scenarioReenumerationRecovers(const QByteArray& fw, Report& r)
{
    FakeRecorder::Policy policy;
    policy.unreachableForMs = 1500;
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

void scenarioBootloaderUnreachable(const QByteArray& fw, Report& r)
{
    FakeRecorder::Policy policy;
    policy.unreachableForMs = -1;
    Rig rig(policy);

    r.check(rig.bringUp(), "device identifies and the driver reaches connected state");
    core.clear();

    rig.dev().sendUpdateFW(fw);

    const bool gaveUp = spin(20000, [&]() { return rig.obs().doneUpgrading > 0; });

    r.note(rigState(rig) + QString(", console: %1").arg(core.lines().join(" | ")));

    r.check(gaveUp, "host abandons the upgrade when the bootloader never answers");
    r.check(rig.dev().upgradeFWStatus() == DevDriver::failUpgrade, "status is failUpgrade",
            QString("got %1").arg(rig.dev().upgradeFWStatus()));
    r.check(!rig.dev().isUpdatingFw(), "driver leaves the updating state");
    r.check(core.countContaining("aborted") > 0, "the give-up is reported to the console");
}

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

void scenarioMarkDependency(const QByteArray& fw, Report& r)
{
    FakeRecorder::Policy policy;
    policy.bootSetsMark = false;
    policy.bootWindowMs = 60000;
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

}

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
