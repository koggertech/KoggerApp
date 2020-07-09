#ifndef KOGGERBINNARYPARSER_H
#define KOGGERBINNARYPARSER_H

#include <QObject>

namespace KoggerBinnaryProtocol {

typedef uint8_t U1;
typedef int8_t S1;
typedef uint16_t U2;
typedef int16_t S2;
typedef uint32_t U4;
typedef int32_t S4;
typedef float F4;
typedef double D8;

typedef enum ID {
    ID_NONE = 0,

    ID_TIMESTAMP = 0x01,
    ID_DIST = 0x02,
    ID_CHART = 0x03,
    ID_ATTITUDE = 0x04,
    ID_TEMP = 0x05,
    ID_IMU = 0x06,
    ID_MAG = 0x07,
    ID_BARO = 0x08,

    ID_DATASET = 0x10,
    ID_DIST_SETUP = 0x11,
    ID_CHART_SETUP = 0x12,
    ID_DSP = 0x13,
    ID_TRANSC = 0x14,
    ID_SND_SPEED = 0x15,
    ID_PIN = 0x16,
    ID_BUS = 0x17,
    ID_UART = 0x18,
    ID_I2C = 0x19,
    ID_CAN = 0x1A,
    ID_GYR_SETUP = 0x1B,
    ID_ACC_SETUP = 0x1C,
    ID_MAG_SETUP = 0x1D,
    ID_BARO_SETUP = 0x1E,

    ID_VERSION = 0x20,
    ID_MARK = 0x21,
    ID_DIAG = 0x22,
    ID_FLASH = 0x23,
    ID_BOOT = 0x24,
    ID_UPDATE = 0x25,
    ID_NAV = 0x64
} ID;

typedef enum {
    typeReserved = 0, CONTENT = 1, SETTING = 2, GETTING = 3
} Type;

typedef enum {
    v0, v1, v2, v3, v4, v5, v6, v7
} Version;

typedef enum {
    respNone = 0,
    respOk,
    respErrorCheck,
    respErrorPayload,
    respErrorID,
    respErrorVersion,
    respErrorType,
    respErrorKey,
    respErrorRuntime
} Resp;


class ProtoBase  : public QObject {
    Q_OBJECT
public:
    ProtoBase(QObject *parent) : QObject(parent) {
    }

    ~ProtoBase() {
    }

    ID id() const {return _id;}
    Type type() const {return (Type)(_mode & 0x3);}
    Version ver() const {return (Version)((_mode >> 3) & 0x7);}
    bool mark() const {return ((_mode >> 6) & 0x1) == 0x1;}
    bool resp() const {return ((_mode >> 7) & 0x1) == 0x1;}

protected:
    typedef enum {
        SYNC1 = 0xBB,
        SYNC2 = 0x55
    } ProtoHeader;

    ID _id;
    uint8_t _mode;

    uint8_t mode(Type type, Version ver, bool response) {
        return (uint8_t)(((uint8_t)type & 0x3) | (((uint8_t)ver & 0x7) << 3) | (((uint8_t)response) << 7));
    }

    void checkUpdate(uint8_t b, uint8_t &c1,  uint8_t &c2) {
        c1 += b;
        c2 += c1;
    }

    void checkReset(uint8_t &c1,  uint8_t &c2) {
        c1 = 0;
        c2 = 0;
    }
};



class ProtOut : public ProtoBase
{
public:
    explicit ProtOut(QObject *parent = nullptr);

    void create(Type type, Version ver, ID id, uint8_t route) {
        resetState();
        _id = id;
        _mode = mode(type, ver, true);

        write<U1>(SYNC1);
        write<U1>(SYNC2);
        write<U1>(route);
        write<U1>(_mode);
        write<U1>(_id);
        uint8_t temp_len = 0;
        write<U1>(temp_len);
    }

    uint8_t writeAvail() {
        return (uint8_t)(254) - _writeOffset;
    }

    template<typename T>
    void write(T data) {
        if(writeAvail() > sizeof (T)) {
            uint8_t* ptr_data = (uint8_t*)(&data);
            for(uint8_t i = 0; i < sizeof (T); i++) {
                _sendData[_writeOffset] = ptr_data[i];
                _writeOffset++;
            }
        }
    }

    void end() {
        uint8_t _sendCheck1, _sendCheck2;

        uint8_t field_len = _writeOffset - 6;
        _sendData[5] = field_len; // hack

        checkReset(_sendCheck1, _sendCheck2);
        for(uint16_t i = 2; i < _writeOffset; i++) {
            checkUpdate(_sendData[i], _sendCheck1, _sendCheck2);
        }

        write<U1>(_sendCheck1);
        write<U1>(_sendCheck2);
    }

    uint8_t* data() {
        return _sendData;
    }

    uint8_t dataSize() {
        return _writeOffset;
    }

protected:
    uint8_t _writeOffset;
    uint8_t _sendData[256];

    void resetState() {
        _writeOffset = 0;
    }
};

class ProtIn : public ProtoBase
{

public:
    explicit ProtIn(QObject *parent = nullptr);

    int len() const {return _fieldLen;}


    template<typename T>
    T read() {
        T *val = (T *)(&_fillPayload[_readOffset]);
        _readOffset += sizeof (T);
        return *val;
    }

    void read(uint8_t* b, uint16_t len) {
        for(uint16_t i = 0; i < len; i++) {
            b[i] = _fillPayload[_readOffset];
            _readOffset++;
        }
    }

    uint16_t readAvailable() {
        return _fieldLen - _readOffset;
    }

public:
    Resp putByte(uint8_t b);

private:
    enum {
        StateSync1,
        StateSync2,
        StateRoute,
        StateLength,
        StateMode,
        StateID,
        StatePayload,
        StateCheck1,
        StateCheck2
    } State;

    uint8_t _fieldLen;
    uint8_t _fieldCheck1;
    uint8_t _fieldCheck2;
    uint8_t _fieldRoute;

    uint8_t _calcCheck1;
    uint8_t _calckCheck2;

    uint16_t _fillPosition;
    uint8_t _fillPayload[256];
    uint16_t _readOffset;
    uint16_t _checkErrorCnt;

    void resetState() {
        State = StateSync1;
        _fillPosition = 0;
        _readOffset = 0;
        checkReset(_calcCheck1, _calckCheck2);
    }

    void resetStateAsError() {
        _checkErrorCnt++;
        resetState();
    }

    bool isCheckValid() {
        if (_fieldCheck1 == _calcCheck1 && _fieldCheck2 == _calckCheck2) {
            return true;
        }
        return false;
    }

    void fillData(uint8_t b) {
        _fillPayload[_fillPosition] = b;
        _fillPosition++;
    }
};

} // namespace KoggerBinnaryProtocol

#endif // KOGGERBINNARYPARSER_H
