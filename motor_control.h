#pragma once

#include <QObject>
#include <QUuid>
#include "Link.h"

typedef enum
{
    MKS_OK      = 0x00U,
    MKS_ADDR    = 0x01U,
    MKS_CRC     = 0x02U,
    MKS_FAIL    = 0x03U,
    MKS_ADDR_ANS= 0x04U
} MKS_Status;

enum class ResponseType {
    kUndefined = 0,
    kSetEn,
    kRunSteps,
    kPosition,
    kGoZero
};

struct ResponseStruct {
    ResponseStruct() : address_(0), type_(ResponseType::kUndefined) {};
    ResponseStruct(uint8_t address, ResponseType type) : address_(address), type_(type) {};

    bool isUndefined() const {
        if (address_ == 0 && type_ == ResponseType::kUndefined)
            return true;
        return false;
    }

    uint8_t address_;
    ResponseType type_;
};

class MotorControl : public QObject
{
    Q_OBJECT

public:
    /* methods */
    MotorControl(QObject* parent = nullptr, Link* linkPtr = nullptr);
    ~MotorControl();

    ///////////////////////////
    // servo
    MKS_Status setEn(uint8_t addr, uint8_t data);
    MKS_Status setEnResponseCheck(const QByteArray& data);

    MKS_Status runSteps(uint8_t addr, uint8_t speed, int32_t steps);
    MKS_Status runStepsResponseCheck(const QByteArray& data);

    MKS_Status position(uint8_t addr, int32_t* value, float* angle);
    MKS_Status positionResponseCheck(const QByteArray& data);

    MKS_Status goZero(uint8_t addr);
    MKS_Status goZeroResponseCheck(const QByteArray& data);
    ///////////////////////////

    static QString convertStatus(MKS_Status status);


private slots:
    void readData(QByteArray data);

signals:
    void commandSended(const QByteArray& data);

private:


    ///////////////////////////
    // servo
    uint8_t calculateCrc(uint8_t* data, uint8_t length);
    void uartSend(uint8_t* data, uint8_t length);
    //void uartRead(uint8_t* data, uint8_t length);


    void resetResponseStruct();

    struct struct_3byte
    {
        uint8_t addr;
        uint8_t byte;
        uint8_t crc;
    };

    struct struct_4byte
    {
        uint8_t addr;
        uint8_t func;
        uint8_t byte;
        uint8_t crc;
    };

    struct struct_encAnswer
    {
        uint8_t addr;
        uint8_t carry[4];
        uint8_t value[2];
        uint8_t crc;
    };

    struct struct_runSteps
    {
        uint8_t addr;
        uint8_t function;
        uint8_t dir_speed;
        uint8_t pulses[4];
        uint8_t crc;
    };
    ///////////////////////////


    Link* linkPtr_;
    ResponseStruct respStruct_;
    QByteArray buffer_;


};
