#ifndef KOGGERBINNARYPARSER_H
#define KOGGERBINNARYPARSER_H

#include "stdint.h"
#include "stddef.h"
#include <stdio.h>
#include <cmath>
#include "mav_link_conf.h"

inline void fletcher(uint8_t* buf, uint16_t len, uint8_t* ch1, uint8_t* ch2) {
    uint8_t check1 = 0, check2 = 0;
    for(uint16_t i = 0; i < len; i++) {
        check1 += buf[i];
        check2 += check1;
    }

    *ch1 = check1;
    *ch2 = check2;
}


inline uint16_t CRC16_MCRF4XX(uint8_t* buf, uint16_t len, uint16_t init = 0xffff) {
    uint16_t crc = init;

    while (len--) {
        uint8_t tmp;
        tmp = (*buf++) ^ (uint8_t)(crc &0xff);
        tmp ^= (tmp<<4);
        crc = (crc>>8) ^ (tmp<<8) ^ (tmp <<3) ^ (tmp>>4);
    }

    return crc;
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
typedef uint64_t U8;

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

    ID_STREAM = 0x40,

    ID_NAV = 0x64,

    ID_DVL_BEAM = 120,
    ID_DVL_VEL = 121,
    ID_DVL_MODE = 122,

    ID_USBL_SOLUTION = 0x65,

    ID_GFW = 200,

    sizer = 0xFFFF
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
        ProtoData,
        ProtoKP1,
        ProtoKP2,
        ProtoNMEA,
        ProtoUBX,
        ProtoMAVLink1,
        ProtoMAVLink2
    } ProtoID;

    FrameParser() {
        _frameChar = (char*)_frame;
        resetContext();
        resetState();
    }

    void process() {
        _proto = ProtoNone;
        if(_proxyState == ProxyWrapper) {
            _proxyState = ProxyContent;
        }
        while (availContextPrivate() > 0) {
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
                if(_frameLen == 4) {
                    uint16_t next_frame_len = *((uint16_t*)(&_frame[2]));
                    if(next_frame_len < sizeof(_frame)) {
                        _completeLen = next_frame_len - 1;
                        _protoState = StateKP2Ending;
                    } else {
                        headerReSync(b);
                    }
                }
                break;
            case StateKP2Ending:
                if(_frameLen == _completeLen) {
                    if(frameAppend(b)) {
                        checkAsKP2();
                    }
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


            ////////////// NMEA //////////////
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
                ////////////// NMEA //////////////

                ////////////// MAVLink //////////////
                case StateMAVLinkPayload:
                    if(_frame[0] == 0xFE) { // V1
                        _completeLen = 5 + b + 2;
                    } else { // V2
                        _completeLen = 9 + b + 2;
                    }
                    _protoState = StateMAVLinkEnding;
                    break;

                case StateMAVLinkEnding:
                    if(_frameLen == _completeLen) {
                        if(frameAppend(b)) {
                            checkAsMAVLink();
                        }

                        resetState();
                        incContext();
                        return;
                    }
                    break;
                ////////////// MAVLink //////////////
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

    void resetContext() {
        _proxyState = ProxyNone;

        _contextData = NULL;
        _contextLen = 0;

        _savedContextData = NULL;
        _savedContextLen = 0;
    }

    bool isNested() {
        return _proxyState == ProxyContent;
    }

    void setProxyContext(uint8_t* data, uint32_t len) {
        _savedContextData = _contextData + 1;
        _savedContextLen = _contextLen - 1;
        _contextData = data;
        _contextLen = len;
        _proxyState = ProxyWrapper;
    }


    int32_t availContext() {
        if(_contextLen == 0 && _savedContextData != NULL) {
            _contextData = _savedContextData;
            _contextLen = _savedContextLen;
            _savedContextLen = 0;
            _savedContextData = NULL;
            _proxyState = ProxyNone;
        }
        return _contextLen;
    }

    int16_t readAvailable() { return _readMaxPosition - _readPosition; }

    ProtoID proto() { return _proto; }

    bool isComplete() { return proto() != ProtoNone; }
    bool isCompleteAs(ProtoID proto_flag) { return proto_flag == proto(); }
    bool completeAsKBP() { return proto() == ProtoKP1; }
    bool completeAsKBP2() { return proto() == ProtoKP2; }
    bool isCompleteAsUBX() { return isCompleteAs(ProtoUBX); }
    bool isCompleteAsMAVLink() { return isCompleteAs(ProtoMAVLink1) || isCompleteAs(ProtoMAVLink2); }
    bool isCompleteAsNMEA() { return proto() == ProtoNMEA; }

    void resetComplete() { _proto = ProtoNone; }

    uint8_t* frame() { return _frame; }
    uint16_t frameLen() { return _frameLen; }
    uint32_t frameError() { return _counter.frameError;}
    uint32_t binError() { return _counter.checkErrorKP1;}
    uint32_t NMEAError() { return _counter.checkErrorNMEA;}
    uint32_t binComplete() { return _counter.completeKP1;}
    uint32_t NMEAComplete() { return _counter.completeNMEA;}
    uint32_t getCompleteTotal() { return _counter.completeTotal; }

    template<typename T>
    T read() {
        T res = {};
        read(&res);
        return res;
    }

    template<typename T>
    int read(T* data_struct) {
        int16_t avail = readAvailable();
        if(avail >= (int16_t)sizeof (T)) {
            *data_struct = *(T *)(&_frame[_readPosition]);
            _readPosition += (int16_t)sizeof (T);
            return sizeof (T);
        } else {
            *data_struct = {};
            read((uint8_t*)data_struct, avail);
            return avail;
        }

        return 0;
    }

    void read(uint8_t* b, int16_t len) {
        for(int16_t i = 0; i < len; i++) {
            b[i] = _frame[_readPosition];
            _readPosition++;
        }
    }

    uint8_t* read(int size) {
        uint8_t* ptr = &_frame[_readPosition];
        _readPosition += size;
        return ptr;
    }

    char readChar() {
        char c = _frame[_readPosition++];
        _readPosition++;
        return c;
    }

    void readSkip(uint16_t skip_nbr) {_readPosition += skip_nbr;}


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

    void write(void* ptr, uint16_t len) {
        uint8_t* ptr_data = (uint8_t*)(ptr);
        for(uint8_t i = 0; i < len; i++) {
            _frame[_frameLen] = ptr_data[i];
            _frameLen++;
        }
    }

    void writeZero(uint16_t len) {
        for(uint8_t i = 0; i < len; i++) {
            _frame[_frameLen] = 0;
            _frameLen++;
        }
    }

    uint16_t payloadLen() { return _payloadLen; }
    int16_t frameSpaceAvail() { return _frameMaxLen - _frameLen; }

    uint8_t route() const { return _address; }
    ID id() const { return _id; }
    Type type() const { return _type; }
    Version ver() const { return _ver;}
    bool mark() const { return _mark; }
    bool resp() const { return _resp; }
    uint16_t checksum() { return _checksum; }

    bool isStream() {
        return _optionFlags.isStream;
    }

    uint16_t streamId() {
        return _stream.id;
    }

    uint32_t streamOffset() {
        return _stream.offset;
    }

    bool isProxy() {
        return _optionFlags.isProxy;
    }

protected:
    typedef union {
        struct {
            uint16_t isProxy : 1;
            uint16_t isAddress : 1;
            uint16_t isStream : 1;
            uint16_t isLTime : 1;
            uint16_t isGTime : 2;
            uint16_t reserved : 10;
        };
        uint16_t val;
    } OP_Flags;

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

        StateMAVLinkPayload,
        StateMAVLinkEnding,
    } _protoState;

    ProtoID _proto;

    uint8_t* _contextData;
    int32_t _contextLen;

    uint8_t* _savedContextData;
    int32_t _savedContextLen;

    enum {
        ProxyNone,
        ProxyWrapper,
        ProxyContent,
        ProxyEnd
    } _proxyState = ProxyNone;

    uint8_t _frame[1024];
    char* _frameChar;
    int16_t _frameLen;
    int16_t _frameMaxLen;
    int16_t _payloadLen;
    int16_t _completeLen;
    int16_t _readPosition;
    int16_t _readMaxPosition;
    uint32_t _ltime;
    uint64_t _gtime;

    ID _id;
    Version _ver;
    Type _type;
    uint8_t _address, _from;
    bool _mark, _resp;
    uint16_t _checksum = 0;

    uint8_t _optionsLen;
    OP_Flags _optionFlags;

    struct {
        uint16_t id;
        uint32_t offset;
        union {
            struct {
                uint8_t isBytes : 1;
                uint8_t reserved : 7;
            };
            uint8_t val;
        } flags;
    } _stream;


    struct {
        uint32_t frameError = 0;
        uint32_t frameReSync = 0;
        uint32_t passByte = 0;

        uint32_t completeKP1 = 0;
        uint32_t checkErrorKP1 = 0;

        uint32_t completeKP2 = 0;
        uint32_t checkErrorKP2 = 0;

        uint32_t completeUBX = 0;
        uint32_t checkErrorUBX = 0;

        uint32_t completeNMEA = 0;
        uint32_t checkErrorNMEA = 0;

        uint32_t completeMAVLink = 0;
        uint32_t checkErrorMAVLink = 0;

        uint32_t completeProxy = 0;
        uint32_t notCompleteProxy = 0;

        uint32_t completeTotal = 0;
    } _counter;

    int32_t availContextPrivate() {
        return _contextLen;
    }

    void incContext() {
        _contextData++;
        _contextLen--;
    }

    void headerSync(uint8_t b) {
        if(b == 0xBB) { switchToKP1(); }
        else if(b == 0xCC) { switchToKP2(); }
        else if(b == 0xB5) { switchToUBX(); }
        else if(b == '$') { switchToNMEA(); }
        else if(b == 0xFD || b == 0xFE) { switchToMAVLink(); }
        else { _counter.passByte++; }
    }

    void headerReSync(uint8_t b) {
        _counter.frameReSync++;
        resetState();
        headerSync(b);
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
        _optionsLen = 0;
        _optionFlags.val = 0;
    }

    void switchToKP1() {
        _protoState = StateKP1Sync;
        _frameMaxLen = 255 + 8;
        resetFrame();
    }

    void switchToKP2() {
        _protoState = StateKP2Sync;
        _frameMaxLen = 256+128;
        resetFrame();
    }

    void switchToUBX() {
        _protoState = StateUBXSync;
        _frameMaxLen = 255 + 8;
        resetFrame();
    }

    void switchToNMEA() {
        _protoState = StateNmeaPayload;
        _frameMaxLen = 111;
        resetFrame();
    }

    void switchToMAVLink() {
        _protoState = StateMAVLinkPayload;
        _frameMaxLen = 288;
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
        bool res = checkFletcher(&_frame[2], _frameLen - 4, _frame[_frameLen - 2], _frame[_frameLen - 1]);
        _checksum = *(uint16_t*)(&_frame[_frameLen - 2]);
        if(res) {
            _readPosition = 6;
            _readMaxPosition = _frameLen - 2;
            _payloadLen = _readMaxPosition - _readPosition;

            _address =_frame[2];
            uint8_t mode = _frame[3];
            _type = (Type)(mode & 0x3);
            _ver = (Version)((mode >> 3) & 0x7);
            _mark = ((mode >> 6) & 0x1) == 0x1;
            _resp = ((mode >> 7) & 0x1) == 0x1;
            _id = (ID)(_frame[4]);

            _proto = ProtoKP1;
            _counter.completeKP1++;
            _counter.completeTotal++;
        } else {
            _counter.checkErrorKP1++;
            _proto = ProtoNone;
        }

        return res;
    }

    bool checkAsKP2() {
        bool res = checkFletcher(&_frame[2], _frameLen - 4, _frame[_frameLen - 2], _frame[_frameLen - 1]);
        _checksum = *(uint16_t*)(&_frame[_frameLen - 2]);
        if(res) {
            _readPosition = 4;

            _optionsLen = _frame[_readPosition];
            ++_readPosition;
            _readMaxPosition = _readPosition + _optionsLen + 4;
            _payloadLen = _optionsLen + 4;

            _optionFlags.val = 0;
            if(_optionsLen >= 3) {
                _optionFlags.val = read<U2>();

                if(_optionFlags.isAddress) {
                    _address = read<U1>();
                    _from = read<U1>();
                }

                if(_optionFlags.isStream) {
                    _stream.flags.val = read<U1>();
                    _stream.id = read<U2>();
                    _stream.offset = read<U4>();
                }

                if(_optionFlags.isLTime) {
                    _ltime = read<U4>();
                }

                if(_optionFlags.isGTime == 1) {
                    _gtime = read<U8>();
                } else if(_optionFlags.isGTime == 2) {
                    uint64_t unix_time = read<U4>();
                    uint32_t us = read<U4>();
                    _gtime = unix_time*1000 + us;
                } else if(_optionFlags.isGTime == 3){
                    read<U8>();
                }
            }

            if(_optionFlags.isProxy) {
//                _readPosition = _optionsLen + 4;
//                uint8_t* context_data = _contextData;
//                int32_t context_len = _contextLen;
                setProxyContext(&_frame[_optionsLen + 3], _frameLen - _readPosition - 1);

//                resetState();
//                resetFrame();
//                process();
//                if(isComplete()) {
//                    _counter.completeProxy++;
//                    _counter.completeTotal++;
//                } else {
//                    _counter.notCompleteProxy++;
//                }

//                setContext(context_data, context_len);
            }

            {
                _readPosition = _optionsLen + 4;

                uint8_t pld_flags = read<U1>();
                uint16_t id_ver = read<U2>();

                _type = (Type)(pld_flags & 0x3);
                _mark = ((pld_flags >> 3) & 0x1) == 0x1;
                _resp = ((pld_flags >> 2) & 0x1) == 0x1;

                _id = (ID)(id_ver >> 3);
                _ver = (Version)(id_ver & 0x7);

                _readMaxPosition = _frameLen - 2;
                _payloadLen = _readMaxPosition - _readPosition;

                _proto = ProtoKP2;
                _counter.completeKP2++;
                _counter.completeTotal++;
            }


        } else {
            _counter.checkErrorKP2++;
            _proto = ProtoNone;
        }

        return res;
    }

    bool checkAsUBX() {
        bool res = checkFletcher(&_frame[2], _frameLen - 4, _frame[_frameLen - 2], _frame[_frameLen - 1]);
        if(res) {
            _readPosition = 6;
            _readMaxPosition = _frameLen - 2;
            _payloadLen = _readMaxPosition - _readPosition;

            _proto = ProtoUBX;
            _counter.completeUBX++;
            _counter.completeTotal++;
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

        uint8_t frameCheck1 = hexToInt(_frame[_frameLen - 4]);
        uint8_t frameCheck2 = hexToInt(_frame[_frameLen - 3]);

        bool res = checkCalck == ((frameCheck1 << 4) + (frameCheck2));

        if(res) {
            uint16_t i = 5;
            const uint16_t max_id_len = 8;
            while(i <= max_id_len && _frame[i] != ',') {
                i++;
            }
            if(i > max_id_len) { res = false; }
            _readPosition = i + 1;
            _payloadLen = checkStopPos - _readPosition;
        }

        if(res) {
            _readMaxPosition = checkStopPos;
            _proto = ProtoNMEA;
            _counter.completeNMEA++;
            _counter.completeTotal++;
        } else {
            _counter.checkErrorNMEA++;
            _proto = ProtoNone;
        }

        return res;
    }

    bool checkMAVLink(uint8_t* buf, uint16_t len, uint8_t ch1, uint8_t ch2, uint32_t msgid) {
        uint8_t extra = getMAVLinkExtra(msgid);
        uint16_t crc = CRC16_MCRF4XX(buf, len, 0xFFFF);
        crc = CRC16_MCRF4XX(&extra, 1, crc);
        uint8_t crc1 = crc & 0xFF;
        uint8_t crc2 = crc  >> 8;
        return crc1 == ch1 && crc2 == ch2;
    }

    bool checkAsMAVLink() {
        uint32_t msgid = 0;
        if(_frame[0] == 0xFE) { // V1
            msgid = _frame[5];
        } else {  // V2
            msgid = (uint32_t(_frame[7])) | (uint32_t(_frame[8]) << 8) | (uint32_t(_frame[9]) << 16);
        }
        bool res = checkMAVLink(&_frame[1], _frameLen - 3, _frame[_frameLen - 2], _frame[_frameLen - 1], msgid);
        if(res) {
            if(_frame[0] == 0xFE) { // V1
                _readPosition = 6;
                _proto = ProtoMAVLink1;
            } else { // V2
                _readPosition = 10;
                _proto = ProtoMAVLink2;
            }

            _readMaxPosition = _frameLen - 2;
            _payloadLen = _readMaxPosition - _readPosition;

            _counter.completeMAVLink++;
            _counter.completeTotal++;
        } else {
            _counter.checkErrorMAVLink++;
            _proto = ProtoNone;
        }

        return res;
    }

    uint8_t hexToInt(uint8_t hex_char) {
         if (hex_char >= 'A' && hex_char <= 'F')
            return (hex_char + 10) - ('A');
          else if (hex_char >= 'a' && hex_char <= 'f')
            return (hex_char + 10) - ('a');
          else
            return hex_char - '0';
    }
};

//class FrameKogger : public FrameParser {
//public:
//    FrameKogger() {}

//    uint8_t route() const { return _frame[2]; }
//    uint8_t mode() const { return _frame[3]; }
//    ID id() const { return (ID)(_frame[4]); }
//    Type type() const { return (Type)(mode() & 0x3);}
//    Version ver() const { return (Version)((mode() >> 3) & 0x7);}
//    bool mark() const { return ((mode() >> 6) & 0x1) == 0x1;}
//    bool resp() const { return ((mode() >> 7) & 0x1) == 0x1;}
//};

//class ProtoKP1 : public FrameKogger {
//public:
//    ProtoKP1() {}

//    uint8_t route() const { return _frame[2]; }
//    uint8_t mode() const { return _frame[3]; }
//    ID id() const { return (ID)(_frame[4]); }
//    Type type() const { return (Type)(mode() & 0x3);}
//    Version ver() const { return (Version)((mode() >> 3) & 0x7);}
//    bool mark() const { return ((mode() >> 6) & 0x1) == 0x1;}
//    bool resp() const { return ((mode() >> 7) & 0x1) == 0x1;}

//protected:

//};

//class ProtoKP2 : public FrameKogger {
//public:
//    ProtoKP2() {}

//    uint8_t route() const { return _frame[2]; }
//    uint8_t mode() const { return _frame[_frame[4] + 4]; }
//    ID id() const { return (ID)(idver() >> 3); }
//    Type type() const { return (Type)(mode() & 0x3); }
//    Version ver() const { return (Version)(idver() & (uint16_t)0x3); }
//    bool mark() const { return ((mode() >> 3) & 0x1) == 0x1; }
//    bool resp() const { return ((mode() >> 2) & 0x1) == 0x1; }


//protected:
//    uint16_t idver() const { return *(uint16_t*)(&_frame[_frame[4] + 5]); }

//    void setRoute(uint8_t route) {_frame[2] = route;}
//    void setMode(uint8_t mode) { _frame[3] = mode; }
//    void setMode(Type type, Version ver, bool response) {
//        setMode((uint8_t)(((uint8_t)type & 0x3) | (((uint8_t)ver & 0x7) << 3) | (((uint8_t)response) << 7)));
//    }
//    void setId(ID id) { _frame[4] = id; }
//    void setLen(uint8_t len) { _frame[5] = len; _payloadLen = len; }
//};

class ProtoUBX : public FrameParser {
public:
    ProtoUBX() {}

    uint8_t msgId() const { return (ID)(_frame[3]); }
    uint8_t msgClass() const { return (ID)(_frame[2]); }

protected:
};

class ProtoMAVLink : public FrameParser {
public:
    ProtoMAVLink() {}

    uint8_t MAVLinkVersion() {
        if(_proto == ProtoMAVLink1) {
            return 1;
        } else {
            return 2;
        }
    }

    uint8_t sequenceNumber() const {
        if(_proto == ProtoMAVLink1) {
            return _frame[2];
        } else {
            return _frame[4];
        }
    }

    uint8_t systemID() const {
        if(_proto == ProtoMAVLink1) {
            return _frame[3];
        } else {
            return _frame[5];
        }
    }

    uint8_t componentID() const {
        if(_proto == ProtoMAVLink1) {
            return _frame[4];
        } else {
            return _frame[6];
        }
    }

    uint32_t msgId() const {
        if(_proto == ProtoMAVLink1) {
            return _frame[5];
        } else {
            return (uint32_t(_frame[7])) | (uint32_t(_frame[8]) << 8) | (uint32_t(_frame[9]) << 16);
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
        while(i < _readMaxPosition && _frame[_readPosition] != ',') {
            _readPosition++;
            i++;
        }

        _readPosition++;
    }

    char readChar() {
        char c = _frame[_readPosition++];
        _readPosition++;
        return c;
    }

    double readDouble() {
        uint32_t i = 0;
        char data[20] = {};
        while(i < sizeof(data) && _frame[_readPosition] != ',') {
            data[i] = _frame[_readPosition];
            _readPosition++;
            i++;
        }

        double res = NAN;
        if(i > 0) {
            sscanf(data, "%lf", &res);
        }

        _readPosition++;
        return res;
    }

    double readLatitude() {
        int16_t deg_h = _frame[_readPosition++] - '0';
        int16_t deg_l = _frame[_readPosition++] - '0';

        int16_t min_h = _frame[_readPosition++] - '0';
        int16_t min_l = _frame[_readPosition++] - '0';

        if(_frame[_readPosition++] != '.') { return std::nan(""); }

        int16_t i = 0;
        int32_t min_part = 0;
        int32_t exp_sum = 1;
        const uint16_t max_part_len = 10;
        while(i <= max_part_len && _frame[_readPosition] != ',') {
            min_part *= 10;
            min_part += _frame[_readPosition++] - '0';
            i++;
            exp_sum *= 10;
        }

        if(i > max_part_len) { return std::nan(""); }

        _readPosition++;

        int16_t q = _frame[_readPosition++];
        _readPosition++;

        if(q != 'N' && q != 'S') { return std::nan(""); }

        double lat = (double)(deg_h*10 + deg_l) + (double)(min_h*10 + min_l)/60.0 + ((double)min_part)/(60.0*exp_sum);
        if(q == 'S') { lat = -lat; }
        return lat;
    }

    double readLongitude() {
        int16_t deg_h = _frame[_readPosition++] - '0';
        int16_t deg_m = _frame[_readPosition++] - '0';
        int16_t deg_l = _frame[_readPosition++] - '0';

        int16_t min_h = _frame[_readPosition++] - '0';
        int16_t min_l = _frame[_readPosition++] - '0';

        if(_frame[_readPosition++] != '.') { return std::nan(""); }

        int16_t i = 0;
        int32_t min_part = 0;
        int32_t exp_sum = 1;
        const uint16_t max_part_len = 10;
        while(i <= max_part_len && _frame[_readPosition] != ',') {
            min_part *= 10;
            min_part += _frame[_readPosition++] - '0';
            i++;
            exp_sum *= 10;
        }

        if(i > max_part_len) { return std::nan(""); }

        _readPosition++;

        int16_t q = _frame[_readPosition++];
        _readPosition++;

        if(q != 'W' && q != 'E') { return std::nan(""); }

        double lon = (double)(deg_h*100 + deg_m*10 + deg_l) + (double)(min_h*10 + min_l)/60.0 + ((double)min_part)/(60.0*exp_sum);
        if(q == 'W') { lon = -lon; }
        return lon;
    }

    bool readTime(uint8_t* hh, uint8_t* mm, uint8_t* ss, uint16_t* mss) {
        if(_frame[_readPosition] == ',') {
            _readPosition++;
            return false;
        }
        int16_t h_h = _frame[_readPosition++] - '0';
        int16_t h_l = _frame[_readPosition++] - '0';

        int16_t m_h = _frame[_readPosition++] - '0';
        int16_t m_l = _frame[_readPosition++] - '0';

        int16_t s_h = _frame[_readPosition++] - '0';
        int16_t s_l = _frame[_readPosition++] - '0';

        if(_frame[_readPosition++] != '.') { return false; }

        int16_t subs_h = _frame[_readPosition++] - '0';
        int16_t subs_l = _frame[_readPosition++] - '0';

        _readPosition++;

        *hh = h_h*10 + h_l;
        *mm = m_h*10 + m_l;
        *ss = s_h*10 + s_l;
        *mss = subs_h*100 + subs_l*10;

        return true;
    }

     bool readDate(uint16_t* year, uint8_t* mounth, uint8_t* day) {
         if(_frame[_readPosition] == ',') {
             _readPosition++;
             return false;
         }

         int16_t d_h = _frame[_readPosition++] - '0';
         int16_t d_l = _frame[_readPosition++] - '0';

         int16_t m_h = _frame[_readPosition++] - '0';
         int16_t m_l = _frame[_readPosition++] - '0';

         int16_t y_h = _frame[_readPosition++] - '0';
         int16_t y_l = _frame[_readPosition++] - '0';

         *year = (y_h*10 + y_l) + 2000;
         *mounth = m_h*10 + m_l;
         *day = d_h*10 + d_l;

         return true;
     }

protected:
};


class ProtoBinOut : public FrameParser
{
public:
    explicit ProtoBinOut() { }

    void create(Type type, Version ver, ID id, uint8_t route) {
        _frame[0] = 0xbb;
        _frame[1] = 0x55;
        setRoute(route);
        setMode(type, ver, true);
        setId(id);

        _frameLen = 6;
        _frameMaxLen = 254;
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

        _checksum = *(uint16_t*)(&_frame[_frameLen - 2]);
        _proto = ProtoKP1;
    }
protected:
    void setRoute(uint8_t route) { _address = route; _frame[2] = route;}
    void setMode(uint8_t mode) { _frame[3] = mode; }
    void setMode(Type type, Version ver, bool response) {
        _type = type;
        _ver =  ver;
        _resp = response;
        setMode((uint8_t)(((uint8_t)type & 0x3) | (((uint8_t)ver & 0x7) << 3) | (((uint8_t)response) << 7)));
    }
    void setId(ID id) { _id = id; _frame[4] = id; }
    void setLen(uint8_t len) { _frame[5] = len; _payloadLen = len; }
};

}

#endif // KOGGERBINNARYPARSER_H
