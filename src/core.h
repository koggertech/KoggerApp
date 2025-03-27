#pragma once

#include <QObject>
#include <QUrl>
#include <QQmlApplicationEngine>
#include <QStandardItemModel>
#include <QQmlContext>
#include <QThread>
#ifdef FLASHER
#include "flasher.h"
#endif
#include "waterfall.h"
#include "logger.h"
#include "console.h"
#include "ConverterXTF.h"
#include "graphicsscene3dview.h"
#include "boattrackcontrolmenucontroller.h"
#include "navigation_arrow_control_menu_controller.h"
#include "bottomtrackcontrolmenucontroller.h"
#include "surfacecontrolmenucontroller.h"
#include "side_scan_view_control_menu_controller.h"
#include "image_view_control_menu_controller.h"
#include "map_view_control_menu_controller.h"
#include "usbl_view_control_menu_controller.h"
#include "pointgroupcontrolmenucontroller.h"
#include "polygongroupcontrolmenucontroller.h"
#include "mpcfiltercontrolmenucontroller.h"
#include "npdfiltercontrolmenucontroller.h"
#include "scene3dtoolbarcontroller.h"
#include "scene3dcontrolmenucontroller.h"
#include "DeviceManagerWrapper.h"
#include "LinkManagerWrapper.h"
//#include <FileReader.h>


class Core : public QObject
{
    Q_OBJECT

public:
    Core();
    ~Core();

    Q_PROPERTY(bool isFactoryMode READ isFactoryMode CONSTANT)
    Q_PROPERTY(bool isSupportedMotorControlMode READ isMotorControlMode CONSTANT)

    Q_PROPERTY(ConsoleListModel* consoleList READ consoleList CONSTANT)
    Q_PROPERTY(bool loggingKlf WRITE setKlfLogging)
    Q_PROPERTY(bool loggingCsv WRITE setCsvLogging)
    Q_PROPERTY(bool fixBlackStripesState WRITE setFixBlackStripesState)
    Q_PROPERTY(int  fixBlackStripesRange WRITE setFixBlackStripesRange)
    Q_PROPERTY(QString filePath READ getFilePath NOTIFY filePathChanged)
    Q_PROPERTY(bool isFileOpening READ getIsFileOpening NOTIFY sendIsFileOpening)
    Q_PROPERTY(bool isMosaicUpdatingInThread READ getIsMosaicUpdatingInThread NOTIFY isMosaicUpdatingInThreadUpdated)
    Q_PROPERTY(bool isSideScanPerformanceMode READ getIsSideScanPerformanceMode NOTIFY isSideScanPerformanceModeUpdated)
    Q_PROPERTY(bool isSeparateReading READ getIsSeparateReading CONSTANT)

    void setEngine(QQmlApplicationEngine *engine);
    Console* getConsolePtr();
    Dataset* getDatasetPtr();
    DeviceManagerWrapper* getDeviceManagerWrapperPtr() const;
    LinkManagerWrapper* getLinkManagerWrapperPtr() const;
    void stopLinkManagerTimer() const;
#ifdef SEPARATE_READING
    QString getTryOpenedfilePath() const;
    void stopDeviceManagerThread() const;
#endif
    void consoleInfo(QString msg);
    void consoleWarning(QString msg);
    void consoleProto(FrameParser& parser, bool isIn = true);
#ifdef FLASHER
    void getFlasherPtr() const;
#endif
    void saveLLARefToSettings();
    void removeLinkManagerConnections();
#ifdef SEPARATE_READING
    void removeDeviceManagerConnections();
#endif

public slots:
#ifdef SEPARATE_READING
    void openLogFile(const QString& filePath, bool isAppend = false, bool onCustomEvent = false);
    bool closeLogFile(bool onOpen = false);
    void onFileStartOpening();
    void onFileReadEnough();
    void onFileOpenBreaked(bool onOpen);
#else
    void openLogFile(const QString& filePath, bool isAppend = false, bool onCustomEvent = false);
    bool closeLogFile();
#endif
    void onFileOpened();
    bool openXTF(QByteArray data);    
    bool openCSV(QString name, int separatorType, int row = -1, int colTime = -1, bool isUtcTime = true, int colLat = -1, int colLon = -1, int colAltitude = -1, int colNorth = -1, int colEast = -1, int colUp = -1);
    bool openProxy(const QString& address, const int port, bool isTcp);
    bool closeProxy();
    bool upgradeFW(const QString& name, QObject* dev);
    void upgradeChanged(int progressStatus);
    void setKlfLogging(bool isLogging);
    void setFixBlackStripesState(bool state);
    void setFixBlackStripesRange(int val);
    bool getIsKlfLogging();
    void setCsvLogging(bool isLogging);
    bool getIsCsvLogging();
    bool exportComplexToCSV(QString filePath);
    bool exportUSBLToCSV(QString filePath);
    bool exportPlotAsCVS(QString filePath, int channel, float decimation = 0);
    bool exportPlotAsXTF(QString filePath);
    void setPlotStartLevel(int level);
    void setPlotStopLevel(int level);
    void setTimelinePosition(double position);
    void resetAim();
    void UILoad(QObject* object, const QUrl& url);
    void setSideScanChannels(int firstChId, int secondChId);
#ifdef FLASHER
    bool simpleFlash(const QString &name);
    bool factoryFlash(const QString &name, int sn, QString pn, QObject* dev);
#endif
    bool getIsFileOpening() const;
    void setIsMosaicUpdatingInThread(bool state);
    void setSideScanWorkMode(SideScanView::Mode mode);

    bool getIsMosaicUpdatingInThread() const;
    bool getIsSideScanPerformanceMode() const;
    bool getIsSeparateReading() const;
    void onChannelsUpdated();

#if defined(FAKE_COORDS)
    Q_INVOKABLE void setPosZeroing(bool state);
#endif

signals:
    void connectionChanged(bool duplex = false);
    void filePathChanged();
    void sendIsFileOpening();
    void isMosaicUpdatingInThreadUpdated();
    void isSideScanPerformanceModeUpdated();

#ifdef SEPARATE_READING
    void sendCloseLogFile(bool onOpen = false);
#endif
private slots:
#ifdef FLASHER
    void updateDeviceID(QByteArray uid);
    void flasherConnectionChanged(Flasher::BootState connection_status);
    bool reconnectForFlash();
#endif

private:
    /*methods*/
    ConsoleListModel* consoleList();
    void createControllers();
    void createDeviceManagerConnections();
    void createLinkManagerConnections();
    bool isOpenedFile() const;
    bool isFactoryMode() const;
    bool isMotorControlMode() const;

    QString getFilePath() const;
    void fixFilePathString(QString& filePath) const;
    void loadLLARefFromSettings();

    /*data*/
    Console* consolePtr_;
    // 3d scene controllers
    std::shared_ptr<BoatTrackControlMenuController> boatTrackControlMenuController_;
    std::shared_ptr<NavigationArrowControlMenuController> navigationArrowControlMenuController_;
    std::shared_ptr<BottomTrackControlMenuController> bottomTrackControlMenuController_;
    std::shared_ptr<MpcFilterControlMenuController> mpcFilterControlMenuController_;
    std::shared_ptr<NpdFilterControlMenuController> npdFilterControlMenuController_;
    std::shared_ptr<SurfaceControlMenuController> surfaceControlMenuController_;
    std::shared_ptr<SideScanViewControlMenuController> sideScanViewControlMenuController_;
    std::shared_ptr<ImageViewControlMenuController> imageViewControlMenuController_;
    std::shared_ptr<MapViewControlMenuController> mapViewControlMenuController_;
    std::shared_ptr<PointGroupControlMenuController> pointGroupControlMenuController_;
    std::shared_ptr<PolygonGroupControlMenuController> polygonGroupControlMenuController_;
    std::shared_ptr<Scene3DControlMenuController> scene3dControlMenuController_;
    std::shared_ptr<Scene3dToolBarController> scene3dToolBarController_;
    std::shared_ptr<UsblViewControlMenuController> usblViewControlMenuController_;
    std::unique_ptr<DeviceManagerWrapper> deviceManagerWrapperPtr_;
    std::unique_ptr<LinkManagerWrapper> linkManagerWrapperPtr_;

#ifdef SEPARATE_READING
    QString tryOpenedfilePath_;
    bool fileIsCompleteOpened_ = false;
    QList<QMetaObject::Connection> deviceManagerWrapperConnections_;
#endif

    QQmlApplicationEngine* qmlAppEnginePtr_;
    Dataset* datasetPtr_;
    GraphicsScene3dView* scene3dViewPtr_;
    ConverterXTF converterXtf_;
    Logger logger_;
    QList<qPlot2D*> plot2dList_;
    QList<QMetaObject::Connection> linkManagerWrapperConnections_;
    QString openedfilePath_;
    bool isLoggingKlf_;
    bool isLoggingCsv_;
    QString filePath_;
#ifdef FLASHER
    Flasher flasher;
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
    bool isFileOpening_;
    bool isMosaicUpdatingInThread_;
    bool isSideScanPerformanceMode_;
};
