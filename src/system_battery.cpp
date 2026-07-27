#include "system_battery.h"

#if defined(Q_OS_WIN)
#include <windows.h>
#elif defined(Q_OS_ANDROID)
#include <QCoreApplication>
#include <QJniObject>
#elif defined(Q_OS_LINUX)
#include <QFile>
#endif

SystemBattery::SystemBattery(QObject* parent)
    : QObject(parent)
{
    timer_.setInterval(30000);
    connect(&timer_, &QTimer::timeout, this, &SystemBattery::refresh);
    refresh();
    timer_.start();
}

void SystemBattery::refresh()
{
    int level = -1;
    bool available = false;
    bool charging = false;

#if defined(Q_OS_WIN)
    SYSTEM_POWER_STATUS sps;
    if (GetSystemPowerStatus(&sps)) {
        const bool noBattery = (sps.BatteryFlag & 128) != 0;
        if (!noBattery && sps.BatteryLifePercent <= 100) {
            available = true;
            level = sps.BatteryLifePercent;
        }
        charging = (sps.ACLineStatus == 1) || ((sps.BatteryFlag & 8) != 0);
    }
#elif defined(Q_OS_ANDROID)
    QJniObject context = QNativeInterface::QAndroidApplication::context();
    if (context.isValid()) {
        QJniObject svcName = QJniObject::getStaticObjectField<jstring>(
            "android/content/Context", "BATTERY_SERVICE");
        QJniObject bm = context.callObjectMethod(
            "getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;",
            svcName.object<jstring>());
        if (bm.isValid()) {
            const jint cap = bm.callMethod<jint>("getIntProperty", "(I)I", 4); // BATTERY_PROPERTY_CAPACITY
            if (cap >= 0 && cap <= 100) {
                available = true;
                level = cap;
            }
            const jint status = bm.callMethod<jint>("getIntProperty", "(I)I", 6); // BATTERY_PROPERTY_STATUS
            charging = (status == 2); // BatteryManager.BATTERY_STATUS_CHARGING
        }
    }
#elif defined(Q_OS_LINUX)
    const QStringList bats = { QStringLiteral("BAT0"), QStringLiteral("BAT1"), QStringLiteral("BAT2") };
    for (const QString& bat : bats) {
        QFile cap(QStringLiteral("/sys/class/power_supply/%1/capacity").arg(bat));
        if (!cap.open(QIODevice::ReadOnly))
            continue;
        bool ok = false;
        const int v = cap.readAll().trimmed().toInt(&ok);
        if (!ok)
            continue;
        available = true;
        level = v;
        QFile st(QStringLiteral("/sys/class/power_supply/%1/status").arg(bat));
        if (st.open(QIODevice::ReadOnly))
            charging = (st.readAll().trimmed() == "Charging");
        break;
    }
#endif

    if (level != level_ || available != available_ || charging != charging_) {
        level_ = level;
        available_ = available;
        charging_ = charging;
        emit changed();
    }
}
