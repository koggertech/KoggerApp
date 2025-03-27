#ifndef ANDROID_H
#define ANDROID_H

#include <QApplication>
#include <QQmlApplicationEngine>

#if defined(Q_OS_ANDROID)
bool checkAndroidWritePermission();
void tryOpenFileAndroid(QQmlApplicationEngine& engine);

#endif

#endif // ANDROID_H
