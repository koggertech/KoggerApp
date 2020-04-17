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

    void request(Version ver);
    virtual void requestAll() {
        request(v0);
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

protected:
    Channel m_channel[3];
};



class IDBinDistSetup : public IDBin
{
    Q_OBJECT
public:
    explicit IDBinDistSetup(ProtIn* proto, QObject *parent = nullptr) : IDBin(proto, parent) {
    }

    ID id() override { return ID_DIST_SETUP; }
    Resp  parsePayload(ProtIn &proto) override;
protected:
    uint32_t m_startOffset = 0;
    uint32_t m_maxDist = 0;
};



class IDBinChartSetup : public IDBin
{
    Q_OBJECT
public:
    explicit IDBinChartSetup(ProtIn* proto, QObject *parent = nullptr) : IDBin(proto, parent) {
    }

    ID id() override { return ID_CHART_SETUP; }
    Resp  parsePayload(ProtIn &proto) override;

protected:
    uint16_t m_sanpleCount = 0;
    uint16_t m_sanpleResolution = 0;
    uint16_t m_sanpleOffset = 0;
};



class IDBinTransc : public IDBin
{
    Q_OBJECT
public:
    explicit IDBinTransc(ProtIn* proto, QObject *parent = nullptr) : IDBin(proto, parent) {
    }

    ID id() override { return ID_TRANSC; }
    Resp  parsePayload(ProtIn &proto) override;

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

public slots:
    bool putUpdate();

protected:
    uint16_t _nbr_packet = 0;
    QByteArray _fw;
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
