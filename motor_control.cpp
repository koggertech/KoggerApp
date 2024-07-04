#include "motor_control.h"


MotorControl::MotorControl(QObject* parent, Link* linkPtr) :
    QObject(parent),
    linkPtr_(linkPtr)
{

}

MotorControl::~MotorControl()
{

}
