#ifndef IDBINNARY_H
#define IDBINNARY_H

#include <QObject>
#include <QVector>
#include <QTimer>
#include <ProtoBinnary.h>

using namespace Parsers;

typedef enum {
    BoardNone,
    BoardEnhanced = 1,
    BoardBase = 3,
    BoardNBase = 4,
    BoardChirp = 5,
    BoardAssist = 6,
    BoardNEnhanced = 7,
    BoardSideEnhanced = 8,
    BoardRecorderMini = 9,
    BoardDVL = 10,
    BoardEcho20 = 12,
    BoardUSBL = 15,
    BoardUSBLBeacon = 16,
    BoardNanoSSS = 17,

} BoardVersion;

struct LastReadInfo {
    LastReadInfo() : version(), checkSum(0), address(0), isReaded(true) {};
    LastReadInfo(Version _version, uint16_t _checkSum, uint8_t _address, bool _isReaded) :
        version(_version), checkSum(_checkSum), address(_address), isReaded(_isReaded) {};
    Version version;
    uint16_t checkSum;
    uint8_t address;
    bool isReaded;
};

class IDBin : public QObject
{
    Q_OBJECT
public:
    explicit IDBin(QObject *parent = nullptr);
    ~IDBin();

    Resp  parse(FrameParser &proto);

    virtual ID id() = 0;
    virtual bool isSettable() { return false; }
    virtual bool isSettup() { return false; }
    virtual bool isRequestable() { return true; }

    Type lastType() { return m_lastType; }
    Version lastVersion() { return m_lastVersion; }
    Resp lastResp() { return m_lastResp; }

    virtual void simpleRequest(Version ver);
    virtual void requestAll() { simpleRequest(v0); }
    virtual void startColdStartTimer() {};

    void setAddress(uint8_t addr) { m_address = addr; }
    void setConsoleOut(bool is_console) { isConsoleOut = is_console; }

#ifdef SEPARATE_READING
    void initTimersConnects();

    QTimer* getSetTimer() {
        return &setTimer_;
    }

    QTimer* getColdStartTimer() {
        return &coldStartTimer_;
    }
#endif

signals:
    void updateContent(Type type, Version ver, Resp resp, uint8_t address);
    void dataSend(QByteArray data);
    void binFrameOut(ProtoBinOut &proto_out);
    void notifyDevDriver(bool state);

protected:
    const U4 m_key = 0xC96B5D4A;

    Type m_lastType;
    Version m_lastVersion;
    Resp m_lastResp;
    uint8_t _lastAddress = 0;
    QList<Version> availableVer;
    uint8_t m_address = 0;
    bool isConsoleOut = false;

    virtual Resp  parsePayload(FrameParser &proto) = 0;
    virtual void requestSpecific(ProtoBinOut &proto_out) { Q_UNUSED(proto_out) }

    bool checkKeyConfirm(U4 key) { return (key == m_key); }
    void appendKey(ProtoBinOut &proto_out);

    void hashBinFrameOut(ProtoBinOut &proto);
    void interExecColdStartTimer();

private:
    /*methods*/
    bool checkResponse(FrameParser& proto);
    void onExpiredColdStartTimer();
    void onExpiredSetTimer();
    /*data*/
    static const uint8_t repeatingCount_ = 7;
    static const int timerPeriodMsec_ = 1500;
    QTimer setTimer_;
    QTimer coldStartTimer_;
    LastReadInfo hashLastInfo_;
    uint8_t setTimerCount_;
    uint8_t coldStartTimerCount_;
    bool isColdStart_;
    bool needToCheckSetResp_;
};



class IDBinTimestamp : public IDBin
{
    Q_OBJECT
public:
    explicit IDBinTimestamp() : IDBin() {
    }

    ID id() override { return ID_TIMESTAMP; }
    Resp  parsePayload(FrameParser &proto) override;

    uint32_t timestamp() { return m_timestamp; }
protected:
    uint32_t m_timestamp;
};



class IDBinDist : public IDBin
{
    Q_OBJECT
public:
    explicit IDBinDist() : IDBin() {
    }

    ID id() override { return ID_DIST; }
    Resp  parsePayload(FrameParser &proto) override;

    uint32_t dist_mm() { return m_dist_mm; }
protected:
    uint32_t m_dist_mm;
};



struct RawData {
    struct RawDataHeader {
        struct  __attribute__((packed)) {
            uint16_t dataType : 5; //
            uint16_t dataSize : 6; // +1 bytes
            uint16_t dataTrigger : 2;
            uint16_t channelGroup : 3;
        };
        uint8_t channelCount = 0;
        uint32_t globalOffset = 0;
        uint32_t localOffset = 0;
        float sampleRate = 0;
    } __attribute__((packed));

    RawDataHeader header;
    QByteArray data;
};


class IDBinChart : public IDBin
{
    Q_OBJECT
public:

    explicit IDBinChart() : IDBin() {
    }

    ID id() override { return ID_CHART; }
    Resp  parsePayload(FrameParser &proto) override;

    uint16_t sampleResol() const { return m_sampleResol; }
    uint16_t absOffset() const { return m_absOffset; }
    int chartSize() const { return m_chartSize; }

    bool isCompleteChart() {
        bool flag_val = m_isCompleteChart;
        m_isCompleteChart = false;
        return flag_val;
    }

    U2 resolution() {return m_sampleResol; }
    U2 offsetRange() {return m_absOffset*m_sampleResol; }

    uint8_t* logData8() { return m_completeChart; }
    uint8_t* logData28() { return m_completeChart2; }
    // uint8_t* rawData() { return _rawDataSave; }
    // uint32_t rawDataSize() { return _rawDataSize; }
    // uint8_t rawType() { return type; }

protected:
    uint32_t m_seqOffset = 0, m_sampleResol = 0, m_absOffset = 0;
    uint32_t m_chartSizeIncr = 0;
    uint32_t m_chartSize = 0;
    uint8_t m_fillChart[20000];
    uint8_t m_completeChart[20000];
    uint8_t m_completeChart2[20000];

    bool m_isCompleteChart = false;

    RawData _rawData;
signals:
    void rawDataRecieved(RawData raw_data);
};



class IDBinAttitude : public IDBin
{
    Q_OBJECT
public:
    explicit IDBinAttitude() : IDBin() {
    }

    ID id() override { return ID_ATTITUDE; }
    Resp  parsePayload(FrameParser &proto) override;

    float yaw(Version src_ver = v0);
    float pitch(Version src_ver = v0);
    float roll(Version src_ver = v0);
    float w0(Version src_ver = v1);
    float w1(Version src_ver = v1);
    float w2(Version src_ver = v1);
    float w3(Version src_ver = v1);
protected:
    float m_yaw, m_pitch, m_roll;
    float m_w0, m_w1, m_w2, m_w3;
};



class IDBinTemp : public IDBin
{
    Q_OBJECT
public:
    explicit IDBinTemp() : IDBin() {
    }

    ID id() override { return ID_TEMP; }
    Resp  parsePayload(FrameParser &proto) override;

    float temp() { return m_temp; }
protected:
    float m_temp;
};



class IDBinDataset : public IDBin
{
    Q_OBJECT
public:
    explicit IDBinDataset() : IDBin() {
    }

    ID id() override { return ID_DATASET; }
    Resp  parsePayload(FrameParser &proto) override;
    void startColdStartTimer() override;
    typedef struct {
        uint8_t id = 0;
        uint32_t period = 0;
        uint32_t mask = 0;
    } Channel;

    typedef enum {
        MASK_DIST_V0 = 1,
        MASK_CHART_V0 = 2,
        MASK_ATTITUDE_V0 = 4,
        MASK_ATTITUDE_V1 = 8,
        MASK_TEMP_V0 = 16,
        MASK_TIMESTAMP_V0 = 32,
        MASK_DIST_SDDBT = 64,
        MASK_DIST_SDDBT_P2 = 128,
    } ChannelMask;

    void setChannel(uint8_t ch_id, uint32_t period, uint32_t mask);
    uint32_t mask(U1 ch_id);
    void setMask(U1 ch_id, uint32_t mask);


    void setFlag(U1 ch_id, uint32_t flag) {
        if(ch_id != 0) {
            setMask(ch_id, mask(ch_id) | ((U4)flag));

            for(int i = 1; i < 3; i++) {
                if(ch_id != i) {
                    resetFlag(i, flag);
                }
            }
        } else {
            for(int i = 1; i < 3; i++) {
                resetFlag(i, flag);
            }
        }
    }

    void resetFlag(U1 ch_id, uint32_t flag) { setMask(ch_id, mask(ch_id) & (~((U4)flag))); }
    void moveFlag(U1 ch_id, uint32_t flag) {
        if(ch_id != 0) {
            uint32_t msk = mask(0)&flag;
            setFlag(ch_id, msk);
        }
    }
    bool flag(U1 ch_id, uint32_t flag) { return (mask(ch_id) & ((U4)flag)) != 0 ;}

    void setDist_v0(U1 ch_id) {
        setFlag(ch_id, MASK_DIST_V0);
        moveFlag(ch_id,    MASK_DIST_V0 | MASK_CHART_V0 | MASK_DIST_SDDBT | MASK_DIST_SDDBT_P2);
        setFlag(0, MASK_DIST_SDDBT | MASK_DIST_SDDBT_P2);
    }
    void resetDist_v0(U1 ch_id) { resetFlag(ch_id, MASK_DIST_V0); }
    bool getDist_v0(U1 ch_id) { return flag(ch_id, MASK_DIST_V0); }

    void setChart_v0(U1 ch_id) {
        setFlag(ch_id, MASK_CHART_V0);
        moveFlag(ch_id,  MASK_DIST_V0 | MASK_CHART_V0 | MASK_DIST_SDDBT | MASK_DIST_SDDBT_P2);
    }
    void resetChart_v0(U1 ch_id) { resetFlag(ch_id, MASK_CHART_V0); }
    bool getChart_v0(U1 ch_id) { return flag(ch_id, MASK_CHART_V0); }

    void setAttitude_v0(U1 ch_id) { setFlag(ch_id, MASK_ATTITUDE_V0); }
    void resetAttitude_v0(U1 ch_id) { resetFlag(ch_id, MASK_ATTITUDE_V0); }
    bool getAttitude_v0(U1 ch_id) { return flag(ch_id, MASK_ATTITUDE_V0); }

    void setAttitude_v1(U1 ch_id) { setFlag(ch_id, MASK_ATTITUDE_V1); }
    void resetAttitude_v1(U1 ch_id) { resetFlag(ch_id, MASK_ATTITUDE_V1); }
    bool getAttitude_v1(U1 ch_id) { return flag(ch_id, MASK_ATTITUDE_V1); }

    void setTemp_v0(U1 ch_id) {
        setFlag(ch_id, MASK_TEMP_V0);
        moveFlag(ch_id,  MASK_TEMP_V0);
    }
    void resetTemp_v0(U1 ch_id) { resetFlag(ch_id, MASK_TEMP_V0); }
    bool getTemp_v0(U1 ch_id) { return flag(ch_id, MASK_TEMP_V0); }

    void setEuler(U1 ch_id) { setFlag(ch_id, MASK_ATTITUDE_V0); }
    void resetEuler(U1 ch_id) { resetFlag(ch_id, MASK_ATTITUDE_V0); }
    bool getEuler(U1 ch_id) { return flag(ch_id, MASK_ATTITUDE_V0); }

    void setTimestamp_v0(U1 ch_id) { setFlag(ch_id, MASK_TIMESTAMP_V0); }
    void resetTimestamp_v0(U1 ch_id) { resetFlag(ch_id, MASK_TIMESTAMP_V0); }
    bool getTimestamp_v0(U1 ch_id) { return flag(ch_id, MASK_TIMESTAMP_V0); }

    void setSDDBT(U1 ch_id) {
        setFlag(ch_id, MASK_DIST_SDDBT);
        moveFlag(ch_id,  MASK_DIST_V0 | MASK_CHART_V0 | MASK_DIST_SDDBT | MASK_DIST_SDDBT_P2);
        setFlag(0, MASK_DIST_V0 | MASK_DIST_SDDBT_P2);
    }
    void resetSDDBT(U1 ch_id) { resetFlag(ch_id, MASK_DIST_SDDBT); }
    bool getSDDBT(U1 ch_id) { return flag(ch_id, MASK_DIST_SDDBT); }

    void setSDDBT_P2(U1 ch_id) {
        setFlag(ch_id, MASK_DIST_SDDBT_P2);
        moveFlag(ch_id,  MASK_DIST_V0 | MASK_CHART_V0 | MASK_DIST_SDDBT | MASK_DIST_SDDBT_P2);
        setFlag(0, MASK_DIST_V0 | MASK_DIST_SDDBT);

    }
    void resetSDDBT_P2(U1 ch_id) { resetFlag(ch_id, MASK_DIST_SDDBT_P2); }
    bool getSDDBT_P2(U1 ch_id) { return flag(ch_id, MASK_DIST_SDDBT_P2); }

    void resetAll() {
        setMask(1, 0);
        setMask(2, 0);
    }

    void commit() {
        sendChannel(1, period(1), mask(1));
        sendChannel(2, period(2), mask(2));
    }

    uint32_t period(U1 ch_id);
    void setPeriod(U1 ch_id, uint32_t period);

    QVector<Channel> getChannels() const;
    Channel getChannel(U1 channelId) const;

protected:
    Channel m_channel[3];

    void sendChannel(U1 ch_id, uint32_t period, uint32_t mask);
    virtual void requestSpecific(ProtoBinOut &proto_out) override { proto_out.write<U1>(0); }
};



class IDBinDistSetup : public IDBin
{
    Q_OBJECT
public:
    explicit IDBinDistSetup() : IDBin() {
    }

    ID id() override { return ID_DIST_SETUP; }
    Resp  parsePayload(FrameParser &proto) override;
    void startColdStartTimer() override;

    void setRange(uint32_t start_offset, uint32_t max_dist);

    int max() { return m_maxDist; }
    void setMax(uint32_t max_dist) { setRange(deadZone(), max_dist); }

    int deadZone() { return m_startOffset; }
    void setDeadZone(uint32_t dead_zone) { setRange(dead_zone, max()); }

    void setConfidence(int confidence);
    int confidence() { return m_confidence; }

    void requestAll() override {
        simpleRequest(v1);
        simpleRequest(v2);
    }
protected:
    uint32_t m_startOffset = 200;
    uint32_t m_maxDist = 50000;
    U1 m_confidence;

};



class IDBinChartSetup : public IDBin
{
    Q_OBJECT
public:
    explicit IDBinChartSetup() : IDBin() {
    }

    ID id() override { return ID_CHART_SETUP; }
    Resp  parsePayload(FrameParser &proto) override;
    void startColdStartTimer() override;

    void setV0(U2 count, U2 resolution, U2 offset);

    U2 count() { return m_sanpleCount; }
    void setCount(U2 count) {
        setV0(count, resolution(), offset());
    }

    U2 resolution() { return m_sanpleResolution; }
    void setResolution(U2 resol) {
        setV0(count(), resol, offset());
    }

    U2 offset() { return m_sanpleOffset; }
    void setOffset(U2 offset) {
        setV0(count(), resolution(), offset);
    }

protected:
    U2 m_sanpleCount = 100;
    U2 m_sanpleResolution = 10;
    U2 m_sanpleOffset = 0;
};


class IDBinDSPSetup : public IDBin
{
    Q_OBJECT
public:
    explicit IDBinDSPSetup() : IDBin() {
    }

    ID id() override { return ID_DSP; }
    Resp  parsePayload(FrameParser &proto) override;
    void startColdStartTimer() override;

    void setV0(U1 hor_smooth_factor);

    U1 horSmoothFactor() { return m_horSmoothFactor; }
    void setHorSmoothFactor(U1 hor_smooth_factor) {
        setV0(hor_smooth_factor);
    }

protected:
    U1 m_horSmoothFactor = 0;
};


class IDBinTransc : public IDBin
{
    Q_OBJECT
public:
    explicit IDBinTransc() : IDBin() {
    }

    ID id() override { return ID_TRANSC; }
    Resp  parsePayload(FrameParser &proto) override;
    void startColdStartTimer() override;

    void setTransc(U2 freq, U1 pulse, U1 boost);

    U2 freq() { return m_freq; }
    void setFreq(U2 freq) { setTransc(freq, pulse(), boost()); }

    U1 pulse() { return m_pulse; }
    void setPulse(U1 pulse) { setTransc(freq(), pulse, boost()); }

    U1 boost() { return m_boost; }
    void setBoost(U1 boost) { setTransc(freq(), pulse(), boost); }

protected:
    U2 m_freq = 700;
    U1 m_pulse = 10;
    U1 m_boost = 0;
};



class IDBinSoundSpeed : public IDBin
{
    Q_OBJECT
public:
    explicit IDBinSoundSpeed() : IDBin() {
    }

    ID id() override { return ID_SND_SPEED; }
    Resp  parsePayload(FrameParser &proto) override;
    void startColdStartTimer() override;

    void setSoundSpeed(U4 snd_spd);
    int getSoundSpeed() { return m_soundSpeed; }
protected:
    U4 m_soundSpeed = 1500000;
};



class IDBinUART : public IDBin
{
    Q_OBJECT
public:
    explicit IDBinUART() : IDBin() {
    }

    ID id() override { return ID_UART; }
    Resp  parsePayload(FrameParser &proto) override;
    void startColdStartTimer() override;

    typedef struct {
        U1 id = 0;
        U4 baudrate = 115200;
        U1 dev_address = 0;
    } UART;

    void setBaudrate(U4 baudrate);
    int getBaudrate(int id = 1) { return m_uart[id].baudrate; }

    void setDevAddress(U1 addr);
    U1 devAddress() { return m_uart[1].dev_address; }

    void setDevDefAddress(U1 addr);
    U1 devDefAddress() { return devDef_address; }

    QVector<UART> getUart() const;

    void requestAll() override {
        simpleRequest(v0);
        simpleRequest(v1);
        simpleRequest(v2);
    }
protected:
    UART m_uart[5];
    U1 devDef_address = 0;

    virtual void requestSpecific(ProtoBinOut &proto_out) override {
        appendKey(proto_out);
        if(proto_out.ver() == 0 || proto_out.ver() == 1) {
            proto_out.write<U1>(1);
        }
    }
};

class IDBinVersion : public IDBin
{
    Q_OBJECT
public:
    explicit IDBinVersion() : IDBin() {
    }

    ID id() override { return ID_VERSION; }
    Resp  parsePayload(FrameParser &proto) override;
    uint8_t productName() { return 0; }

    BoardVersion boardVersion() { return m_boardVersion; }
    uint8_t boardVersionMinor() { return m_boardVersionMinor; }

    int fwVersion() { return _fwVersion; }
    int fwVersionMinor() { return _fwVersionMinor; }

    uint32_t serialNumber() { return m_serialNumber; }

    void reset() {
        m_boardVersion = BoardNone;
        m_boardVersionMinor = 0;
        m_serialNumber = 0;
    }

    QByteArray uid() { return _uid; }

    void requestAll() override {
        simpleRequest(v0);
        simpleRequest(v1);
        simpleRequest(v2);
    }
protected:
    BoardVersion m_boardVersion = BoardNone;
    uint8_t m_boardVersionMinor = 0;
    uint32_t m_serialNumber = 0;
    QString m_pn;
    QByteArray _uid;
    int _fwVersion = 0;
    int _fwVersionMinor = 0;
    int _bootVersion = 0;
    int _bootVersionMinor = 0;
    int _bootMode = 0;
};

class IDBinMark : public IDBin
{
    Q_OBJECT
public:
    explicit IDBinMark() : IDBin() {
    }

    ID id() override { return ID_MARK; }
    Resp  parsePayload(FrameParser &proto) override;

    bool mark() { return (m_mark & 0x1) == 0x1; }
    void setMark();

protected:
    U1 m_mark = 0;
};

class IDBinFlash : public IDBin
{
    Q_OBJECT
public:
    explicit IDBinFlash() : IDBin() {
    }

    ID id() override { return ID_FLASH; }
    Resp  parsePayload(FrameParser &proto) override;

    void flashing();
    void restore();
    void erase();

protected:
};



class IDBinBoot : public IDBin
{
    Q_OBJECT
public:
    explicit IDBinBoot() : IDBin() {
    }

    ID id() override { return ID_BOOT; }
    Resp  parsePayload(FrameParser &proto) override;

    void reboot();
    void runFW();
protected:

};



class IDBinUpdate : public IDBin
{
    Q_OBJECT
public:
    typedef struct __attribute__((packed)){
       uint16_t lastNumMsg = 0;
       uint32_t lastOffset = 0;
       uint8_t type = 0;
       uint8_t rcvNumMsg = 0;
    } ID_UPGRADE_V0;

    explicit IDBinUpdate() : IDBin() {
    }

    ID id() override { return ID_UPDATE; }
    Resp  parsePayload(FrameParser &proto) override;

    void setUpdate(QByteArray fw);
    int availSend() {return _fw.length() - _currentFwOffset; }
    int progress() {
        if(_fw.length() != 0) {
            return 100*_currentFwOffset / _fw.length();
        }

        return 0;
    }

    ID_UPGRADE_V0 getDeviceProgress() { return _progress; }

    uint16_t currentNumPacket() {
        return _currentNumPacket;
    }

    int currentFwOffset() {
        return _currentFwOffset;
    }

    void setUpgradeNewPoint(uint16_t num_packet, int offset) {
        _currentNumPacket = num_packet + 1;
        _currentFwOffset = offset;
    }

public slots:
    bool putUpdate(bool is_auto_offset = true);

protected:
    uint16_t _currentNumPacket = 0;
    int _currentFwOffset = 0;
    QByteArray _fw;
    const uint16_t _packetSize = 32*3;

    ID_UPGRADE_V0 _progress;
};

class IDBinVoltage : public IDBin
{
    Q_OBJECT
public:
    explicit IDBinVoltage() : IDBin() {
    }

    ID id() override { return ID_VOLTAGE; }
    Resp  parsePayload(FrameParser &proto) override;

    float voltage(int v_id) { return _v[v_id]; }
protected:
    float _v[255];
};


class IDBinNav : public IDBin
{
    Q_OBJECT
public:
    explicit IDBinNav() : IDBin() {
    }

    ID id() override { return ID_NAV; }
    Resp  parsePayload(FrameParser &proto) override;

    double latitude, longitude;
    float accuracy;
protected:
};

class IDBinDVL : public IDBin
{
    Q_OBJECT
public:
    explicit IDBinDVL() : IDBin() {
    }

    ID id() override { return ID_DVL_VEL; }
    Resp  parsePayload(FrameParser &proto) override;

    typedef struct   __attribute__((packed)) {
        uint8_t num;
        uint8_t flags;
        float velocity = NAN;
        float uncertainty = NAN;
        float dt = NAN;
        float distance = NAN;
        uint8_t amplitude;
        uint8_t mode;
        uint8_t coherence[4];
        int8_t difference[4];
    } BeamSolution;

    typedef struct  __attribute__((packed)) {
        union {
            struct {
                uint32_t xValid : 1;
                uint32_t yValid : 1;
                uint32_t zValid : 1;
                uint32_t z1Valid : 1;
                uint32_t z2Valid : 1;
                uint32_t isBottomTrack : 1;
                uint32_t beam1Valid : 1;
                uint32_t beam2Valid : 1;
                uint32_t beam3Valid : 1;
                uint32_t beam4Valid : 1;
                uint32_t beam5Valid : 1;
                uint32_t beam6Valid : 1;
                uint32_t beamAvailNum : 3;
                uint32_t beamSolutionNum : 3;
            };
            uint32_t flags;
        };

        struct {
            uint32_t timestamp_ms;
            float deltaT;
            float latency;
        } solutionTime  __attribute__((packed));

        struct {
            float x;
            float y;
            float z;
            float z1;
            float z2;
        } velocity  __attribute__((packed));

        struct {
            float x;
            float y;
            float z;
            float z1;
            float z2;
        } uncertainty  __attribute__((packed));

        struct {
            float z;
            float z1;
            float z2;
        } distance  __attribute__((packed));
    } DVLSolution;

    float velX() { return vel_x; }
    float velY() { return vel_y; }
    float velZ() { return vel_z; }
    float dist() { return _dist; }

    BeamSolution* beams() { return _beams; }
    uint16_t beamsCount() { return _beamCount; }

    DVLSolution dvlSolution() {
        return _dvlSolution;
    }

protected:
    float vel_x, vel_y, vel_z;
    float _dist;

    BeamSolution _beams[4] = {};
    DVLSolution _dvlSolution;
    uint16_t _beamCount = 0;
    float test_bias = 0;
};

class IDBinDVLMode : public IDBin
{
    Q_OBJECT
public:
    explicit IDBinDVLMode() : IDBin() {
    }

    ID id() override { return ID_DVL_MODE; }
    Resp  parsePayload(FrameParser &proto) override;

    typedef struct  __attribute__((packed)) {
        uint8_t id = 0;
        uint8_t selection = 1; // 0 - not select, 1 - always
        int8_t gain = 0;
        int8_t curve = 0;
        uint16_t reserved = 0;
        float start = 0, stop = 0; // 0 - ignore
    } DVLModeSetup;

    void setModes(bool ismode1, bool ismode2, bool ismode3, bool ismode4, float range_mode4);

protected:

};



class IDBinUsblSolution : public IDBin
{
    Q_OBJECT
public:
    explicit IDBinUsblSolution() : IDBin() {
    }

    ID id() override { return ID_USBL_SOLUTION; }
    Resp  parsePayload(FrameParser &proto) override;

    struct UsblSolution {
        uint8_t id = 0;
        uint8_t role = 0;
        uint16_t watermark = 0;

        int64_t timestamp_us = 0;
        uint32_t ping_counter = 0;
        int64_t carrier_counter = 0;

        float distance_m = 0;
        float distance_unc = 0;

        float azimuth_deg = 0;
        float azimuth_unc = 0;

        float elevation_deg = 0;
        float elevation_unc = 0;

        float snr = 0;

        float x_m = NAN;
        float y_m = NAN;
        double latitude_deg = NAN;
        double longitude_deg = NAN;
        float depth_m = NAN;

        float usbl_yaw = NAN;
        float usbl_pitch = NAN;
        float usbl_roll = NAN;

        double usbl_latitude = NAN;
        double usbl_longitude = NAN;
        uint32_t last_iTOW = 0;

        float beacon_n = NAN;
        float beacon_e = NAN;
    } __attribute__((packed));

    struct USBLRequestBeacon {
        uint8_t id = 0; // 0 is promisc mode
        uint8_t reserved = 0;
        uint16_t watermark = 0;
        double latitude_deg = NAN;
        double longitude_deg = NAN;
        float external_heading_deg = NAN;
        float force_beacon_depth_m = NAN;
        float external_pitch = NAN;
        float external_roll = NAN;
    }  __attribute__((packed));

    struct BeaconActivationResponce {
        uint8_t id = 0; // 0 is promisc mode
        uint8_t reserved = 0;
        uint16_t reserved1 = 0;
    }  __attribute__((packed));

    struct BeaconActivate {
        float timeout_s = 2;
    }  __attribute__((packed));

    UsblSolution usblSolution() {
        return _usblSolution;
    }

    void askBeacon(USBLRequestBeacon ask);
    void enableBeaconOnce(float timeout);

protected:
    UsblSolution _usblSolution;
    BeaconActivationResponce _beaconResponcel;
};


#endif // IDBINNARY_H
