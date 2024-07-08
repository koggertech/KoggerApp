#include "motor_control.h"


MotorControl::MotorControl(QObject* parent, Link* linkPtr) :
    QObject(parent),
    linkPtr_(linkPtr)
{
    auto connType{ Qt::AutoConnection };
    QObject::connect(this, &MotorControl::commandSended, linkPtr_, &Link::write, connType);
    QObject::connect(linkPtr_, &Link::dataReady, this, &MotorControl::readData, connType);
}

MotorControl::~MotorControl()
{
    if (linkPtr_) {
        QObject::disconnect(this, &MotorControl::commandSended, linkPtr_, &Link::write);
        QObject::disconnect(linkPtr_, &Link::dataReady, this, &MotorControl::readData);
    }
}

///////////////////////////
// servo
void MotorControl::uartSend(uint8_t* data, uint8_t length)
{
    QByteArray byteArray(reinterpret_cast<char*>(data), length);

    emit commandSended(byteArray);
}

void MotorControl::resetResponseStruct()
{
    respStruct_.address_ = 0;
    respStruct_.type_ = ResponseType::kUndefined;
}

//void MotorControl::uartRead(uint8_t* data, uint8_t length)
//{
//    Q_UNUSED(data);
//    Q_UNUSED(length);
//}

void MotorControl::readData(QByteArray data)
{
    qDebug() << "MotorControl::readData: " << data;
    qDebug() << "data.size(): " << data.size();
    qDebug() << "data: " << data;

    MKS_Status resCheck = MKS_Status::MKS_FAIL;

    switch (respStruct_.type_) {
    case ResponseType::kUndefined: { break; }
    case ResponseType::kGoZero: { resCheck = goZeroResponseCheck(data); break; }
    case ResponseType::kPosition: { resCheck = positionResponseCheck(data); break; }
    case ResponseType::kRunSteps: {
        // TODO
        if (buffer_.size() == 6)
            buffer_.clear();
        buffer_.append(data);
        if (buffer_.size() == 6) {
            resCheck = runStepsResponseCheck(buffer_);
        }

        break;
    }
    case ResponseType::kSetEn: { resCheck = setEnResponseCheck(data); break; }
    default:
        break;
    }

    qDebug() << "resCheck: " << convertStatus(resCheck);
    qDebug() << "MotorControl::readData: end";
}

uint8_t MotorControl::calculateCrc(uint8_t* data, uint8_t length)
{
    uint32_t crc_math = 0;

    for (uint8_t i = 0; i < length; ++i) {
        crc_math += data[i];
    }

    return (uint8_t)(crc_math & 0xFF);
}






MKS_Status MotorControl::setEn(uint8_t addr, uint8_t data)
{
    qDebug() << "MotorControl::setEn start";

    if (!respStruct_.isUndefined()) {
        return MKS_FAIL;
    }

    if (addr > 9) {
        return MKS_ADDR;
    }

    respStruct_ = { addr, ResponseType::kSetEn };

    data &= 1;
    uint8_t msg[4] = {static_cast<uint8_t>(0xE0 + addr), 0xF3, data, static_cast<uint8_t>(0xd4 + data + addr)};
    uartSend(msg, 4);

    qDebug() << "MotorControl::setEn succesfully end";

    return MKS_OK;
}

MKS_Status MotorControl::setEnResponseCheck(const QByteArray& data)
{
    qDebug() << "MotorControl::setEnResponseCheck start";

    MKS_Status retVal = MKS_Status::MKS_OK;

    if (respStruct_.type_ != ResponseType::kSetEn || data.size() != sizeof(struct_3byte)) {
        retVal = MKS_FAIL;
    }
    else {
        struct_3byte answer;
        memcpy(&answer, data.data(), sizeof(struct_3byte));

        uint8_t inCRC = calculateCrc((uint8_t*)&answer, 2);

        if (inCRC != answer.crc) {
            retVal = MKS_CRC;
        }
        else if (answer.addr != (0xE0 + respStruct_.address_)) {
            retVal =  MKS_ADDR_ANS;
        }
        else if (answer.byte == 0) {
            retVal = MKS_FAIL;
        }
        else {
            retVal =  MKS_OK;
        }
    }

    resetResponseStruct();
    qDebug() << "respStruct_: " << convertStatus(retVal);
    qDebug() << "MotorControl::setEnResponseCheck end";

    return retVal;
}









MKS_Status MotorControl::runSteps(uint8_t addr, uint8_t speed, int32_t steps)
{
    qDebug() << "MotorControl::runSteps start";

    if (!respStruct_.isUndefined()) {
        return MKS_FAIL;
    }

    if (addr > 9) {
        return MKS_ADDR;
    }

    respStruct_ = { addr, ResponseType::kRunSteps };

    struct_runSteps request;
    request.addr = 0xE0 + addr;
    request.function = 0xFD;
    request.dir_speed = speed & 0x7F;
    uint32_t steps_modul = 0;

    if (steps > 0) {
        request.dir_speed |= 0x80;
        steps_modul = steps;
    }
    else {
        steps_modul = (uint32_t)(steps * -1);
    }

    for (int8_t s = 3; s >= 0; s--) {
        request.pulses[s] = steps_modul & 0x000000FF;
        steps_modul >>= 8;
    }

    request.crc = calculateCrc((uint8_t*)&request, 7);
    uartSend((uint8_t*)&request, 8);

    qDebug() << "MotorControl::runSteps succesfully end";

    return MKS_OK;
}

MKS_Status MotorControl::runStepsResponseCheck(const QByteArray& data)
{
    qDebug() << "MotorControl::runStepsResponseCheck start";

    MKS_Status retVal = MKS_Status::MKS_OK;

    const int size = 2;
    //qDebug() << "data.size(): " << data.size();

    if (respStruct_.type_ != ResponseType::kRunSteps || data.size() != sizeof(struct_3byte) * size) {
        retVal = MKS_FAIL;
        //qDebug() << "eeeeh 1";
    }
    else {
        struct_3byte answer[size];
        memcpy(&answer, data.data(), sizeof(struct_3byte) * size);

        for (uint8_t m = 0; m < 2; ++m) {
            uint8_t inCRC = calculateCrc((uint8_t*)&answer[m], 2);

            if (inCRC != answer[m].crc) {
                retVal = MKS_CRC;
            }
            else if (answer[m].addr != (0xE0 + respStruct_.address_)) {
                retVal = MKS_ADDR_ANS;
            }
            else if (answer[m].byte == 0) {
                retVal = MKS_FAIL;
            }
        }
    }

    resetResponseStruct();
    qDebug() << "respStruct_: " << convertStatus(retVal);
    qDebug() << "MotorControl::runStepsResponseCheck end";

    return retVal;
}








MKS_Status MotorControl::position(uint8_t addr, int32_t* value, float* angle)
{
    Q_UNUSED(value);
    Q_UNUSED(angle);

    qDebug() << "MotorControl::position start";

    if (!respStruct_.isUndefined()) {
        return MKS_FAIL;
    }

    if (addr > 9) {
        return MKS_ADDR;
    }

    respStruct_ = { addr, ResponseType::kPosition };

    uint8_t msg[3] = {static_cast<uint8_t>(0xE0 + addr), 0x30, static_cast<uint8_t>(0x10 + addr)};
    uartSend(msg, 3);

    qDebug() << "MotorControl::position succesfully end";

    return MKS_OK;
}

MKS_Status MotorControl::positionResponseCheck(const QByteArray& data)
{
    qDebug() << "MotorControl::positionResponseCheck start";

    MKS_Status retVal = MKS_Status::MKS_OK;

    if (respStruct_.type_ != ResponseType::kPosition || data.size() != sizeof(struct_encAnswer)) {
        retVal = MKS_FAIL;
    }
    else {
        struct_encAnswer answer;
        memcpy(&answer, data.data(), sizeof(struct_encAnswer));

        uint8_t inCRC = calculateCrc((uint8_t*)&answer, 7);

        if (inCRC != answer.crc) {
            retVal = MKS_CRC;
        }
        else if (answer.addr != (0xE0 + respStruct_.address_)) {
            retVal = MKS_ADDR_ANS;
        }

        int32_t carry = 0;
        uint16_t mks_value = 0;

        for (int8_t s = 0; s < 4; s++) {
            carry <<= 8;
            carry += answer.carry[s];
        }

        mks_value = answer.value[0];
        mks_value <<= 8;
        mks_value += answer.value[1];

        int32_t value = carry * 0xFFFF + mks_value;
        float angle = (((float)value) / (65535.0f / 400.0f)) * 0.9f;

        qDebug() << "value: " << value << ", angle: " << angle;

    }

    resetResponseStruct();
    qDebug() << "respStruct_: " << convertStatus(retVal);
    qDebug() << "MotorControl::positionResponseCheck end";

    return retVal;
}






MKS_Status MotorControl::goZero(uint8_t addr)
{
    qDebug() << "MotorControl::goZero start";

    if (!respStruct_.isUndefined()) {
        return MKS_FAIL;
    }

    if (addr > 9) {
        return MKS_ADDR;
    }

    respStruct_ = { addr, ResponseType::kGoZero };

    uint8_t msg[4] = {static_cast<uint8_t>(0xE0 + addr), 0x94, 0x00, static_cast<uint8_t>(0x74 + addr)};
    uartSend(msg, 4);

    qDebug() << "MotorControl::goZero succesfully end";

    return MKS_OK;
}

MKS_Status MotorControl::goZeroResponseCheck(const QByteArray& data)
{
    qDebug() << "MotorControl::goZeroResponseCheck start";

    MKS_Status retVal = MKS_Status::MKS_OK;

    if (respStruct_.type_ != ResponseType::kGoZero || data.size() != sizeof(struct_3byte)) {
        retVal = MKS_FAIL;
    }
    else {
        struct_3byte answer;
        memcpy(&answer, data.data(), sizeof(struct_3byte));
        uint8_t inCRC = calculateCrc((uint8_t*)&answer, 2);

        if (inCRC != answer.crc) {
            retVal = MKS_CRC;
        }
        else if (answer.addr != (0xE0 + respStruct_.address_)) {
            retVal = MKS_ADDR_ANS;
        }
    }

    resetResponseStruct();
    qDebug() << "respStruct_: " << convertStatus(retVal);
    qDebug() << "MotorControl::goZeroResponseCheck end";

    return retVal;
}





QString MotorControl::convertStatus(MKS_Status status)
{
    QString retVal;

    switch (status) {
    case MKS_Status::MKS_ADDR: {
        retVal = "MKS_ADDR";
        break;
    }
    case MKS_Status::MKS_ADDR_ANS: {
        retVal = "MKS_ADDR_ANS";
        break;
    }
    case MKS_Status::MKS_CRC: {
        retVal = "MKS_CRC";
        break;
    }
    case MKS_Status::MKS_FAIL: {
        retVal = "MKS_FAIL";
        break;
    }
    case MKS_Status::MKS_OK: {
        retVal = "MKS_OK";
        break;
    }
    default:
        break;
    }

    return retVal;
}




///////////////////////////
