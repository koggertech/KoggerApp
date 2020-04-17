#include "ProtoBinnary.h"
#include "QDataStream"
#include "QVariant"

using namespace KoggerBinnaryProtocol;


ProtOut::ProtOut(QObject *parent)  : ProtoBase(parent)
{

}

ProtIn::ProtIn(QObject *parent)  : ProtoBase(parent)
{

}

Resp ProtIn::putByte(uint8_t b) {
    switch (State) {
    case StateRoute:
    case StateMode:
    case StateID:
    case StateLength:
    case StatePayload:
        checkUpdate(b, _calcCheck1, _calckCheck2);
        break;
    default:
        break;
    }

    Resp resp = respNone;
    switch (State) {
    case StateSync1:
        if (b == SYNC1) {
            State = StateSync2;
        }
        break;

    case StateSync2:
        if (b == SYNC2) {
            State = StateRoute;
        } else if (b == SYNC1) {
            State = StateSync2;
        } else {
            State = StateSync1;
        }
        break;

    case StateRoute:
        _fieldRoute = b;
        State = StateMode;
        break;

    case StateMode:
        _mode = b;
        State = StateID;
        break;

    case StateID:
        _id = (ID)(b);
        State = StateLength;
        break;

    case StateLength:
        _fieldLen = b;
        if (_fieldLen > 0) {
            _fillPosition = 0;
            State = StatePayload;
        } else {
            State = StateCheck1;
        }
        break;

    case StatePayload:
        fillData(b);
        if (_fillPosition >= _fieldLen) {
            State = StateCheck1;
        }
        break;

    case StateCheck1:
        State = StateCheck2;
        _fieldCheck1 = b;
        break;

    case StateCheck2:
        _fieldCheck2 = b;
        if (isCheckValid()) {
            resetState();
            resp = respOk;
        } else {
            resetStateAsError();
            resp = respErrorCheck;
        }
        break;
    }

    return resp;
}
