#pragma once

#include <functional>

#include <QByteArray>
#include <QElapsedTimer>
#include <QObject>
#include <QTimer>
#include <QVector>

#include "id_binnary.h"
#include "kp1_codec.h"

namespace FwTest {

class FakeRecorder : public QObject
{
public:
    enum Persona { PersonaApp, PersonaBoot };

    struct Policy {
        int  latencyMs          = 1;
        int  bootWindowMs       = 5000;
        int  unreachableForMs   = 0;
        bool bootAnswersVersion = true;
        bool bootSetsMark       = true;
        bool legacyAckOnly      = false;
        int  silentAfterPacket  = -1;
    };

    explicit FakeRecorder(uint8_t route, Policy policy, QObject* parent = nullptr)
        : QObject(parent), route_(route), policy_(policy)
    {
        bootWindow_.setSingleShot(true);
        QObject::connect(&bootWindow_, &QTimer::timeout, this, [this]() {
            bootWindowExpired_ = true;
            enterApp();
        });
    }

    void setToHost(std::function<void(const QByteArray&)> sink) { toHost_ = std::move(sink); }

    int  rebootCount()       const { return rebootCount_; }
    int  runFwCount()        const { return runFwCount_; }
    int  updatePackets()     const { return updatePackets_; }
    bool bootWindowExpired() const { return bootWindowExpired_; }
    bool inBootloader()      const { return persona_ == PersonaBoot; }
    const QByteArray& received() const { return received_; }

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
            break;
        }
    }

private:
    bool unreachable() const
    {
        if (!sinceReboot_.isValid()) return false;
        if (policy_.unreachableForMs < 0) return true;
        return sinceReboot_.elapsed() < policy_.unreachableForMs;
    }

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
            put8(p, 0);
            put8(p, uint8_t(BoardRecorderMini));
            put16(p, 0); put16(p, 0); put16(p, 0);
            put32(p, 0);
            put16(p, 0);
            put32(p, 4242);
        } else if (ver == v1) {
            for (int i = 0; i < 12; ++i) put8(p, uint8_t(i));
        } else if (ver == v2) {
            put8(p, persona_ == PersonaBoot ? 1 : 0);
            put8(p, 0);
            put8(p, uint8_t(BoardRecorderMini));
            put8(p, 3); put8(p, 1);
            put16(p, 0);
            put8(p, 7); put8(p, 2);
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
        bootWindow_.start(policy_.bootWindowMs);

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
        put8(p, 0);
        put8(p, uint8_t(numPacket & 0xFF));
        send(encode(CONTENT, v0, ID_UPDATE, route_, policy_.bootSetsMark, false, p));
    }

    void enterBoot()
    {
        persona_    = PersonaBoot;
        markOn_     = false;
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
