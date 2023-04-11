#include <QGuiApplication>
#include <QQmlContext>
#include <QApplication>
#include <QQmlApplicationEngine>
#include <waterfall.h>
#include <plotcash.h>
#include <connection.h>
#include <console.h>
#include <core.h>
#include <Themes.h>
#include <QThread>
#include "3Plot.h"

Core core;
Themes theme;

#if defined(Q_OS_ANDROID)
#include "android.h"
#endif

void messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    core.consoleInfo(msg);
}

int main(int argc, char *argv[]) {
    QCoreApplication::setOrganizationName("KOGGER");
    QCoreApplication::setOrganizationDomain("kogger.tech");
    QCoreApplication::setApplicationName("KoggerApp");
    QCoreApplication::setApplicationVersion("1-1-1");

#if defined(Q_OS_WIN)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif

    QGuiApplication app(argc, argv);
    QQmlApplicationEngine engine;


    qInstallMessageHandler(messageHandler);

//    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::Round);

    qmlRegisterType<WaterFall>("WaterFall", 1, 0, "WaterFall");

    engine.rootContext()->setContextProperty("plot", core.plot());

    engine.rootContext()->setContextProperty("core", &core);
    engine.rootContext()->setContextProperty("theme", &theme);

#ifdef FLASHER
    engine.rootContext()->setContextProperty("flasher", &core.flasher);
#endif

    engine.rootContext()->setContextProperty("logViewer", core.console());
    engine.rootContext()->setContextProperty("devs", core.dev());

    core.consoleInfo("Run...");
    core.setEngine(&engine);

    const QUrl url(QStringLiteral("qrc:/main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);


    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &core, &Core::UILoad, Qt::QueuedConnection);

    engine.load(url);

#if defined(Q_OS_ANDROID)
//    checkAndroidWritePermission();
#endif

    return app.exec();
}
