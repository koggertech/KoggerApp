#include "motor_control.h"


MotorControl::MotorControl(QObject* parent, Link* linkPtr) :
    QObject(parent),
    linkPtr_(linkPtr)
{

}

MotorControl::~MotorControl()
{

}

///////////////////////////
// servo
void MotorControl::uartSend(uint8_t* data, uint8_t length)
{
    Q_UNUSED(data);
    Q_UNUSED(length);
}

void MotorControl::uartRead(uint8_t* data, uint8_t length)
{
    Q_UNUSED(data);
    Q_UNUSED(length);
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
    if (addr > 9) {
        return MKS_ADDR;
    }

    data &= 1;
    uint8_t msg[4] = {static_cast<uint8_t>(0xE0 + addr), 0xF3, data, static_cast<uint8_t>(0xd4 + data + addr)};
    uartSend(msg, 4);

    struct_3byte answer;
    uartRead((uint8_t*)&answer, 3);
    uint8_t inCRC = calculateCrc((uint8_t*)&answer, 2);

    if (inCRC != answer.crc) {
        return MKS_CRC;
    }
    else if (answer.addr != (0xE0 + addr)) {
        return MKS_ADDR_ANS;
    }
    else if (answer.byte == 0) {
        return MKS_FAIL;
    }

    return MKS_OK;
}

MKS_Status MotorControl::runSteps(uint8_t addr, uint8_t speed, int32_t steps)
{
    if (addr > 9) {
        return MKS_ADDR;
    }

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

    struct_3byte answer[2];
    uartRead((uint8_t*)&answer[0], 6);

    for (uint8_t m = 0; m < 2; m++) {
        uint8_t inCRC = calculateCrc((uint8_t*)&answer[m], 2);

        if (inCRC != answer[m].crc) {
            return MKS_CRC;
        }
        else if (answer[m].addr != (0xE0 + addr)) {
            return MKS_ADDR_ANS;
        }
        else if (answer[m].byte == 0) {
            return MKS_FAIL;
        }
    }

    return MKS_OK;
}

MKS_Status MotorControl::position(uint8_t addr, int32_t* value, float* angle)
{
    if (addr > 9) {
        return MKS_ADDR;
    }

    uint8_t msg[3] = {static_cast<uint8_t>(0xE0 + addr), 0x30, static_cast<uint8_t>(0x10 + addr)};
    uartSend(msg, 3);

    struct_encAnswer answer;
    uartRead((uint8_t*)&answer, sizeof(answer));
    uint8_t inCRC = calculateCrc((uint8_t*)&answer, 7);

    if (inCRC != answer.crc) {
        return MKS_CRC;
    }
    else if (answer.addr != (0xE0 + addr)) {
        return MKS_ADDR_ANS;
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

    *value = carry * 0xFFFF + mks_value;

    *angle = (((float)*value) / (65535.0f / 400.0f)) * 0.9f;

    return MKS_OK;
}

MKS_Status MotorControl::goZero(uint8_t addr)
{
    if (addr > 9) {
        return MKS_ADDR;
    }

    uint8_t msg[4] = {static_cast<uint8_t>(0xE0 + addr), 0x94, 0x00, static_cast<uint8_t>(0x74 + addr)};
    uartSend(msg, 4);

    struct_3byte answer;
    uartRead((uint8_t*)&answer, 3);
    uint8_t inCRC = calculateCrc((uint8_t*)&answer, 2);

    if (inCRC != answer.crc) {
        return MKS_CRC;
    }
    else if (answer.addr != (0xE0 + addr)) {
        return MKS_ADDR_ANS;
    }

    return MKS_OK;
}
///////////////////////////
