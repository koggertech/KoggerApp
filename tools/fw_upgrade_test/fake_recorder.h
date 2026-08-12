#pragma once

// A Recorder that behaves the way the real STM32 USB-VCP part does during a flash:
//
//   ID_BOOT v0 -> the MCU resets, so the USB device detaches and COM11 disappears from
//                 the host for the length of a re-enumeration;
//              -> the bootloader then waits BOOT WINDOW milliseconds for the first
//                 ID_UPDATE and, hearing nothing, jumps into the existing firmware.
//
// That second part is the whole problem: the window is a deadline, and once it lapses the
// device is back to being an ordinary application that has never heard of ID_UPDATE. The
// faults below are that sequence with the two free variables -- how long the port is gone
// and how long the bootloader waits -- turned into knobs.

#include <functional>

#include <QByteArray>
#include <QElapsedTimer>
#include <QObject>
#include <QTimer>
#include <QVector>

#include "id_binnary.h"
#include "kp1_codec.h"

namespace FwTest {

// Plain QObject, no Q_OBJECT: it only needs to be a timer context, and staying out of
// moc keeps the test's build a single clang invocation over the app's own moc output.
class FakeRecorder : public QObject
{
public:
    enum Persona { PersonaApp, PersonaBoot };

    struct Policy {
        // Never zero. A device that answers on a zero-delay timer produces an unbroken
        // stream of posted events that starves DevDriver's 200 ms process() tick, and then
        // scenarios pass or fail depending on machine load rather than on the code. A real
        // link has gaps; so does this one.
        int  latencyMs          = 1;
        // The bootloader runs the existing firmware if no ID_UPDATE arrives within this
        // long. Measured on the bench at 5 s; it is a deadline, not a courtesy.
        int  bootWindowMs       = 5000;
        // How long the host cannot reach the device after the reboot command, i.e. the USB
        // re-enumeration. -1 means it never comes back.
        int  unreachableForMs   = 0;
        bool bootAnswersVersion = true;  // false = bootloader marks but never identifies
        bool bootSetsMark       = true;  // false = bootloader answers without the mark bit
        bool legacyAckOnly      = false; // answer ID_UPDATE with a bare ACK, no v0 progress
        int  silentAfterPacket  = -1;    // >=0: accept this many packets, then go quiet
    };

    explicit FakeRecorder(uint8_t route, Policy policy, QObject* parent = nullptr)
        : QObject(parent), route_(route), policy_(policy)
    {
        bootWindow_.setSingleShot(true);
        QObject::connect(&bootWindow_, &QTimer::timeout, this, [this]() {
            bootWindowExpired_ = true;
            enterApp();                  // jumps into the firmware that is already flashed
        });
    }

    void setToHost(std::function<void(const QByteArray&)> sink) { toHost_ = std::move(sink); }

    int  rebootCount()       const { return rebootCount_; }
    int  runFwCount()        const { return runFwCount_; }
    int  updatePackets()     const { return updatePackets_; }
    bool bootWindowExpired() const { return bootWindowExpired_; }
    bool inBootloader()      const { return persona_ == PersonaBoot; }
    const QByteArray& received() const { return received_; }

    // Bytes the host put on the wire. One complete KP1 frame per call, which is what
    // Link::writeFrame does.
    void fromHost(const QByteArray& bytes)
    {
        WireFrame f;
        if (!decode(bytes, &f)) return;
        if (f.route != route_) return;
        if (unreachable()) return;

        switch (f.id) {
        case ID_MARK:
            if (f.type == SETTING) { markOn_ = (persona_ == PersonaApp) || policy_.bootSetsMark; ack(f); }
            break;

        case ID_VERSION:
            if (f.type == GETTING) answerVersion(f.ver);
            break;

        case ID_BOOT:
            ack(f);
            if (f.type == SETTING && f.ver == v0) { ++rebootCount_; enterBoot(); }
            if (f.type == SETTING && f.ver == v1) { ++runFwCount_;  enterApp();  }
            break;

        case ID_UPDATE:
            if (f.type == SETTING && persona_ == PersonaBoot) onUpdate(f);
            break;

        default:
            // Setup IDs, recorder status, stream list: a bootloader has none of them and
            // the application under test must not depend on an answer.
            break;
        }
    }

private:
    // The port is gone for the whole re-enumeration. Nothing the host writes lands, and
    // nothing it hears back exists -- which is exactly what a closed Link looks like from
    // the driver's side, since Link::write returns false with no ioDevice_.
    bool unreachable() const
    {
        if (!sinceReboot_.isValid()) return false;
        if (policy_.unreachableForMs < 0) return true;
        return sinceReboot_.elapsed() < policy_.unreachableForMs;
    }

    // Always deferred through the event loop, never a direct call back into the host.
    // A synchronous answer would turn the 974-packet transfer into 974 nested stack
    // frames and would hide any ordering bug that a real link exposes.
    void send(const QByteArray& frame)
    {
        QTimer::singleShot(policy_.latencyMs > 0 ? policy_.latencyMs : 0, Qt::PreciseTimer, this,
                           [this, frame]() { if (toHost_) toHost_(frame); });
    }

    void ack(const WireFrame& f)
    {
        QByteArray p;
        put8(p, uint8_t(respOk));
        put16(p, f.checksum);
        send(encode(CONTENT, f.ver, f.id, route_, markOn_, true, p));
    }

    void answerVersion(Version ver)
    {
        if (persona_ == PersonaBoot && !policy_.bootAnswersVersion) return;

        QByteArray p;
        if (ver == v0) {
            put8(p, 0);                                  // board version minor
            put8(p, uint8_t(BoardRecorderMini));         // board version
            put16(p, 0); put16(p, 0); put16(p, 0);
            put32(p, 0);
            put16(p, 0);
            put32(p, 4242);                              // serial number
        } else if (ver == v1) {
            for (int i = 0; i < 12; ++i) put8(p, uint8_t(i));
        } else if (ver == v2) {
            put8(p, persona_ == PersonaBoot ? 1 : 0);    // boot mode
            put8(p, 0);
            put8(p, uint8_t(BoardRecorderMini));
            put8(p, 3); put8(p, 1);                      // bootloader version
            put16(p, 0);
            put8(p, 7); put8(p, 2);                      // fw version minor / major
        } else {
            return;
        }
        send(encode(CONTENT, ver, ID_VERSION, route_, markOn_, false, p));
    }

    void onUpdate(const WireFrame& f)
    {
        if (f.payload.size() < 2) return;

        const uint16_t numPacket = rd16(f.payload, 0);
        const QByteArray chunk = f.payload.mid(2);

        ++updatePackets_;
        bootWindow_.start(policy_.bootWindowMs);   // the deadline is per packet, not per flash

        // The device writes at its own cursor; the host's packet number is what lets it
        // notice a gap. A retransmit of a packet number already stored is a no-op.
        if (numPacket == uint16_t(lastNumMsg_ + 1)) {
            received_.append(chunk);
            offset_ += uint32_t(chunk.size());
            lastNumMsg_ = numPacket;
        }

        if (policy_.silentAfterPacket >= 0 && updatePackets_ >= policy_.silentAfterPacket) return;

        if (policy_.legacyAckOnly) { ack(f); return; }

        QByteArray p;
        put16(p, lastNumMsg_);
        put32(p, offset_);
        put8(p, 0);                       // type 0 = accepted, no reposition needed
        put8(p, uint8_t(numPacket & 0xFF));
        send(encode(CONTENT, v0, ID_UPDATE, route_, policy_.bootSetsMark, false, p));
    }

    void enterBoot()
    {
        persona_    = PersonaBoot;
        markOn_     = false;             // a reboot clears it; the host has to re-mark
        offset_     = 0;
        lastNumMsg_ = 0;
        received_.clear();
        sinceReboot_.start();
        bootWindow_.start(policy_.bootWindowMs);
    }

    void enterApp()
    {
        persona_ = PersonaApp;
        markOn_  = false;
        bootWindow_.stop();
    }

    uint8_t  route_;
    Policy   policy_;
    Persona  persona_       = PersonaApp;
    bool     markOn_        = false;
    bool     bootWindowExpired_ = false;
    int      rebootCount_   = 0;
    int      runFwCount_    = 0;
    int      updatePackets_ = 0;
    uint16_t lastNumMsg_    = 0;
    uint32_t offset_        = 0;
    QByteArray received_;
    QElapsedTimer sinceReboot_;
    QTimer   bootWindow_;
    std::function<void(const QByteArray&)> toHost_;
};

} // namespace FwTest
