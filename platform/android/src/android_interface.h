/****************************************************************************
 *
 * Copyright (C) 2018 Pinecone Inc. All rights reserved.
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QString>
#include <QtCore/QLoggingCategory>

#include <functional>
#include <jni.h>

class QObject;

Q_DECLARE_LOGGING_CATEGORY(AndroidInterfaceLog)

namespace AndroidInterface
{
    bool cleanJavaException();
    jclass getActivityClass();
    void setNativeMethods();
    void jniLogDebug(JNIEnv *envA, jobject thizA, jstring messageA);
    void jniLogWarning(JNIEnv *envA, jobject thizA, jstring messageA);
    void jniStoragePermissionResult(JNIEnv *envA, jobject thizA, jboolean grantedA);
    bool checkStoragePermissions();
    void setStoragePermissionHandler(QObject *context, std::function<void(bool)> handler);
    QString getSDCardPath();
    void setKeepScreenOn(bool on);
    void moveTaskToBack();

    constexpr const char *kJniKoggerActivityClassName = "org/kogger/koggerapp/KoggerActivity";
};
