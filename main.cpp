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

    core.consoleInfo("Run...");

    QCoreApplication::setOrganizationName("KOGGER");
    QCoreApplication::setOrganizationDomain("kogger.tech");
    QCoreApplication::setApplicationName("Sonar Viewer");
    QCoreApplication::setApplicationVersion("1-1-1");


#if defined(Q_OS_WIN)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
    QGuiApplication app(argc, argv);
    QQmlApplicationEngine engine;

//    WaterFall waterfall;
//    engine.rootContext()->setContextProperty("waterfall", &waterfall);

    qmlRegisterType<WaterFall>("WaterFall", 1, 0, "WaterFall");
//    qmlRegisterType<Console>("Console", 1, 0, "Console");
//    qmlRegisterType<QSerialPortInfo>("SerialList", 1, 0, "SerialList");


    engine.rootContext()->setContextProperty("core", &core);
    engine.rootContext()->setContextProperty("sonarDriver", core.dev_driver);



//    PlotCash plot_cash;
//    plot_cash.setLineCount(1000);
//    QVector<uint8_t> data(1000);
//    plot_cash.addData(&data, 1000, 10, 200);
//    plot_cash.addData(&data, 1000, 10, 200);
//    plot_cash.addData(&data, 1000, 10, 200);
//    plot_cash.getImage({100, 100});

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



//    QQmlComponent component(&engine, url);
//    QObject *object = component.create();

//    WaterFall* childObject = object->findChild<WaterFall*>();
//    qInfo("WF, %u", childObject);



//    QVector<uint8_t> data(1000);
//    data.fill(100);
//    childObject->Plot.addData(&data, 1000, 10, 200);

    return app.exec();
}
