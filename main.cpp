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
#include <sceneobject.h>
#include "Plot2D.h"

Core core;
Themes theme;


void messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg) {
    core.consoleInfo(msg);
}

int main(int argc, char *argv[]) {

#if defined(Q_OS_LINUX)
    QApplication::setAttribute(Qt::AA_ForceRasterWidgets, false);
    ::qputenv("QT_SUPPORT_GL_CHILD_WIDGETS", "1");
#endif

    QCoreApplication::setOrganizationName("KOGGER");
    QCoreApplication::setOrganizationDomain("kogger.tech");
    QCoreApplication::setApplicationName("KoggerApp");
    QCoreApplication::setApplicationVersion("1-1-1");

#if defined(Q_OS_WIN)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif


    QSurfaceFormat format;
    format.setSwapInterval(0);
    QSurfaceFormat::setDefaultFormat(format);

    QGuiApplication app(argc, argv);
    QQmlApplicationEngine engine;

    engine.addImportPath("qrc:/");


    SceneObject::qmlDeclare();

//    qInstallMessageHandler(messageHandler);

//    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::Round);

    qmlRegisterType<qPlot2D>("WaterFall", 1, 0, "WaterFall");

    engine.rootContext()->setContextProperty("dataset", core.dataset());
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

    engine.load(url);

    qCritical() << "App is created";

#if defined(Q_OS_ANDROID)
//    checkAndroidWritePermission();
#endif

    return app.exec();
}
