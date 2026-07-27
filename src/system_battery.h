#pragma once

#include <QObject>
#include <QTimer>

class SystemBattery : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int level READ level NOTIFY changed)
    Q_PROPERTY(bool available READ available NOTIFY changed)
    Q_PROPERTY(bool charging READ charging NOTIFY changed)

public:
    explicit SystemBattery(QObject* parent = nullptr);

    int level() const { return level_; }
    bool available() const { return available_; }
    bool charging() const { return charging_; }

signals:
    void changed();

private:
    void refresh();

    QTimer timer_;
    int level_ = -1;
    bool available_ = false;
    bool charging_ = false;
};
