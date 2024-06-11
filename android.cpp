#include "android.h"

#include <QGuiApplication>
#include <QQmlContext>
#include <QApplication>
#include <QQmlApplicationEngine>

//#include <core.h>
//Core core;

#if defined(Q_OS_ANDROID)
#include "android.h"

#if defined(Q_OS_ANDROID)
#include <jni.h>
#include <QtGlobal>
#include <QtAndroid>
#if (QT_VERSION_MAJOR < 6)
#include <QtAndroidExtras>
#include <QAndroidJniObject>
#include <QAndroidJniEnvironment>
#else
#include <QJniObject>
#include <QJniEnvironment>

#define QAndroidJniEnvironment  QJniEnvironment
#define QAndroidJniObject   QJniObject
#endif

#include "qtandroidserialport/src/qserialport.h"



static jobject _class_loader = nullptr;
static jobject _context = nullptr;

//-----------------------------------------------------------------------------
extern "C" {
    void gst_amc_jni_set_java_vm(JavaVM *java_vm);

    jobject gst_android_get_application_class_loader(void)
    {
        return _class_loader;
    }
}

//-----------------------------------------------------------------------------
static void
gst_android_init(JNIEnv* env, jobject context)
{
    jobject class_loader = nullptr;

    jclass context_cls = env->GetObjectClass(context);
    if (!context_cls) {
        return;
    }

    jmethodID get_class_loader_id = env->GetMethodID(context_cls, "getClassLoader", "()Ljava/lang/ClassLoader;");
    if (env->ExceptionCheck()) {
        env->ExceptionDescribe();
        env->ExceptionClear();
        return;
    }

    class_loader = env->CallObjectMethod(context, get_class_loader_id);
    if (env->ExceptionCheck()) {
        env->ExceptionDescribe();
        env->ExceptionClear();
        return;
    }

    _context = env->NewGlobalRef(context);
    _class_loader = env->NewGlobalRef (class_loader);
}

//-----------------------------------------------------------------------------
static const char kJniClassName[] {"org/kogger/koggerapp/KoggerActivity"};

void setNativeMethods(void)
{
    JNINativeMethod javaMethods[] {
        {"nativeInit", "()V", reinterpret_cast<void *>(gst_android_init)}
    };

    QAndroidJniEnvironment jniEnv;
    if (jniEnv->ExceptionCheck()) {
        jniEnv->ExceptionDescribe();
        jniEnv->ExceptionClear();
    }

    jclass objectClass = jniEnv->FindClass(kJniClassName);
    if(!objectClass) {
//        core.consoleInfo("Couldn't find class:");
        return;
    }

    jint val = jniEnv->RegisterNatives(objectClass, javaMethods, sizeof(javaMethods) / sizeof(javaMethods[0]));

    if (val < 0) {
//        qWarning() << "Error registering methods: " << val;
//        core.consoleInfo("Error registering methods: ");
    } else {
//        qDebug() << "Main Native Functions Registered";
//        core.consoleInfo("Main Native Functions Registered");
    }

    if (jniEnv->ExceptionCheck()) {
        jniEnv->ExceptionDescribe();
        jniEnv->ExceptionClear();
    }
}

//-----------------------------------------------------------------------------
jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
    Q_UNUSED(reserved);

    JNIEnv* env;
    if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
        return -1;
    }

    setNativeMethods();
    QSerialPort::setNativeMethods();

    return JNI_VERSION_1_6;
}

bool checkAndroidWritePermission() {

    QtAndroid::PermissionResult r = QtAndroid::checkPermission("android.permission.WRITE_EXTERNAL_STORAGE");

    if (r == QtAndroid::PermissionResult::Denied) {
        QtAndroid::requestPermissionsSync( QStringList() << "android.permission.WRITE_EXTERNAL_STORAGE" );
        QtAndroid::requestPermissionsSync( QStringList() << "android.permission.READ_EXTERNAL_STORAGE" );

        r = QtAndroid::checkPermission("android.permission.WRITE_EXTERNAL_STORAGE");
        if (r == QtAndroid::PermissionResult::Denied) {
             return false;
        }
   }
   return true;
}
#endif
#endif
