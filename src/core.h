#pragma once

#include <QObject>
#include <QUrl>
#include <QQmlApplicationEngine>
#include <QStandardItemModel>
#include <QQmlContext>
#include <QThread>
#ifdef FLASHER
#include "flasher/deviceflasher.h"
#endif
#include "data_processor.h"
#include "qPlot2D.h"
#include "logger.h"
#include "console.h"
#include "converter_xtf.h"
#include "scene3d_view.h"
#include "boat_track_control_menu_controller.h"
#include "navigation_arrow_control_menu_controller.h"
#include "bottom_track_control_menu_controller.h"
#include "isobaths_view_control_menu_controller.h"
#include "mosaic_view_control_menu_controller.h"
#include "image_view_control_menu_controller.h"
#include "map_view_control_menu_controller.h"
#include "usbl_view_control_menu_controller.h"
#include "point_group_control_menu_controller.h"
#include "polygon_group_control_menu_controller.h"
#include "mpc_filter_control_menu_controller.h"
#include "npd_filter_control_menu_controller.h"
#include "scene3d_toolbar_controller.h"
#include "scene3d_control_menu_controller.h"
#include "device_manager_wrapper.h"
#include "link_manager_wrapper.h"
#include "tile_manager.h"
#include "data_horizon.h"


class Core : public QObject
{
    Q_OBJECT

public:
    Core();
    ~Core();

    Q_PROPERTY(bool              isGPSAlive                   READ getIsGPSAlive                   NOTIFY isGPSAliveChanged)
    Q_PROPERTY(bool              isFactoryMode                READ isFactoryMode                   CONSTANT)
    Q_PROPERTY(ConsoleListModel* consoleList                  READ consoleList                     CONSTANT)
    Q_PROPERTY(bool              loggingKlf                   READ getKlfLogging                   WRITE setKlfLogging)
    Q_PROPERTY(bool              isKlfLogging                 READ getKlfLogging                   NOTIFY loggingKlfChanged)
    Q_PROPERTY(bool              loggingCsv                   READ getCsvLogging                   WRITE setCsvLogging)
    Q_PROPERTY(bool              useGPS                       READ getUseGPS                       WRITE setUseGPS)
    Q_PROPERTY(bool              fixBlackStripesState         READ getFixBlackStripesState         WRITE setFixBlackStripesState)
    Q_PROPERTY(int               fixBlackStripesForwardSteps  READ getFixBlackStripesForwardSteps  WRITE setFixBlackStripesForwardSteps)
    Q_PROPERTY(int               fixBlackStripesBackwardSteps READ getFixBlackStripesBackwardSteps WRITE setFixBlackStripesBackwardSteps)
    Q_PROPERTY(QString           filePath                     READ getFilePath                     NOTIFY filePathChanged)
    Q_PROPERTY(bool              isFileOpening                READ getIsFileOpening                NOTIFY sendIsFileOpening)
    Q_PROPERTY(bool              isSeparateReading            READ getIsSeparateReading            CONSTANT)
    Q_PROPERTY(QString           ch1Name                      READ getChannel1Name                 NOTIFY channelListUpdated FINAL)
    Q_PROPERTY(QString           ch2Name                      READ getChannel2Name                 NOTIFY channelListUpdated FINAL)
    Q_PROPERTY(int               dataProcessorState           READ getDataProcessorState           NOTIFY dataProcessorStateChanged)

    void setEngine(QQmlApplicationEngine *engine);
    Console* getConsolePtr();
    Dataset* getDatasetPtr();
    DataProcessor* getDataProcessorPtr() const;
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
    void saveLLARefToSettings();
    void removeLinkManagerConnections();
#ifdef SEPARATE_READING
    void removeDeviceManagerConnections();
#endif
    QHash<QUuid, QString> getLinkNames() const;

public slots:    
    void setIsGPSAlive(bool state) { qDebug() << "Core::setIsGPSAlive" << state; isGPSAlive_ = state; emit isGPSAliveChanged(); }
    bool getIsGPSAlive() const { return isGPSAlive_; };

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
    bool openXTF(const QByteArray& data);
    bool openCSV(QString name, int separatorType, int row = -1, int colTime = -1, bool isUtcTime = true, int colLat = -1, int colLon = -1, int colAltitude = -1, int colNorth = -1, int colEast = -1, int colUp = -1);
    bool openProxy(const QString& address, const int port, bool isTcp);
    bool closeProxy();
    bool upgradeFW(const QString& name, QObject* dev);
    void upgradeChanged(int progressStatus);
    bool getKlfLogging() const;
    void setKlfLogging(bool isLogging);
    bool getFixBlackStripesState() const;
    int  getFixBlackStripesForwardSteps() const;
    int  getFixBlackStripesBackwardSteps() const;
    void setFixBlackStripesState(bool state);
    void setFixBlackStripesForwardSteps(int val);
    void setFixBlackStripesBackwardSteps(int val);
    bool getCsvLogging() const;
    void setCsvLogging(bool isLogging);
    bool getUseGPS() const;
    void setUseGPS(bool state);
    bool exportComplexToCSV(QString filePath);
    bool exportUSBLToCSV(QString filePath);
    bool exportPlotAsCVS(QString filePath, const ChannelId& channelId, float decimation = 0);
    bool exportPlotAsXTF(QString filePath);
    void setPlotStartLevel(int level);
    void setPlotStopLevel(int level);
    void setTimelinePosition(double position);
    void resetAim();
    void UILoad(QObject* object, const QUrl& url);
    void setMosaicChannels(const QString& firstChStr, const QString& secondChStr);
    bool getIsFileOpening() const;
    bool getIsSeparateReading() const;
    void onChannelsUpdated();
    void onRedrawEpochs(const QSet<int>& indxs);
    int getDataProcessorState() const;
    void initStreamList();

#ifdef FLASHER
    void connectOpenedLinkAsFlasher(QString pn);
    void setFlasherData(QString data);
    void releaseFlasherLink();
#endif

#if defined(FAKE_COORDS)
    Q_INVOKABLE void setPosZeroing(bool state);
#endif

    Q_INVOKABLE QString getChannel1Name() const;
    Q_INVOKABLE QString getChannel2Name() const;
    Q_INVOKABLE QVariant getConvertedMousePos(int indx, int mouseX, int mouseY);

    Q_INVOKABLE void setIsAttitudeExpected(bool state);

signals:
    void connectionChanged(bool duplex = false);
    void filePathChanged();
    void sendIsFileOpening();
    void channelListUpdated();
    void dataProcessorStateChanged();
    void isGPSAliveChanged();
    void loggingKlfChanged();

#ifdef SEPARATE_READING
    void sendCloseLogFile(bool onOpen = false);
#endif

private slots:
    void onFileStopsOpening();
    void onSendMapTextureIdByTileIndx(const map::TileIndex& tileIndx, GLuint textureId); // TODO: maybe store map texture id in mapView
    void onDataProcesstorStateChanged(const DataProcessorType& state);
    void onSendFrameInputToLogger(QUuid uuid, Link* link, const Parsers::FrameParser& frame);

private:
    /*methods*/
    void createMapTileManagerConnections();
    void createDatasetConnections();
    void createDataProcessor();
    void destroyDataProcessor();
    void createScene3dConnections();

    void setDataProcessorConnections();
    void resetDataProcessorConnections();

    ConsoleListModel* consoleList();
    void createControllers();
    void createDeviceManagerConnections();
    void createLinkManagerConnections();
    bool isOpenedFile() const;
    bool isFactoryMode() const;

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
    std::shared_ptr<IsobathsViewControlMenuController> isobathsViewControlMenuController_;
    std::shared_ptr<MosaicViewControlMenuController> mosaicViewControlMenuController_;
    std::shared_ptr<ImageViewControlMenuController> imageViewControlMenuController_;
    std::shared_ptr<MapViewControlMenuController> mapViewControlMenuController_;
    std::shared_ptr<PointGroupControlMenuController> pointGroupControlMenuController_;
    std::shared_ptr<PolygonGroupControlMenuController> polygonGroupControlMenuController_;
    std::shared_ptr<Scene3DControlMenuController> scene3dControlMenuController_;
    std::shared_ptr<Scene3dToolBarController> scene3dToolBarController_;
    std::shared_ptr<UsblViewControlMenuController> usblViewControlMenuController_;
    std::unique_ptr<DeviceManagerWrapper> deviceManagerWrapperPtr_;
    std::unique_ptr<LinkManagerWrapper> linkManagerWrapperPtr_;
    std::unique_ptr<map::TileManager> tileManager_;
    // data processor
    DataProcessor* dataProcessor_;
    QThread* dataProcThread_;
    std::unique_ptr<DataHorizon> dataHorizon_; // this thread

#ifdef SEPARATE_READING
    QString tryOpenedfilePath_;
    bool fileIsCompleteOpened_ = false;
    QList<QMetaObject::Connection> deviceManagerWrapperConnections_;
#endif

    QQmlApplicationEngine* qmlAppEnginePtr_;
    Dataset* datasetPtr_;
    QPointer<GraphicsScene3dView> scene3dViewPtr_;
    ConverterXTF converterXtf_;
    Logger logger_;
    QList<qPlot2D*> plot2dList_;
    QList<QMetaObject::Connection> linkManagerWrapperConnections_;
    QString openedfilePath_;
    bool isLoggingKlf_;
    bool isLoggingCsv_;
    QString filePath_;
    QString fChName_;
    QString sChName_;

    bool isFileOpening_;

    bool isGPSAlive_;
    bool isUseGPS_;

    bool fixBlackStripesState_;;
    int  fixBlackStripesForwardSteps_;
    int  fixBlackStripesBackwardSteps_;

#ifdef FLASHER
    Q_PROPERTY(QString flasherTextInfo READ flasherTextInfo NOTIFY dev_flasher_changed)
    Q_PROPERTY(int flasherIdInfo READ flasherIdInfo NOTIFY dev_flasher_changed)
private:
    DeviceFlasher dev_flasher_;
    int dev_flasher_msg_id_ = 0;
    QString dev_flasher_msg_;

    QString flasherTextInfo() { return dev_flasher_msg_; }
    int flasherIdInfo() { return dev_flasher_msg_id_; }
private slots:
    void dev_flasher_rcv(QString msg, int num);
signals:
    void dev_flasher_changed();
#endif

    QVector<QMetaObject::Connection> dataProcessorConnections_;
    DataProcessorType dataProcessorState_ = DataProcessorType::kUndefined;
};
