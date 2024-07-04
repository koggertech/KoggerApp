#pragma once

#include <QObject>
#include <QUuid>
#include "Link.h"

class MotorControl : public QObject
{
    Q_OBJECT

public:
    /*methods*/
    MotorControl(QObject* parent = nullptr, Link* linkPtr = nullptr);
    ~MotorControl();

public slots:

signals:

private:
    Link* linkPtr_;

private slots:

};
