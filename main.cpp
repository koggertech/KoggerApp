#include <QGuiApplication>
#include <QQmlContext>
#include <QQmlApplicationEngine>
#include <waterfall.h>
#include <plotcash.h>
#include <connection.h>
#include <console.h>
#include <core.h>

Core core;

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

    qmlRegisterType<WaterFall>("WaterFall", 1, 0, "WaterFall");
//    qmlRegisterType<Console>("Console", 1, 0, "Console");

    engine.rootContext()->setContextProperty("core", &core);
#ifdef FLASHER
    engine.rootContext()->setContextProperty("flasher", &core.flasher);
#endif
    engine.rootContext()->setContextProperty("sonarDriver", core.dev_driver);
    engine.rootContext()->setContextProperty("logViewer", core.console());

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

    return app.exec();
}
