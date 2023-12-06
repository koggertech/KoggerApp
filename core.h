#ifndef CORE_H
#define CORE_H

#include <QObject>
#include <connection.h>
#include <console.h>
#include <DevQProperty.h>
#include <QUrl>
#include <QQmlApplicationEngine>
#include <QStandardItemModel>
#include <QQmlContext>
#include <waterfall.h>
#include <DevHub.h>
#include <logger.h>
#include <QThread>
#include <3Plot.h>
#include <Plot2D.h>

#include "XTFConf.h"
#include "ConverterXTF.h"

#include <graphicsscene3dview.h>
#include <bottomtrackprovider.h>
#include <bottomtrackcontrolmenucontroller.h>
#include <surfacecontrolmenucontroller.h>
#include <pointgroupcontrolmenucontroller.h>
#include <polygongroupcontrolmenucontroller.h>
#include <mpcfiltercontrolmenucontroller.h>
#include <npdfiltercontrolmenucontroller.h>
#include <scene3dtoolbarcontroller.h>
#include <scene3dcontrolmenucontroller.h>

//#define FLASHER

#ifdef FLASHER
#include "flasher.h"
#endif

class Core : public QObject
{
    Q_OBJECT

public:
    explicit Core();

    Q_PROPERTY(bool isFactoryMode READ isFactoryMode)

    Q_PROPERTY(ConsoleListModel* consoleList READ consoleList CONSTANT)

    Q_PROPERTY(bool logging WRITE setLogging)

    ConsoleListModel* consoleList() {
        return m_console->listModel();
    }

    Console *console() {
        return m_console;
    }

    Dataset* dataset() {
        return _dataset;
    }

    void consoleInfo(QString msg) {
        console()->put(QtMsgType::QtInfoMsg, msg);
    }

    void consoleWarning(QString msg) {
        console()->put(QtMsgType::QtWarningMsg, msg);
    }

    void consoleProto(FrameParser &parser, bool is_in = true);

    void setEngine(QQmlApplicationEngine *engine);

public slots:
    QList<QSerialPortInfo> availableSerial();
    QStringList availableSerialName();
    bool openConnectionAsSerial(const int id, bool autoconn, const QString &name, int baudrate, bool mode);
    bool openConnectionAsIP(const int id, bool autoconn, const QString &address, const int port, bool is_tcp);
    bool openConnectionAsFile(const int id, const QString &name, bool is_append = false);
    bool openXTF(QByteArray data);
    bool openCSV(QString name, int separator_type, int row = -1, int col_time = -1, bool is_utc_time = true, int col_lat = -1, int col_lon = -1, int col_north = -1, int col_east = -1, int altitude = -1, int distance = -1);
    bool devsConnection();

    bool isOpenConnection();
    bool closeConnection();

    bool openProxy(const QString &address, const int port, bool is_tcp);
    bool closeProxy();

    bool connectionBaudrate(int baudrate);

    bool upgradeFW(const QString &name, QObject* dev);
    void upgradeChanged(int progress_status);

    void setLogging(bool is_logging);
    bool isLogging();



    bool exportPlotAsCVS(QString file_path, int channel, float decimation = 0);
    bool exportPlotAsXTF(QString file_path);


#ifdef FLASHER
    bool simpleFlash(const QString &name);
    bool factoryFlash(const QString &name, int sn, QString pn, QObject* dev);
#endif

    void setPlotStartLevel(int level) {
//        m_plot->setStartLevel(level);
        for(int i = 0; i < _plots2d.size(); i++) {
            if(_plots2d.at(i) != NULL) {
                _plots2d.at(i)->setEchogramLowLevel(level);
            }
        }
    }

    void setPlotStopLevel(int level) {
//        m_plot->setStopLevel(level);
//        m_waterFall->setEchogramHightLevel(level);
        for(int i = 0; i < _plots2d.size(); i++) {
            if(_plots2d.at(i) != NULL) {
                _plots2d.at(i)->setEchogramHightLevel(level);
            }
        }
    }

    void setTimelinePosition(double position) {
//        m_plot->setTimelinePosition(position);
//        m_waterFall->setDataPosition(position);
        for(int i = 0; i < _plots2d.size(); i++) {
            if(_plots2d.at(i) != NULL) {
                _plots2d.at(i)->setTimelinePosition(position);
            }
        }
    }

//    void setChartVis(bool visible) {
//        m_plot->setChartVis(visible);
//    }

//    void setOscVis(bool visible) {
//        m_plot->setOscVis(visible);
//    }

//    void setDistVis(bool visible) {
//        m_plot->setDistVis(visible);
//    }

    Device* dev() { return &_devs; }

    void UILoad(QObject *object, const QUrl &url);

signals:
    void connectionChanged(bool duplex = false);

public:
    Console *m_console;
    Connection *m_connection;
    Dataset* _dataset;
    QList<qPlot2D*> _plots2d;
//    QPlot2D* _plot1 = NULL;
    FboInSGRenderer* _render = NULL;
    GraphicsScene3dView* m_scene3dView = nullptr;


    Device _devs;
    Logger _logger;
    ConverterXTF _converterXTF;
    QThread connectionThread;
    QQmlApplicationEngine *m_engine = nullptr;

#ifdef FLASHER
    Flasher flasher;
#endif

private slots:
    void closing();
#ifdef FLASHER
    void updateDeviceID(QByteArray uid);
    void flasherConnectionChanged(Flasher::BootState connection_status);
    bool reconnectForFlash();
#endif

protected:
#ifdef FLASHER
    QByteArray boot_data;
    QByteArray fw_data;

    QTcpSocket *_socket = new QTcpSocket();
    bool getFW(void* uid);
    QString _pn;
    enum  {
        FactoryIdle,
        FactoryTest,
        FactoryProduct,
        FactorySimple
    } _factoryState = FactoryIdle;

    QByteArray _flashUID;
#endif

    bool _isLogging;

    int backupBaudrate = 115200;
    void restoreBaudrate();
    void setUpgradeBaudrate();

private:

    //! Метод создания контроллеров
    void createControllers();
    //! Метод создания моделей
    void createModels();
\

private:

    // View controllers
    std::shared_ptr <BottomTrackControlMenuController>  m_bottomTrackControlMenuController;
    std::shared_ptr <MpcFilterControlMenuController>    m_mpcFilterControlMenuController;
    std::shared_ptr <NpdFilterControlMenuController>    m_npdFilterControlMenuController;
    std::shared_ptr <SurfaceControlMenuController>      m_surfaceControlMenuController;
    std::shared_ptr <PointGroupControlMenuController>   m_pointGroupControlMenuController;
    std::shared_ptr <PolygonGroupControlMenuController> m_polygonGroupControlMenuController;
    std::shared_ptr <Scene3DControlMenuController>      m_scene3dControlMenuController;
    std::shared_ptr <Scene3dToolBarController>          m_scene3dToolBarController;

    std::shared_ptr <BottomTrackProvider> mpBottomTrackProvider;
    std::unique_ptr <QStandardItemModel>  m_sceneObjectListModel; ///< graphics object list model

    bool isFactoryMode() {
#ifdef FLASHER
        return true;
#else
        return false;
#endif
    }

};

#endif // CORE_H
