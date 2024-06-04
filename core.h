#ifndef CORE_H
#define CORE_H

#include <QObject>
#include <console.h>
#include <DevQProperty.h>
#include <QUrl>
#include <QQmlApplicationEngine>
#include <QStandardItemModel>
#include <QQmlContext>
#include <waterfall.h>
#include <logger.h>
#include <QThread>
#include <3Plot.h>
#include <Plot2D.h>

#include "XTFConf.h"
#include "ConverterXTF.h"

#include <graphicsscene3dview.h>
#include <bottomtrackcontrolmenucontroller.h>
#include <surfacecontrolmenucontroller.h>
#include <pointgroupcontrolmenucontroller.h>
#include <polygongroupcontrolmenucontroller.h>
#include <mpcfiltercontrolmenucontroller.h>
#include <npdfiltercontrolmenucontroller.h>
#include <scene3dtoolbarcontroller.h>
#include <scene3dcontrolmenucontroller.h>

#include <DeviceManagerWrapper.h>
#include <LinkManagerWrapper.h>
#include <FileReader.h>

#ifdef FLASHER
#include "flasher.h"
#endif

class Core : public QObject
{
    Q_OBJECT

public:
    explicit Core();
    ~Core();

    Q_PROPERTY(bool isFactoryMode READ isFactoryMode CONSTANT)
    Q_PROPERTY(ConsoleListModel* consoleList READ consoleList CONSTANT)
    Q_PROPERTY(bool logging WRITE setLogging)
    Q_PROPERTY(int fileReaderProgress READ getFileReaderProgress NOTIFY fileReaderProgressChanged)

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

    DeviceManagerWrapper* getDeviceManagerWrapper() const;
    LinkManagerWrapper* getLinkManagerWrapperPtr() const;
    void stopLinkManagerTimer() const;

public slots:
    bool openConnectionAsSerial(const int id, bool autoconn, const QString &name, int baudrate, bool mode);
    bool openConnectionAsIP(const int id, bool autoconn, const QString &address, const int port, bool is_tcp);


    bool openConnectionAsFile(const int id, const QString &name, bool is_append = false);
    bool closeConnectionAsFile();


    bool openXTF(QByteArray data);
    bool openCSV(QString name, int separator_type, int row = -1, int col_time = -1, bool is_utc_time = true, int col_lat = -1, int col_lon = -1, int col_north = -1, int col_east = -1, int altitude = -1, int distance = -1);
    bool devsConnection();


    bool openProxy(const QString &address, const int port, bool is_tcp);
    bool closeProxy();

    bool upgradeFW(const QString &name, QObject* dev);
    void upgradeChanged(int progress_status);

    void setLogging(bool is_logging);
    bool isLogging();


    bool exportComplexToCSV(QString file_path);
    bool exportUSBLToCSV(QString file_path);
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

    //Device* dev() { return &_devs; }

    void UILoad(QObject *object, const QUrl &url);

    // fileReader
    void startFileReader(const QString& filePath);
    void stopFileReader();
    void receiveFileReaderProgress(int progress);
    int getFileReaderProgress();

signals:
    void connectionChanged(bool duplex = false);

    // fileReader
    //void sendStartFileReader(const QString& filePath);
    void sendStopFileReader();
    void fileReaderProgressChanged();

public:
    Console *m_console;
    Dataset* _dataset;
    QList<qPlot2D*> _plots2d;

    FboInSGRenderer* _render = NULL;
    GraphicsScene3dView* m_scene3dView = nullptr;


    //Device _devs;
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
    void createControllers();
    void createLinkManagerConnections();
    void removeLinkManagerConnections();

private:
    // deviceManager
    std::unique_ptr<DeviceManagerWrapper> deviceManagerWrapper_;

    // linkManager
    std::unique_ptr<LinkManagerWrapper> linkManagerWrapper_;
    QList<QMetaObject::Connection> linkManagerWrapperConnections_;

    // fileReader
    std::unique_ptr<FileReader> fileReader_;
    std::unique_ptr<QThread> fileReaderThread_;
    QList<QMetaObject::Connection> fileReaderConnections_;
    int fileReaderProgress_ = 0;

    // View controllers
    std::shared_ptr <BottomTrackControlMenuController>  m_bottomTrackControlMenuController;
    std::shared_ptr <MpcFilterControlMenuController>    m_mpcFilterControlMenuController;
    std::shared_ptr <NpdFilterControlMenuController>    m_npdFilterControlMenuController;
    std::shared_ptr <SurfaceControlMenuController>      m_surfaceControlMenuController;
    std::shared_ptr <PointGroupControlMenuController>   m_pointGroupControlMenuController;
    std::shared_ptr <PolygonGroupControlMenuController> m_polygonGroupControlMenuController;
    std::shared_ptr <Scene3DControlMenuController>      m_scene3dControlMenuController;
    std::shared_ptr <Scene3dToolBarController>          m_scene3dToolBarController;


    QString openedfilePath_;

    bool isFactoryMode() {
#ifdef FLASHER
        return true;
#else
        return false;
#endif
    }

};

#endif // CORE_H
