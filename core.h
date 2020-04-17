#ifndef CORE_H
#define CORE_H

#include <QObject>
#include <connection.h>
#include <console.h>
#include <SonarDriver.h>
#include <QUrl>
#include <QQmlApplicationEngine>
#include <waterfall.h>

class Core : public QObject
{
    Q_OBJECT

public:
    explicit Core();

    Q_PROPERTY(ConsoleListModel* consoleList READ consoleList CONSTANT)
    ConsoleListModel* consoleList() {
        return m_console->listModel();
    }

    Console *console() {
        return m_console;
    }

    void consoleInfo(QString msg) {
        console()->put(QtMsgType::QtInfoMsg, msg);
    }

    void setEngine(QQmlApplicationEngine *engine) {
        m_engine = engine;
    }

public slots:
    QList<QSerialPortInfo> availableSerial();
    QStringList availableSerialName();
    bool openConnectionAsSerial(const QString &name, int baudrate);
    bool openConnectionAsFile(const QString &name);
    bool isOpenConnection();
    bool closeConnection();

    bool upgradeFW(const QString &name);

    void UILoad(QObject *object, const QUrl &url);

signals:
    void connectionChanged();

public:
    Console *m_console;
    Connection *m_connection;
    SonarDriver *dev_driver;
    PlotCash* m_plot;

    QQmlApplicationEngine *m_engine = nullptr;

private slots:
    void closing();
};

#endif // CORE_H
