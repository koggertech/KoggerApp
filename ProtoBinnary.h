#ifndef KOGGERBINNARYPARSER_H
#define KOGGERBINNARYPARSER_H

#include "stdint.h"
#include "stddef.h"
#include <cmath>

inline void fletcher(uint8_t* buf, uint16_t len, uint8_t* ch1, uint8_t* ch2) {
    uint8_t check1 = 0, check2 = 0;
    for(uint16_t i = 0; i < len; i++) {
        check1 += buf[i];
        check2 += check1;
    }

    *ch1 = check1;
    *ch2 = check2;
}

namespace Parsers {

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
    ID_ENCODER = 0x08,

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

    ID_EVENT = 0x30,
    ID_VOLTAGE = 0x31,

    ID_NAV = 0x64,
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


class FrameParser {
public:
    typedef enum {
        ProtoNone,
        ProtoKP1,
        ProtoKP2,
        ProtoNMEA,
        ProtoUBX
    } ProtoID;

    FrameParser() {
        _frameChar = (char*)_frame;
        setContext(nullptr, 0);
        resetState();
    }

    void headerSync(uint8_t b) {
        if(b == 0xBB) { switchToKP1(); }
        else if(b == 0xCC) { switchToKP2(); }
        else if(b == 0xB5) { switchToUBX(); }
        else if(b == '$') { switchToNMEA(); }
    }

    void headerReSync(uint8_t b) {
        _counter.frameReSync++;
        resetState();
        headerSync(b);
    }

    void process() {
        _proto = ProtoNone;
        while (availContext() > 0) {
            uint8_t b = *_contextData;

            switch (_protoState) {
            case StateSync:
                headerSync(b);
                break;

            ////////////// KPv1 //////////////
            case StateKP1Sync:
                if(b == 0x55) { _protoState = StateKP1Header; }
                else { headerReSync(b); }
                break;
            case StateKP1Header:
                if(_frameLen == 5) {
                    _completeLen = b + (5 + 2);
                    _protoState = StateKP1Ending;
                }
                break;
            case StateKP1Ending:
                if(_frameLen == _completeLen) {
                    if(frameAppend(b)) {
                        checkAsKP1();
                    } else { }

                    incContext();
                    resetState();
                    return;
                }
                break;
            ////////////// KPv1 //////////////

            ////////////// KPv2 //////////////
            case StateKP2Sync:
                if(b == 0x55) { _protoState = StateKP2Header; }
                else { headerReSync(b); }
                break;
            case StateKP2Header:
                if(_frameLen == 3) {
                    _completeLen = *(int16_t*)(&_frame[3]) + (4 + 2);
                    _protoState = StateKP2Ending;
                }
                break;
            case StateKP2Ending:
                if(_frameLen == _completeLen) {
                    if(frameAppend(b)) {
                        checkAsKP2();
                    } else { }

                    incContext();
                    resetState();
                    return;
                }
                break;
            ////////////// KPv2 //////////////

            ////////////// UBX //////////////
            case StateUBXSync:
                if(b == 0x62) { _protoState = StateUBXHeader; }
                else { headerReSync(b); }
                break;

            case StateUBXHeader:
                if(_frameLen == 4) {
                    _completeLen = b + (5 + 2);
                    _protoState = StateUBXEnding;
                }
                break;
            case StateUBXEnding:
                if(_frameLen == _completeLen) {
                    if(frameAppend(b)) {
                        checkAsUBX();
                    } else { }

                    incContext();
                    resetState();
                    return;
                }
                break;
            ////////////// UBX //////////////


            case StateNmeaPayload:
                if(b == '*') {
                    _protoState = StateNmeaEnding;
                    _completeLen = _frameLen + 2;
                }
                break;
            case StateNmeaEnding:
                if(_frameLen == _completeLen) {
                    if(frameAppend(b) && frameAppend('\r') && frameAppend('\n')) {
                        checkAsNMEA();
                    } else { }

                    incContext();
                    resetState();
                    return;
                }
                break;
            }

            if(_protoState != StateSync) {
                if(!frameAppend(b)) {
                }
            }
            incContext();
        }
    }

    void setContext(uint8_t* data, uint32_t len) {
        _contextData = data;
        _contextLen = len;
    }

    uint32_t availContext() {
        return _contextLen;
    }

    int16_t readAvailable() {
        return _readMaxLen - _readOffset;
    }

    ProtoID proto() { return _proto; }

    bool isCompleteAs(ProtoID proto_flag) {
        return proto_flag == proto();
    }

    bool completeAsKBP() {
        return proto() == ProtoKP1;
    }

    bool isCompleteAsUBX() {
        return isCompleteAs(ProtoUBX);
    }

    bool completeAsNMEA() {
        return proto() == ProtoNMEA;
    }

    void resetComplete() { _proto = ProtoNone; }

    uint8_t* frame() { return _frame; }
    uint16_t frameLen() { return _frameLen; }
    uint32_t frameError() { return _counter.frameError;}
    uint32_t binError() { return _counter.checkErrorKP1;}
    uint32_t NMEAError() { return _counter.checkErrorNMEA;}
    uint32_t binComplete() { return _counter.completeKP1;}
    uint32_t NMEAComplete() { return _counter.completeNMEA;}

protected:
    enum ProtoState {
        StateSync,

        StateKP1Sync,
        StateKP1Header,
        StateKP1Ending,

        StateKP2Sync,
        StateKP2Header,
        StateKP2Ending,

        StateUBXSync,
        StateUBXHeader,
        StateUBXEnding,

        StateNmeaPayload,
        StateNmeaEnding,
    } _protoState;

    ProtoID _proto;

    uint8_t* _contextData;
    int32_t _contextLen;
    uint8_t _frame[1024];
    char* _frameChar;
    int16_t _frameLen;
    int16_t _frameMaxLen;
    int16_t _payloadLen;
    int16_t _completeLen;
    int16_t _readOffset;
    int16_t _readMaxLen;

    struct {
        uint32_t frameError = 0;
        uint32_t frameReSync = 0;

        uint32_t completeKP1 = 0;
        uint32_t checkErrorKP1 = 0;

        uint32_t completeKP2 = 0;
        uint32_t checkErrorKP2 = 0;

        uint32_t completeUBX = 0;
        uint32_t checkErrorUBX = 0;

        uint32_t completeNMEA = 0;
        uint32_t checkErrorNMEA = 0;
    } _counter;

    void incContext() {
        _contextData++;
        _contextLen--;
    }

    bool frameAppend(uint8_t b) {
        _frame[_frameLen] = b;
        _frameLen++;
        if(_frameLen > _frameMaxLen) {
            _counter.frameError++;
            resetFrame();
            resetState();
            return false;
        }

        return true;
    }

    void resetFrame() {
        _proto = ProtoNone;
        _frameLen = 0;
    }

    void switchToKP1() {
        _protoState = StateKP1Sync;
        _frameMaxLen = 255 + 8;
        resetFrame();
    }

    void switchToKP2() {
        _protoState = StateKP1Sync;
        _frameMaxLen = 255 + 8;
        resetFrame();
    }

    void switchToUBX() {
        _protoState = StateUBXSync;
        _frameMaxLen = 255 + 8;
        resetFrame();
    }

    void switchToNMEA() {
        _protoState = StateNmeaPayload;
        _frameMaxLen = 82;
        resetFrame();
    }

    void resetState() {
        _protoState = StateSync;
        _frameMaxLen = 2; // max sync len
    }

    bool checkFletcher(uint8_t* buf, uint16_t len, uint8_t ch1, uint8_t ch2) {
            uint8_t check1 = 0, check2 = 0;
            fletcher(buf, len, &check1, &check2);
            return (ch1 == check1) && (ch2 == check2);
        }

    bool checkAsKP1() {
        uint8_t check1 = 0, check2 = 0;
        uint16_t check1Pos = _frameLen - 2;
        for(uint16_t i = 2; i < check1Pos; i++) {
            check1 += _frame[i];
            check2 += check1;
        }

        bool res = (check1 == _frame[check1Pos]) && (check2 == _frame[check1Pos + 1]);
        if(res) {
            _proto = ProtoKP1;
            _readOffset = 6;
            _readMaxLen = _frameLen - 2;
            _payloadLen = _readMaxLen - _readOffset;
            _counter.completeKP1++;
        } else {
            _counter.checkErrorKP1++;
            _proto = ProtoNone;
        }

        return res;
    }

    bool checkAsKP2() {
        uint8_t check1 = 0, check2 = 0;
        uint16_t check1Pos = _frameLen - 2;
        for(uint16_t i = 2; i < check1Pos; i++) {
            check1 += _frame[i];
            check2 += check1;
        }

        bool res = (check1 == _frame[check1Pos]) && (check2 == _frame[check1Pos + 1]);
        if(res) {
            _proto = ProtoKP2;
            _readOffset = _frame[4] + (4 + 3);
            _readMaxLen = _frameLen - 2;
            _payloadLen = _readMaxLen - _readOffset;
            _counter.completeKP2++;
        } else {
            _counter.checkErrorKP2++;
            _proto = ProtoNone;
        }

        return res;
    }

    bool checkAsUBX() {
            bool res = checkFletcher(&_frame[2], _frameLen - 4, _frame[_frameLen - 2], _frame[_frameLen - 1]);
            if(res) {
                _proto = ProtoUBX;
                _readOffset = 6;
                _readMaxLen = _frameLen - 2;
                _payloadLen = _readMaxLen - _readOffset;
                _counter.completeUBX++;
            } else {
                _counter.checkErrorUBX++;
                _proto = ProtoNone;
            }

            return res;
        }

    bool checkAsNMEA() {
        uint16_t checkStopPos = _frameLen - 5;
        uint8_t checkCalck = 0;

        for(uint16_t i = 1; i < checkStopPos; i++) {
            checkCalck ^= _frame[i];
        }

        uint8_t frameCheck1 = fromHexChar(_frame[_frameLen - 4]);
        uint8_t frameCheck2 = fromHexChar(_frame[_frameLen - 3]);

        bool res = checkCalck == ((frameCheck1 << 4) + (frameCheck2));

        if(res) {
            uint16_t i = 5;
            const uint16_t max_id_len = 8;
            while(i <= max_id_len && _frame[i] != ',') {
                i++;
            }
            if(i > max_id_len) { res = false; }
            _readOffset = i + 1;
            _payloadLen = checkStopPos - _readOffset;
        }

        if(res) {
            _proto = ProtoNMEA;

            _readMaxLen = checkStopPos;
            _counter.completeNMEA++;
        } else {
            _counter.checkErrorNMEA++;
            _proto = ProtoNone;
        }

        return res;
    }

    uint8_t fromHexChar(uint8_t hex_char) {
         if (hex_char >= 'A' && hex_char <= 'F')
            return (hex_char + 10) - ('A');
          else if (hex_char >= 'a' && hex_char <= 'f')
            return (hex_char + 10) - ('a');
          else
            return hex_char - '0';
    }
};

class ProtoKP1 : public FrameParser {
public:
    ProtoKP1() {}

    uint8_t route() const { return _frame[2]; }
    uint8_t mode() const { return _frame[3]; }
    ID id() const { return (ID)(_frame[4]); }
    Type type() const { return (Type)(mode() & 0x3);}
    Version ver() const { return (Version)((mode() >> 3) & 0x7);}
    bool mark() const { return ((mode() >> 6) & 0x1) == 0x1;}
    bool resp() const { return ((mode() >> 7) & 0x1) == 0x1;}
    uint16_t payloadLen() { return _payloadLen; }

    bool isOut() { return _isOut;}
protected:
    bool _isOut = false;

    void setRoute(uint8_t route) {_frame[2] = route;}
    void setMode(uint8_t mode) { _frame[3] = mode; }
    void setMode(Type type, Version ver, bool response) {
        setMode((uint8_t)(((uint8_t)type & 0x3) | (((uint8_t)ver & 0x7) << 3) | (((uint8_t)response) << 7)));
    }
    void setId(ID id) { _frame[4] = id; }
    void setLen(uint8_t len) { _frame[5] = len; _payloadLen = len; }
};

class ProtoKP2 : public FrameParser {
public:
    ProtoKP2() {}

    uint8_t route() const { return _frame[2]; }
    uint8_t mode() const { return _frame[_frame[4] + 4]; }
    ID id() const { return (ID)(idver() >> 3); }
    Type type() const { return (Type)(mode() & 0x3); }
    Version ver() const { return (Version)(idver() & (uint16_t)0x3); }
    bool mark() const { return ((mode() >> 3) & 0x1) == 0x1; }
    bool resp() const { return ((mode() >> 2) & 0x1) == 0x1; }
    uint16_t payloadLen() { return _payloadLen; }

protected:
    uint16_t idver() const { return *(uint16_t*)(&_frame[_frame[4] + 5]); }

    void setRoute(uint8_t route) {_frame[2] = route;}
    void setMode(uint8_t mode) { _frame[3] = mode; }
    void setMode(Type type, Version ver, bool response) {
        setMode((uint8_t)(((uint8_t)type & 0x3) | (((uint8_t)ver & 0x7) << 3) | (((uint8_t)response) << 7)));
    }
    void setId(ID id) { _frame[4] = id; }
    void setLen(uint8_t len) { _frame[5] = len; _payloadLen = len; }
};


class ProtoKP1In : public ProtoKP1 {
public:
    ProtoKP1In() { _isOut = false; }

    template<typename T>
    T read() {
        T *val = (T *)(&_frame[_readOffset]);
        _readOffset += sizeof (T);
        return *val;
    }

    void read(uint8_t* b, uint16_t len) {
        for(uint16_t i = 0; i < len; i++) {
            b[i] = _frame[_readOffset];
            _readOffset++;
        }
    }
protected:
};

class ProtoUBX : public FrameParser {
public:
    ProtoUBX() {}

    uint8_t msgId() const { return (ID)(_frame[3]); }
    uint8_t msgClass() const { return (ID)(_frame[2]); }
    uint16_t payloadLen() { return _payloadLen; }

    void readSkip(uint16_t skip_nbr) {_readOffset += skip_nbr;}

    template<typename T>
    T read() {
        T *val = (T *)(&_frame[_readOffset]);
        _readOffset += sizeof (T);
        return *val;
    }

    void read(uint8_t* b, uint16_t len) {
        for(uint16_t i = 0; i < len; i++) {
            b[i] = _frame[_readOffset];
            _readOffset++;
        }
    }
protected:
};

class ProtoNMEA : public FrameParser {
public:
    ProtoNMEA() {}

    uint32_t hashId() {
        return (*((uint32_t*)(&_frame[3]))) & 0x00FFFFFF;
    }

    uint16_t hashTalker() {
        return (*((uint16_t*)(&_frame[1])));
    }

    bool isEqualId(const char* id_test) {
        uint32_t hash_id = hashId();
        uint32_t test_id = (*((uint32_t*)(id_test)) & 0x00FFFFFF);
        return test_id == hash_id;
    }

    bool isEqualTalker(const char* id_test) {
        return (*((uint16_t*)(id_test))) == hashTalker();
    }

    void skip() {
        int16_t i = 0;
        while(i < _readMaxLen && _frame[_readOffset] != ',') {
            _readOffset++;
            i++;
        }

        _readOffset++;
    }

    double readLatitude() {
        int16_t deg_h = _frame[_readOffset++] - '0';
        int16_t deg_l = _frame[_readOffset++] - '0';

        int16_t min_h = _frame[_readOffset++] - '0';
        int16_t min_l = _frame[_readOffset++] - '0';

        if(_frame[_readOffset++] != '.') { return std::nan(""); }

        int16_t i = 0;
        int32_t min_part = 0;
        int32_t exp_sum = 1;
        const uint16_t max_part_len = 10;
        while(i <= max_part_len && _frame[_readOffset] != ',') {
            min_part *= 10;
            min_part += _frame[_readOffset++] - '0';
            i++;
            exp_sum *= 10;
        }

        if(i > max_part_len) { return std::nan(""); }

        _readOffset++;

        int16_t q = _frame[_readOffset++];
        _readOffset++;

        if(q != 'N' && q != 'S') { return std::nan(""); }

        double lat = (double)(deg_h*10 + deg_l) + (double)(min_h*10 + min_l)/60.0 + ((double)min_part)/(60.0*exp_sum);
        if(q == 'S') { lat = -lat; }
        return lat;
    }

    double readLongitude() {
        int16_t deg_h = _frame[_readOffset++] - '0';
        int16_t deg_m = _frame[_readOffset++] - '0';
        int16_t deg_l = _frame[_readOffset++] - '0';

        int16_t min_h = _frame[_readOffset++] - '0';
        int16_t min_l = _frame[_readOffset++] - '0';

        if(_frame[_readOffset++] != '.') { return std::nan(""); }

        int16_t i = 0;
        int32_t min_part = 0;
        int32_t exp_sum = 1;
        const uint16_t max_part_len = 10;
        while(i <= max_part_len && _frame[_readOffset] != ',') {
            min_part *= 10;
            min_part += _frame[_readOffset++] - '0';
            i++;
            exp_sum *= 10;
        }

        if(i > max_part_len) { return std::nan(""); }

        _readOffset++;

        int16_t q = _frame[_readOffset++];
        _readOffset++;

        if(q != 'W' && q != 'E') { return std::nan(""); }

        double lon = (double)(deg_h*100 + deg_m*10 + deg_l) + (double)(min_h*10 + min_l)/60.0 + ((double)min_part)/(60.0*exp_sum);
        if(q == 'W') { lon = -lon; }
        return lon;
    }

    uint32_t readTimems() {
        int16_t h_h = _frame[_readOffset++] - '0';
        int16_t h_l = _frame[_readOffset++] - '0';

        int16_t m_h = _frame[_readOffset++] - '0';
        int16_t m_l = _frame[_readOffset++] - '0';

        int16_t s_h = _frame[_readOffset++] - '0';
        int16_t s_l = _frame[_readOffset++] - '0';

        if(_frame[_readOffset++] != '.') { return 0xFFFFFFFF; }

        int16_t subs_h = _frame[_readOffset++] - '0';
        int16_t subs_l = _frame[_readOffset++] - '0';

        _readOffset++;

        return (uint32_t)(h_h*10 + h_l)*3600000 + (uint32_t)(m_h*10 + m_l)*60000 + (uint32_t)(s_h*10 + s_l)*1000 + subs_h*100 + subs_l*10;
    }

    char readChar() {
        char c = _frame[_readOffset++];
        _readOffset++;
        return c;
    }

protected:
};


class ProtoBinOut : public ProtoKP1
{
public:
    explicit ProtoBinOut() {  _isOut = true;  }

    void create(Type type, Version ver, ID id, uint8_t route) {
        _frame[0] = 0xbb;
        _frame[1] = 0x55;
        setRoute(route);
        setMode(type, ver, true);
        setId(id);

        _frameLen = 6;
        _frameMaxLen = 254;
    }

    int16_t frameSpaceAvail() {
        return _frameMaxLen - _frameLen;
    }

    template<typename T>
    void write(T data) {
        if(frameSpaceAvail() > (int16_t)sizeof (T)) {
            uint8_t* ptr_data = (uint8_t*)(&data);
            for(uint8_t i = 0; i < sizeof (T); i++) {
                _frame[_frameLen] = ptr_data[i];
                _frameLen++;
            }
        }
    }

    void end() {
        setLen(_frameLen - 6);

        uint8_t check1 = 0, check2 = 0;
        for(uint16_t i = 2; i < _frameLen; i++) {
            check1 += _frame[i];
            check2 += check1;
        }

        _frame[_frameLen] = check1;
        _frame[_frameLen + 1] = check2;

        _frameLen += 2;
    }
protected:
};

}

#endif // KOGGERBINNARYPARSER_H
