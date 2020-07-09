#ifndef IDBINNARY_H
#define IDBINNARY_H

#include <QObject>
#include <ProtoBinnary.h>

using namespace KoggerBinnaryProtocol;

class IDBin : public QObject
{
    Q_OBJECT
public:
    explicit IDBin(ProtIn* proto, QObject *parent = nullptr);
    ~IDBin();
    void setProto(ProtIn* proto);
    Resp  parse();

    virtual ID id() = 0;
    virtual bool isSettable() { return false; }
    virtual bool isSettup() { return false; }
    virtual bool isRequestable() { return true; }

    Type lastType() {return m_lastType;}
    Version lastVersion() {return m_lastVersion;}
    Resp lastResp() {return m_lastResp;}

    virtual void simpleRequest(Version ver);
    virtual void requestAll() {
        simpleRequest(v0);
    }

signals:
    void updateContent(Type type, Version ver, Resp resp);
    void dataSend(QByteArray data);

protected:
    const U4 m_key = 0xC96B5D4A;

    ProtIn* m_proto;
    Type m_lastType;
    Version m_lastVersion;
    Resp m_lastResp;
    QList<Version> availableVer;

    virtual Resp  parsePayload(ProtIn &proto) = 0;
    virtual void requestSpecific(ProtOut &proto_out) { Q_UNUSED(proto_out) }

    bool checkKeyConfirm(U4 key) { return (key == m_key); }
    void appendKey(ProtOut &proto_out);

    void sendDataProcessing(ProtOut &proto_out);
};



class IDBinTimestamp : public IDBin
{
    Q_OBJECT
public:
    explicit IDBinTimestamp(ProtIn* proto, QObject *parent = nullptr) : IDBin(proto, parent) {
    }

    ID id() override { return ID_TIMESTAMP; }
    Resp  parsePayload(ProtIn &proto) override;

    uint32_t timestamp() { return m_timestamp; }
protected:
    uint32_t m_timestamp;
};



class IDBinDist : public IDBin
{
    Q_OBJECT
public:
    explicit IDBinDist(ProtIn* proto, QObject *parent = nullptr) : IDBin(proto, parent) {
    }

    ID id() override { return ID_DIST; }
    Resp  parsePayload(ProtIn &proto) override;

    uint32_t dist_mm() { return m_dist_mm; }
protected:
    uint32_t m_dist_mm;
};



class IDBinChart : public IDBin
{
    Q_OBJECT
public:
    explicit IDBinChart(ProtIn* proto, QObject *parent = nullptr) : IDBin(proto, parent) {
    }

    ID id() override { return ID_CHART; }
    Resp  parsePayload(ProtIn &proto) override;

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

    uint8_t* rawData() { return m_fillChart; }

protected:
    uint16_t m_seqOffset, m_sampleResol, m_absOffset;
    uint16_t m_chartSizeIncr;
    uint16_t m_chartSize;
    uint8_t m_fillChart[5000];
    uint8_t m_completeChart[5000];
    bool m_isCompleteChart;
};



class IDBinAttitude : public IDBin
{
    Q_OBJECT
public:
    explicit IDBinAttitude(ProtIn* proto, QObject *parent = nullptr) : IDBin(proto, parent) {
    }

    ID id() override { return ID_ATTITUDE; }
    Resp  parsePayload(ProtIn &proto) override;

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
    explicit IDBinTemp(ProtIn* proto, QObject *parent = nullptr) : IDBin(proto, parent) {
    }

    ID id() override { return ID_TEMP; }
    Resp  parsePayload(ProtIn &proto) override;

    float temp() { return m_temp; }
protected:
    float m_temp;
};



class IDBinDataset : public IDBin
{
    Q_OBJECT
public:
    explicit IDBinDataset(ProtIn* proto, QObject *parent = nullptr) : IDBin(proto, parent) {
    }

    ID id() override { return ID_DATASET; }
    Resp  parsePayload(ProtIn &proto) override;

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
        moveFlag(ch_id,  MASK_DIST_V0 | MASK_CHART_V0 | MASK_DIST_SDDBT | MASK_DIST_SDDBT_P2);
//        setFlag(0, MASK_DIST_SDDBT);
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

    void setTemp_v0(U1 ch_id) { setFlag(ch_id, MASK_TEMP_V0); }
    void resetTemp_v0(U1 ch_id) { resetFlag(ch_id, MASK_TEMP_V0); }
    bool getTemp_v0(U1 ch_id) { return flag(ch_id, MASK_TEMP_V0); }

    void setTimestamp_v0(U1 ch_id) { setFlag(ch_id, MASK_TIMESTAMP_V0); }
    void resetTimestamp_v0(U1 ch_id) { resetFlag(ch_id, MASK_TIMESTAMP_V0); }
    bool getTimestamp_v1(U1 ch_id) { return flag(ch_id, MASK_TIMESTAMP_V0); }

    void setSDDBT(U1 ch_id) {
        setFlag(ch_id, MASK_DIST_SDDBT);
        moveFlag(ch_id,  MASK_DIST_V0 | MASK_CHART_V0 | MASK_DIST_SDDBT | MASK_DIST_SDDBT_P2);
//        setFlag(0, MASK_DIST_V0);
    }
    void resetSDDBT(U1 ch_id) { resetFlag(ch_id, MASK_DIST_SDDBT); }
    bool getSDDBT(U1 ch_id) { return flag(ch_id, MASK_DIST_SDDBT); }

    void setSDDBT_P2(U1 ch_id) {
        setFlag(ch_id, MASK_DIST_SDDBT_P2);
        moveFlag(ch_id,  MASK_DIST_V0 | MASK_CHART_V0 | MASK_DIST_SDDBT | MASK_DIST_SDDBT_P2);
    }
    void resetSDDBT_P2(U1 ch_id) { resetFlag(ch_id, MASK_DIST_SDDBT_P2); }
    bool getSDDBT_P2(U1 ch_id) { return flag(ch_id, MASK_DIST_SDDBT_P2); }

    void commit() {
        sendChannel(1, period(1), mask(1));
        sendChannel(2, period(2), mask(2));
    }

    uint32_t period(U1 ch_id);
    void setPeriod(U1 ch_id, uint32_t period);

protected:
    Channel m_channel[3];

    void sendChannel(U1 ch_id, uint32_t period, uint32_t mask);
    virtual void requestSpecific(ProtOut &proto_out) override { proto_out.write<U1>(0); }
};



class IDBinDistSetup : public IDBin
{
    Q_OBJECT
public:
    explicit IDBinDistSetup(ProtIn* proto, QObject *parent = nullptr) : IDBin(proto, parent) {
    }

    ID id() override { return ID_DIST_SETUP; }
    Resp  parsePayload(ProtIn &proto) override;

    void setRange(uint32_t start_offset, uint32_t max_dist);

    int max() { return m_maxDist; }
    void setMax(uint32_t max_dist) { setRange(deadZone(), max_dist); }

    int deadZone() { return m_startOffset; }
    void setDeadZone(uint32_t dead_zone) { setRange(dead_zone, max()); }
protected:
    uint32_t m_startOffset = 250;
    uint32_t m_maxDist = 50000;
};



class IDBinChartSetup : public IDBin
{
    Q_OBJECT
public:
    explicit IDBinChartSetup(ProtIn* proto, QObject *parent = nullptr) : IDBin(proto, parent) {
    }

    ID id() override { return ID_CHART_SETUP; }
    Resp  parsePayload(ProtIn &proto) override;

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



class IDBinTransc : public IDBin
{
    Q_OBJECT
public:
    explicit IDBinTransc(ProtIn* proto, QObject *parent = nullptr) : IDBin(proto, parent) {
    }

    ID id() override { return ID_TRANSC; }
    Resp  parsePayload(ProtIn &proto) override;

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
    explicit IDBinSoundSpeed(ProtIn* proto, QObject *parent = nullptr) : IDBin(proto, parent) {
    }

    ID id() override { return ID_SND_SPEED; }
    Resp  parsePayload(ProtIn &proto) override;

    void setSoundSpeed(U4 snd_spd);
    int getSoundSpeed() { return m_soundSpeed; }
protected:
    U4 m_soundSpeed = 1500000;
};



class IDBinUART : public IDBin
{
    Q_OBJECT
public:
    explicit IDBinUART(ProtIn* proto, QObject *parent = nullptr) : IDBin(proto, parent) {
    }

    ID id() override { return ID_UART; }
    Resp  parsePayload(ProtIn &proto) override;

    typedef struct {
        U1 id = 0;
        U4 baudrate = 115200;
        U1 dev_address = 0;
    } UART;

    void setBaudrate(U4 baudrate);
protected:
    UART m_uart[3];
};



class IDBinMark : public IDBin
{
    Q_OBJECT
public:
    explicit IDBinMark(ProtIn* proto, QObject *parent = nullptr) : IDBin(proto, parent) {
    }

    ID id() override { return ID_MARK; }
    Resp  parsePayload(ProtIn &proto) override;

    bool mark() { return (m_mark & 0x1) == 0x1; }
    void setMark();

protected:
    U1 m_mark = 0;
};



class IDBinFlash : public IDBin
{
    Q_OBJECT
public:
    explicit IDBinFlash(ProtIn* proto, QObject *parent = nullptr) : IDBin(proto, parent) {
    }

    ID id() override { return ID_FLASH; }
    Resp  parsePayload(ProtIn &proto) override;

    void flashing();
    void restore();
    void erase();

protected:
};



class IDBinBoot : public IDBin
{
    Q_OBJECT
public:
    explicit IDBinBoot(ProtIn* proto, QObject *parent = nullptr) : IDBin(proto, parent) {
    }

    ID id() override { return ID_BOOT; }
    Resp  parsePayload(ProtIn &proto) override;

    void reboot();
    void runFW();
protected:
};



class IDBinUpdate : public IDBin
{
    Q_OBJECT
public:
    explicit IDBinUpdate(ProtIn* proto, QObject *parent = nullptr) : IDBin(proto, parent) {
    }

    ID id() override { return ID_UPDATE; }
    Resp  parsePayload(ProtIn &proto) override;

    void setUpdate(QByteArray fw);
    int availSend() {return _fw.length() - _fw_offset; }
    int progress() {
        if(_fw.length() != 0) {
            return 100*_fw_offset / _fw.length();
        }

        return 0;
    }

public slots:
    bool putUpdate();

protected:
    uint16_t _nbr_packet = 0;
    QByteArray _fw;
    int _fw_offset = 0;
};



class IDBinNav : public IDBin
{
    Q_OBJECT
public:
    explicit IDBinNav(ProtIn* proto, QObject *parent = nullptr) : IDBin(proto, parent) {
    }

    ID id() override { return ID_NAV; }
    Resp  parsePayload(ProtIn &proto) override;

    double latitude, longitude;
    float accuracy;
protected:

};


#endif // IDBINNARY_H
