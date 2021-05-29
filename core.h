#ifndef CORE_H
#define CORE_H

#include <QObject>
#include <connection.h>
#include <console.h>
#include <DevQProperty.h>
#include <QUrl>
#include <QQmlApplicationEngine>
#include <waterfall.h>
#include <DevHub.h>
#include <logger.h>


//#define FLASHER

#ifdef FLASHER
#include "flasher.h"
#endif

class Core : public QObject
{
    Q_OBJECT

public:
    explicit Core();

    Q_PROPERTY(ConsoleListModel* consoleList READ consoleList CONSTANT)

    Q_PROPERTY(bool logging WRITE setLogging)

    ConsoleListModel* consoleList() {
        return m_console->listModel();
    }

    Console *console() {
        return m_console;
    }

    PlotCash* plot() {
        return m_plot;
    }

    void consoleInfo(QString msg) {
        console()->put(QtMsgType::QtInfoMsg, msg);
    }

    void consoleWarning(QString msg) {
        console()->put(QtMsgType::QtWarningMsg, msg);
    }

    void consoleProto(ProtoBin &parser, bool is_in = true);

    void setEngine(QQmlApplicationEngine *engine) {
        m_engine = engine;
    }

public slots:
    QList<QSerialPortInfo> availableSerial();
    QStringList availableSerialName();
    bool openConnectionAsSerial(const QString &name, int baudrate, bool mode);
    bool devsConnection();
    bool openConnectionAsFile(const QString &name);
    bool isOpenConnection();
    bool closeConnection();

    bool connectionBaudrate(int baudrate);

    bool upgradeFW(const QString &name, QObject* dev);
    void upgradeChanged(int progress_status);

    void setLogging(bool is_logging);
    bool isLogging();

    bool exportPlotAsCVS();

#ifdef FLASHER
    bool factoryFlash(const QString &name, int sn, QString pn);
#endif

    void setPlotStartLevel(int level) {
        m_plot->setStartLevel(level);
    }

    void setPlotStopLevel(int level) {
        m_plot->setStopLevel(level);
    }

    void setTimelinePosition(double position) {
        m_plot->setTimelinePosition(position);
    }

    void setChartVis(bool visible) {
        m_plot->setChartVis(visible);
    }

    void setOscVis(bool visible) {
        m_plot->setOscVis(visible);
    }

    void setDistVis(bool visible) {
        m_plot->setDistVis(visible);
    }

    Device* dev() { return &_devs; }

    void UILoad(QObject *object, const QUrl &url);

signals:
    void connectionChanged(bool duplex = false);

public:
    Console *m_console;
    Connection *m_connection;
    PlotCash* m_plot;
    WaterFall* m_waterFall;

    Device _devs;

    Logger _logger;

    QQmlApplicationEngine *m_engine = nullptr;

#ifdef FLASHER
    Flasher flasher;
#endif

private slots:
    void closing();

#ifdef FLASHER
    void flasherConnectionChanged(Flasher::BootState connection_status);
#endif

protected:
#ifdef FLASHER
    QByteArray fw_data;
#endif

    bool _isLogging;

    int backupBaudrate = 0;
    void restoreBaudrate();
    void setUpgradeBaudrate();
};

#endif // CORE_H
