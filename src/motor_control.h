#pragma once

#include <QObject>
#include <QUuid>
#include <QTimer>
#include <QQueue>
#include <QStringList>

#include "Link.h"


typedef enum
{
    MKS_OK      = 0x00U,
    MKS_ADDR    = 0x01U,
    MKS_CRC     = 0x02U,
    MKS_FAIL    = 0x03U,
    MKS_ADDR_ANS= 0x04U
} MKS_Status;

enum class ResponseType
{
    kUndefined = 0,
    kSetEn,
    kRunSteps,
    kPosition,
    kMotorPosition,
    kGoZero
};

struct ResponseStruct
{
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

struct QueueItem
{
    QueueItem() : fAngle_(0.0f), sAngle_(0.0f), pause_(0.0f) {};
    QueueItem(float fAngle, float sAngle, float pause) : fAngle_(fAngle), sAngle_(sAngle), pause_(pause) {};

    bool isUndefined() const {
        if (qFuzzyCompare(1.0f, 1.0f + pause_))
            return true;
        return false;
    }

    float fAngle_;
    float sAngle_;
    float pause_;
};

class MotorControl : public QObject
{
    Q_OBJECT

public:
    /* methods */
    MotorControl(QObject* parent = nullptr, Link* linkPtr = nullptr);
    ~MotorControl();

    void addTask(QStringList tasks);
    void clearTasks();

    ///////////////////////////
    // servo
    MKS_Status setEn(uint8_t addr, uint8_t data);
    MKS_Status setEnResponseCheck(const QByteArray& data);
    MKS_Status runSteps(uint8_t addr, uint8_t speed, float angle, bool force = false ,  bool useMovementTimer = true);
    MKS_Status runStepsResponseCheck(const QByteArray& data);
    MKS_Status goZero(uint8_t addr, bool force = false);
    MKS_Status goZeroResponseCheck(const QByteArray& data);
    ///////////////////////////

    static QString convertStatus(MKS_Status status);

    uint8_t getFAddr() const;
    uint8_t getSAddr() const;

private slots:
    MKS_Status readData(QByteArray data);
    void onTimerEnd();
    void onMovementTimerEnd();
    void onWaitingTimerEnd();

signals:
    void commandSended(const QByteArray& data);
    void angleChanged(uint8_t addr, float angle);
    void posIsConstant(float currFAngle, float taskFAngle, float currSAngle, float taskSAngle);

private:
    ///////////////////////////
    // servo
    MKS_Status position(uint8_t addr, int32_t* value, float* angle);
    MKS_Status positionResponseCheck(const QByteArray& data);
    MKS_Status motorPosition(uint8_t addr, int32_t* value, float* angle);
    MKS_Status motorPositionResponseCheck(const QByteArray& data);

    uint8_t calculateCrc(uint8_t* data, uint8_t length);
    void uartSend(uint8_t* data, uint8_t length);
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
    struct struct_encAnswerMotorPos
    {
        uint8_t addr;
        uint8_t carry[4];
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

    float PosToAngle(int32_t pos) const;
    int32_t AngleToPos(float angle) const;
    bool CheckPos(uint8_t addr, int32_t pos) const;

    const int timerInterval_ = 200;
    const float step_ = 1.8;
    const int numMSteps_ = 16;
    const float circDeg_ = 360.0f;
    const float fullRotSec_ = 7.0f;
    const float confirmDelta_ = 1.0f;
    const float reducCoeff = 1.0f / 3.0f;

    Link* linkPtr_;
    ResponseStruct respStruct_;
    QByteArray buffer_;
    QQueue<QueueItem> taskQueue_;
    QTimer elapsedTimer_;
    QTimer movementTimer_;
    QTimer waitingTimer_;
    float taskFAngle_ = 0.0f;
    float taskSAngle_ = 0.0f;
    float lastTaskPause_ = 0.0f;
    int32_t fPos_ = 0;
    int32_t sPos_ = 0;
    uint8_t fAddr_ = 0x01;
    uint8_t sAddr_ = 0x02;
    bool isFirst_ = false;  
    bool isMovementTimer_ = false;
    bool fStartMark_ = true;
    bool sStartMark_ = true;
    bool isConstantFPos_ = false;
    bool isConstantSPos_ = false;
    bool wasTask_ = false;
};
