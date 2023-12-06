#ifndef ANDROID_H
#define ANDROID_H

#include <QApplication>

#if defined(Q_OS_ANDROID)
bool checkAndroidWritePermission();
#endif

#endif // ANDROID_H
