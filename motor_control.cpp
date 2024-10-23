#include "motor_control.h"


MotorControl::MotorControl(QObject* parent, Link* linkPtr) :
    QObject(parent),
    linkPtr_(linkPtr)
{
    auto connType{ Qt::AutoConnection };
    QObject::connect(this, &MotorControl::commandSended, linkPtr_, &Link::write, connType);
    QObject::connect(linkPtr_, &Link::dataReady, this, &MotorControl::readData, connType);

    elapsedTimer_.setInterval(timerInterval_);
    QObject::connect(&elapsedTimer_, &QTimer::timeout, this, &MotorControl::onTimerEnd);
    elapsedTimer_.start();

    QObject::connect(&movementTimer_, &QTimer::timeout, this, &MotorControl::onMovementTimerEnd);
    QObject::connect(&waitingTimer_, &QTimer::timeout, this, &MotorControl::onWaitingTimerEnd);
}

MotorControl::~MotorControl()
{
    if (linkPtr_) {
        QObject::disconnect(this, &MotorControl::commandSended, linkPtr_, &Link::write);
        QObject::disconnect(linkPtr_, &Link::dataReady, this, &MotorControl::readData);
    }
}

void MotorControl::addTask(QStringList tasks)
{
    for (auto &itm : tasks) {
        QStringList numbers = itm.split(QLatin1Char(','));

        if (numbers.count() != 3)
            continue;

        taskQueue_.append(QueueItem(numbers.at(0).toFloat(), numbers.at(1).toFloat(), numbers.at(2).toFloat()));
    }
}

void MotorControl::clearTasks()
{
    taskQueue_.clear();
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

float MotorControl::PosToAngle(int32_t carry) const
{
    return static_cast<float>(carry) / 65536.0f * 360.0f;
}

int32_t MotorControl::AngleToPos(float carry) const
{
    return carry * 65536.0f / 360.0f;
}

bool MotorControl::CheckPos(uint8_t addr, int32_t pos) const
{
    auto deg = PosToAngle(pos);

    if (deg < 181.0f && deg > -181.0f && addr == fAddr_)
        return true;
    if (deg < 181.0f && deg > -181.0f && addr == sAddr_)
        return true;
    else
        return false;
}

MKS_Status MotorControl::readData(QByteArray data)
{
    MKS_Status resCheck = MKS_Status::MKS_FAIL;

    switch (respStruct_.type_) {
    case ResponseType::kGoZero: {
        auto requiredSize = static_cast<int>(sizeof(struct_3byte));
        if (buffer_.size() >= requiredSize) {
            buffer_.clear();
        }
        buffer_.append(data);
        if (buffer_.size() == requiredSize) {
            resCheck = goZeroResponseCheck(data);
            buffer_.clear();
        }
        break;
    }
    case ResponseType::kPosition: {
        auto requiredSize = static_cast<int>(sizeof(struct_encAnswer));
        if (buffer_.size() >= requiredSize) {
            buffer_.clear();
        }
        buffer_.append(data);
        if (buffer_.size() == requiredSize) {
            resCheck = positionResponseCheck(data);
            buffer_.clear();
        }
        break;
    }
    case ResponseType::kMotorPosition: {
        auto requiredSize = static_cast<int>(sizeof(struct_encAnswerMotorPos));
        if (buffer_.size() >= requiredSize) {
            buffer_.clear();
        }
        buffer_.append(data);
        if (buffer_.size() == requiredSize) {
            resCheck = motorPositionResponseCheck(data);
            buffer_.clear();
        }
        break;
    }
    case ResponseType::kRunSteps: {
        auto requiredSize = static_cast<int>(sizeof(struct_3byte) * 2);
        if (buffer_.size() >= requiredSize) {
            buffer_.clear();
        }
        buffer_.append(data);
        if (buffer_.size() == requiredSize) {
            resCheck = runStepsResponseCheck(buffer_);
            buffer_.clear();
        }
        break;
    }
    case ResponseType::kSetEn: {
        auto requiredSize = static_cast<int>(sizeof(struct_3byte));
        if (buffer_.size() >= requiredSize) {
            buffer_.clear();
        }
        buffer_.append(data);
        if (buffer_.size() == requiredSize) {
            resCheck = setEnResponseCheck(data);
            buffer_.clear();
        }
        break;
    }
    case ResponseType::kUndefined:
        break;
    default:
        break;
    }

    return resCheck;
}

void MotorControl::onTimerEnd()
{
    if (!taskQueue_.isEmpty() && !isMovementTimer_ && (isConstantFPos_ && isConstantSPos_) && !waitingTimer_.isActive()) {
        auto task = taskQueue_.dequeue();

        // calc max pause time
        int32_t deltaFPos =  AngleToPos(task.fAngle_) - fPos_;
        int32_t deltaSPos =  AngleToPos(task.sAngle_ / reducCoeff) - sPos_;
        float deltaFAngle =  PosToAngle(deltaFPos);
        float deltaSAngle =  PosToAngle(deltaSPos);
        float needFSecs = (std::fabs(deltaFAngle) / circDeg_) * fullRotSec_;
        float needSSecs = (std::fabs(deltaSAngle) / circDeg_) * fullRotSec_;
        float maxSecVal = std::max(std::max(needFSecs, needSSecs), 1.0f);

        lastTaskPause_ = task.pause_;

        isMovementTimer_ = true;
        movementTimer_.setInterval(maxSecVal * 1000.f);
        movementTimer_.start();

        wasTask_ = true;

        qDebug() << "start task";

        runSteps(fAddr_, 1, task.fAngle_, true, false);
        runSteps(sAddr_, 1, task.sAngle_, true, false);
    }
    else if (!isMovementTimer_) {
        //qDebug() << "checking pos!!!!, waiting timer: " << waitingTimer_.isActive();
        if (isFirst_) {
           motorPosition(fAddr_,nullptr,nullptr);
        }
        else {
           motorPosition(sAddr_,nullptr,nullptr);
        }
    }

    isFirst_ = !isFirst_;
    elapsedTimer_.start();
}

void MotorControl::onMovementTimerEnd()
{
    movementTimer_.stop();
    isMovementTimer_ = false;

    qDebug() << "movement finished";

    if (wasTask_) {
        wasTask_= false;

        qDebug() << "start waiting: " << lastTaskPause_;

        waitingTimer_.setInterval(lastTaskPause_ * 1000.0f);
        waitingTimer_.start();

        motorPosition(fAddr_, nullptr, nullptr);
        motorPosition(sAddr_, nullptr, nullptr);

        lastTaskPause_ = 0.0f;
    }
}

void MotorControl::onWaitingTimerEnd()
{
    waitingTimer_.stop();
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

    return MKS_OK;
}

MKS_Status MotorControl::setEnResponseCheck(const QByteArray& data)
{
    MKS_Status retVal = MKS_Status::MKS_OK;

    if (respStruct_.type_ != ResponseType::kSetEn) {
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

    return retVal;
}

MKS_Status MotorControl::runSteps(uint8_t addr, uint8_t speed, float angle, bool force, bool useMovementTimer)
{
    if (!CheckPos(addr, angle)) {
        return MKS_Status::MKS_FAIL;        
    }

    float needSecs = 0.0f;
    int32_t deltaPos = addr == fAddr_ ? AngleToPos(angle) - fPos_ : AngleToPos(angle / reducCoeff) - sPos_;
    float deltaAngle =  PosToAngle(deltaPos);

    if (useMovementTimer) {
        needSecs = std::max(1.0f, (std::fabs(deltaAngle) / circDeg_) * fullRotSec_);
        isMovementTimer_ = true;
        movementTimer_.setInterval(needSecs * 1000.f);
        movementTimer_.start();
    }

    int32_t steps = static_cast<int32_t>(std::round(deltaAngle / (step_ / static_cast<float>(numMSteps_))));

    isConstantFPos_ = false;
    isConstantSPos_ = false;

    addr == fAddr_ ? taskFAngle_ = angle : taskSAngle_ = angle;

    //qDebug() << "runSteps: addr: " << addr << ", speed: " << speed << ", angle: " << angle << ", steps: " << steps << ", needSecs: " << needSecs << ", deltaAngle: " << (addr == fAddr_ ? deltaAngle : deltaAngle * reducCoeff);

    if (!force) {
        if (!respStruct_.isUndefined()) {
            return MKS_FAIL;
        }
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

    return MKS_OK;
}

MKS_Status MotorControl::runStepsResponseCheck(const QByteArray& data)
{
    MKS_Status retVal = MKS_Status::MKS_OK;

    const int count = 2;

    if (respStruct_.type_ != ResponseType::kRunSteps || data.size() != sizeof(struct_3byte) * count) {
        retVal = MKS_FAIL;
    }
    else {
        struct_3byte answer[count];
        memcpy(&answer, data.data(), sizeof(struct_3byte) * count);

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

    return retVal;
}

MKS_Status MotorControl::position(uint8_t addr, int32_t* value, float* angle)
{
    Q_UNUSED(value);
    Q_UNUSED(angle);

    if (!respStruct_.isUndefined()) {
        return MKS_FAIL;
    }

    if (addr > 9) {
        return MKS_ADDR;
    }

    respStruct_ = { addr, ResponseType::kPosition };

    uint8_t msg[3] = {static_cast<uint8_t>(0xE0 + addr), 0x30, static_cast<uint8_t>(0x10 + addr)};
    uartSend(msg, 3);

    return MKS_OK;
}

MKS_Status MotorControl::positionResponseCheck(const QByteArray& data)
{
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

        // TODO: now it's not working correctly
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
        float angle = mks_value;

        if (answer.addr == sAddr_)
            angle = angle * reducCoeff;

        emit angleChanged(answer.addr, angle);
    }

    resetResponseStruct();

    return retVal;
}

MKS_Status MotorControl::motorPosition(uint8_t addr, int32_t* value, float* angle)
{
    Q_UNUSED(value);
    Q_UNUSED(angle);

    if (addr > 9) {
        return MKS_ADDR;
    }

    respStruct_ = { addr, ResponseType::kMotorPosition };

    uint8_t msg[3] = {static_cast<uint8_t>(0xE0 + addr), 0x36, static_cast<uint8_t>(0x16 + addr)};
    uartSend(msg, 3);

    return MKS_OK;
}

MKS_Status MotorControl::motorPositionResponseCheck(const QByteArray& data)
{
    //qDebug() << "MotorControl::motorPositionResponseCheck";

    MKS_Status retVal = MKS_Status::MKS_OK;

    if (respStruct_.type_ != ResponseType::kMotorPosition || data.size() != sizeof(struct_encAnswerMotorPos)) {
        retVal = MKS_FAIL;
    }
    else {
        struct_encAnswerMotorPos answer;
        memcpy(&answer, data.data(), sizeof(struct_encAnswerMotorPos));

        uint8_t inCRC = calculateCrc((uint8_t*)&answer, 7);

        if (inCRC != answer.crc) {
            retVal = MKS_CRC;
        }
        else if (answer.addr != (0xE0 + respStruct_.address_)) {
            retVal = MKS_ADDR_ANS;
        }

        uint8_t carry_arr[4] = {answer.carry[3], answer.carry[2], answer.carry[1], answer.carry[0]};

        auto isConstantSender = [this]() {
            if (isConstantFPos_ && isConstantSPos_) {
                float currFAngle = PosToAngle(fPos_);
                float currSAngle = PosToAngle(sPos_);
                emit posIsConstant(currFAngle, taskFAngle_, currSAngle * reducCoeff, taskSAngle_);
            }
        };

        if (answer.addr == 225) {
            if (!isConstantFPos_) {
                float delta = std::fabs(PosToAngle(*(int32_t*)(carry_arr)) - PosToAngle(fPos_));

                if (delta < confirmDelta_) {
                    isConstantFPos_ = true;
                    isConstantSender();
                }
            }

            fPos_ = *(int32_t*)(carry_arr);

            if (fStartMark_) {
                runSteps(fAddr_,1,0.0f,true);
                fStartMark_ = false;
            }
        }
        if (answer.addr == 226) {
            if (!isConstantSPos_) {
                float delta = std::fabs(PosToAngle(*(int32_t*)(carry_arr)) - PosToAngle(sPos_));

                if (delta < confirmDelta_) {
                    isConstantSPos_ = true;
                    isConstantSender();
                }
            }

            sPos_ = *(int32_t*)(carry_arr);

            if (sStartMark_) {
                runSteps(sAddr_,1,0.0f, true);
                sStartMark_ = false;
            }
        }

        float angle = answer.addr == 226 ? PosToAngle(*(int32_t*)(carry_arr)) * reducCoeff : PosToAngle(*(int32_t*)(carry_arr));

        emit angleChanged(answer.addr, angle);
    }

    resetResponseStruct();

    return retVal;
}

MKS_Status MotorControl::goZero(uint8_t addr, bool force)
{
    if (!force) {
        if (!respStruct_.isUndefined()) {
            return MKS_FAIL;
        }
    }

    if (addr > 9) {
        return MKS_ADDR;
    }

    respStruct_ = { addr, ResponseType::kGoZero };

    uint8_t msg[4] = {static_cast<uint8_t>(0xE0 + addr), 0x94, 0x00, static_cast<uint8_t>(0x74 + addr)};
    uartSend(msg, 4);

    return MKS_OK;
}

MKS_Status MotorControl::goZeroResponseCheck(const QByteArray& data)
{
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

    return retVal;
}

QString MotorControl::convertStatus(MKS_Status status)
{
    QString retVal;

    switch (status) {
    case MKS_Status::MKS_ADDR:      { retVal = "MKS_ADDR";      break; }
    case MKS_Status::MKS_ADDR_ANS:  { retVal = "MKS_ADDR_ANS";  break; }
    case MKS_Status::MKS_CRC:       { retVal = "MKS_CRC";       break; }
    case MKS_Status::MKS_FAIL:      { retVal = "MKS_FAIL";      break; }
    case MKS_Status::MKS_OK:        { retVal = "MKS_OK";        break; }
    default: break;
    }

    return retVal;
}

uint8_t MotorControl::getFAddr() const
{
    return fAddr_;
}

uint8_t MotorControl::getSAddr() const
{
    return sAddr_;
}
