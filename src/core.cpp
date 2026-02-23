#include "core.h"

#include <QSettings>
#include <cmath>
#include <ctime>
#include <QDebug>
#include "bottom_track.h"
#include "hotkeys_manager.h"
#include "tile_provider_ids.h"
#ifdef Q_OS_WINDOWS
#include <Windows.h>
#endif


Core::Core() :
    QObject(),
    consolePtr_(new Console),
    deviceManagerWrapperPtr_(std::make_unique<DeviceManagerWrapper>(this)),
    linkManagerWrapperPtr_(std::make_unique<LinkManagerWrapper>(this)),
    internetManager_(nullptr),
    internetThread_(nullptr),
    dataProcessor_(nullptr),
    dataProcThread_(nullptr),
    dataHorizon_(std::make_unique<DataHorizon>()),
    qmlAppEnginePtr_(nullptr),
    datasetPtr_(new Dataset),
    scene3dViewPtr_(nullptr),
    openedfilePath_(),
    isLoggingKlf_(false),
    isLoggingCsv_(false),
    filePath_(),
    isFileOpening_(false),
    isGPSAlive_(false),
    isUseGPS_(false),
    fixBlackStripesState_(false),
    fixBlackStripesForwardSteps_(0),
    fixBlackStripesBackwardSteps_(0),
    isActiveZeroing_(false),
    lastSub1_(0),
    lastSub2_(0),
    mosaicIndexProvider_(6200)
{
    qRegisterMetaType<uint8_t>("uint8_t");
    logger_.setDatasetPtr(datasetPtr_);
    createDeviceManagerConnections();
    createLinkManagerConnections();
    createControllers();
    createDatasetConnections();
    createDataHorizonConnections();

#ifdef FLASHER
    connect(&dev_flasher_, &DeviceFlasher::sendStepInfo, this, &Core::dev_flasher_rcv);
#endif

    createDataProcessor();
}

Core::~Core()
{
    destroyInternetManager();
    destroyDataProcessor();
}

MosaicIndexProvider *Core::getMosaicIndexProviderPtr()
{
    return &mosaicIndexProvider_;
}

void Core::setEngine(QQmlApplicationEngine *engine)
{
    qmlAppEnginePtr_ = engine;
    QObject::connect(qmlAppEnginePtr_, &QQmlApplicationEngine::objectCreated, this, &Core::UILoad, Qt::QueuedConnection);

    createInternetManager();

    qmlAppEnginePtr_->rootContext()->setContextProperty("BoatTrackControlMenuController",       boatTrackControlMenuController_.get());
    qmlAppEnginePtr_->rootContext()->setContextProperty("NavigationArrowControlMenuController", navigationArrowControlMenuController_.get());
    qmlAppEnginePtr_->rootContext()->setContextProperty("BottomTrackControlMenuController",     bottomTrackControlMenuController_.get());
    qmlAppEnginePtr_->rootContext()->setContextProperty("IsobathsViewControlMenuController",    isobathsViewControlMenuController_.get());
    qmlAppEnginePtr_->rootContext()->setContextProperty("MosaicViewControlMenuController",      mosaicViewControlMenuController_.get());
    qmlAppEnginePtr_->rootContext()->setContextProperty("ImageViewControlMenuController",       imageViewControlMenuController_.get());
    qmlAppEnginePtr_->rootContext()->setContextProperty("MapViewControlMenuController",         mapViewControlMenuController_.get());
    qmlAppEnginePtr_->rootContext()->setContextProperty("PointGroupControlMenuController",      pointGroupControlMenuController_.get());
    qmlAppEnginePtr_->rootContext()->setContextProperty("PolygonGroupControlMenuController",    polygonGroupControlMenuController_.get());
    qmlAppEnginePtr_->rootContext()->setContextProperty("MpcFilterControlMenuController",       mpcFilterControlMenuController_.get());
    qmlAppEnginePtr_->rootContext()->setContextProperty("NpdFilterControlMenuController",       npdFilterControlMenuController_.get());
    qmlAppEnginePtr_->rootContext()->setContextProperty("Scene3DControlMenuController",         scene3dControlMenuController_.get());
    qmlAppEnginePtr_->rootContext()->setContextProperty("Scene3dToolBarController",             scene3dToolBarController_.get());
    qmlAppEnginePtr_->rootContext()->setContextProperty("UsblViewControlMenuController",        usblViewControlMenuController_.get());

    bool flasherState = false;
#ifdef FLASHER
    flasherState = true;
#endif

    qmlAppEnginePtr_->rootContext()->setContextProperty("FLASHER_STATE", flasherState);
}

Console* Core::getConsolePtr()
{
    return consolePtr_;
}

Dataset* Core::getDatasetPtr()
{
    return datasetPtr_;
}

DataProcessor* Core::getDataProcessorPtr() const
{
    return dataProcessor_;
}

DeviceManagerWrapper* Core::getDeviceManagerWrapperPtr() const
{
    return deviceManagerWrapperPtr_.get();
}

LinkManagerWrapper* Core::getLinkManagerWrapperPtr() const
{
    return linkManagerWrapperPtr_.get();
}

void Core::stopLinkManagerTimer() const
{
    emit linkManagerWrapperPtr_->sendStopTimer();
}

void Core::consoleInfo(QString msg)
{
    getConsolePtr()->put(QtMsgType::QtInfoMsg, msg);
}

void Core::consoleWarning(QString msg)
{
    getConsolePtr()->put(QtMsgType::QtWarningMsg, msg);
}

void Core::consoleProto(FrameParser &parser, bool isIn)
{
    QString str_mode;
    QString comment = "";

    switch (parser.type()) {
    case CONTENT:
        str_mode = "DATA";
        if (parser.resp()) {
            switch(parser.frame()[6]) {
            case respNone: comment = "[respNone]"; break;
            case respOk: comment = "[respOk]"; break;
            case respErrorCheck: comment = "[respErrorCheck]"; break;
            case respErrorPayload: comment = "[respErrorPayload]"; break;
            case respErrorID: comment = "[respErrorID]"; break;
            case respErrorVersion: comment = "[respErrorVersion]"; break;
            case respErrorType: comment = "[respErrorType]"; break;
            case respErrorKey: comment = "[respErrorKey]"; break;
            case respErrorRuntime: comment = "[respErrorRuntime]"; break;
            default:
                comment = QString("[resp %1]").arg((int)parser.frame()[6]);
                break;
            }
        }
        else {
            if (parser.id() == ID_EVENT) {
                comment = QString("Event ID %1").arg(*(uint32_t*)(&parser.frame()[10]));
            }
        }
        break;
    case SETTING:
        str_mode = "SET";
        break;
    case GETTING:
        str_mode = "GET";
        break;
    default:
        str_mode = "NAN";
        break;
    }

    QString str_dir;
    isIn ? str_dir = "-->> " : str_dir = "<<-- ";

    try {
        QString str_data = QByteArray((char*)parser.frame(), parser.frameLen()).toHex();

        consoleInfo(
            str_dir % "KG[" % QString::number(parser.route()) % "]: id "
            % QString::number(parser.id())
            % " v" % QString::number(parser.ver())
            % ", " % str_mode
            % ", len " % QString::number(parser.payloadLen())
            % "; " % comment
            % " [ " % str_data % " ]"
            );
    }
    catch(std::bad_alloc& ex) {
        qCritical().noquote() << __func__ << " --> " << ex.what();
    }
}


#ifdef SEPARATE_READING
void Core::openLogFile(const QString &filePath, bool isAppend, bool onCustomEvent)
{
    QString localfilePath = filePath;

    if (onCustomEvent) {
        fixFilePathString(localfilePath);
        filePath_ = localfilePath;
        emit filePathChanged();
    }

    linkManagerWrapperPtr_->closeOpenedLinks();
    removeLinkManagerConnections();

    QCoreApplication::processEvents(QEventLoop::AllEvents);

    if (!isAppend) {
        QMetaObject::invokeMethod(dataProcessor_, "setSuppressResults", Qt::QueuedConnection, Q_ARG(bool, true));
        resetDataProcessorConnections();
        datasetPtr_->resetDataset();
        dataHorizon_->clear();
        QMetaObject::invokeMethod(dataProcessor_, "clearProcessing", Qt::QueuedConnection);
        QMetaObject::invokeMethod(dataProcessor_, "setFilePath", Qt::QueuedConnection, Q_ARG(QString, localfilePath));
        setDataProcessorConnections();
    }

    if (scene3dViewPtr_) {
        if (!isAppend) {
            scene3dViewPtr_->clear();
        }
    }

    QStringList splitname = localfilePath.split(QLatin1Char('.'), Qt::SkipEmptyParts);
    if (splitname.size() > 1) {
        QString format = splitname.last();
        if (format.contains("xtf", Qt::CaseInsensitive)) {
            QFile file;
            QUrl url(localfilePath);
            url.isLocalFile() ? file.setFileName(url.toLocalFile()) : file.setFileName(url.toString());
            if (file.open(QIODevice::ReadOnly)) {
                openXTF(file.readAll());
            }
            return;
        }
    }

    tryOpenedfilePath_ = filePath;
    if (closeLogFile(true) && !fileIsCompleteOpened_) {
        return;
    }

    fileIsCompleteOpened_ = false;
    openedfilePath_ = filePath;

    datasetPtr_->setState(Dataset::DatasetState::kFile);
    QMetaObject::invokeMethod(dataProcessor_, "setIsOpeningFile", Qt::QueuedConnection, Q_ARG(bool, true));
    if (scene3dViewPtr_) {
        scene3dViewPtr_->setIsOpeningFile(true);
    }

    emit deviceManagerWrapperPtr_->sendOpenFile(localfilePath);
}

bool Core::closeLogFile(bool onOpen)
{
    if (isOpenedFile()) {
        QMetaObject::invokeMethod(dataProcessor_, "prepareForFileClose", Qt::BlockingQueuedConnection, Q_ARG(int, 1500));
        if (!onOpen) {
            resetDataProcessorConnections();
        }
        emit sendCloseLogFile(onOpen ? !tryOpenedfilePath_.isEmpty() : false);
        openedfilePath_.clear();

        if (datasetPtr_) {
            datasetPtr_->resetDataset();
            dataHorizon_->clear();
        }

        QMetaObject::invokeMethod(dataProcessor_, "clearProcessing", Qt::BlockingQueuedConnection);

        if (scene3dViewPtr_) {
            scene3dViewPtr_->clear();
            scene3dViewPtr_->getNavigationArrowPtr()->resetPositionAndAngle();
        }
        if (!onOpen) {
            setDataProcessorConnections();
            createLinkManagerConnections();
            linkManagerWrapperPtr_->openClosedLinks();
            QMetaObject::invokeMethod(dataProcessor_, "setSuppressResults", Qt::QueuedConnection, Q_ARG(bool, false));
            QMetaObject::invokeMethod(dataProcessor_, "setIsOpeningFile", Qt::QueuedConnection, Q_ARG(bool, false));
            if (scene3dViewPtr_) {
                scene3dViewPtr_->setIsOpeningFile(false);
            }
        }

        return true;
    }
    return false;
}

void Core::onFileStartOpening()
{
    qDebug() << "file start opening!";
    isFileOpening_ = true;
    emit sendIsFileOpening();
    dataHorizon_->setIsFileOpening(isFileOpening_);

    if (scene3dViewPtr_) {
        scene3dViewPtr_->forceUpdateDatasetLlaRef();
    }
}

void Core::onFileOpened()
{
    qDebug() << "file opened!";
    QMetaObject::invokeMethod(dataProcessor_, "setSuppressResults", Qt::QueuedConnection, Q_ARG(bool, false));
    QMetaObject::invokeMethod(dataProcessor_, "setIsOpeningFile", Qt::QueuedConnection, Q_ARG(bool, false));
    setTimelinePosition(1.0);
    if (scene3dViewPtr_) {
        scene3dViewPtr_->setIsOpeningFile(false);
    }

    tryOpenedfilePath_.clear();
    fileIsCompleteOpened_ = true;

    if (scene3dViewPtr_) {
        scene3dViewPtr_->forceUpdateDatasetLlaRef();
    };
}

void Core::onFileReadEnough()
{
    QMetaObject::invokeMethod(dataProcessor_, "setSuppressResults", Qt::QueuedConnection, Q_ARG(bool, false));
    datasetPtr_->setRefPositionByFirstValid();
    // datasetPtr_->usblProcessing();
    if (scene3dViewPtr_) {
        scene3dViewPtr_->forceUpdateDatasetLlaRef();
        //scene3dViewPtr_->addPoints(datasetPtr_->beaconTrack(), QColor(255, 0, 0), 10);
        //scene3dViewPtr_->addPoints(datasetPtr_->beaconTrack1(), QColor(0, 255, 0), 10);
    }

    onChannelsUpdated();
}

void Core::onFileOpenBreaked(bool onOpen)
{
    fileIsCompleteOpened_ = false;
    if (datasetPtr_) {
        datasetPtr_->resetDataset();
        dataHorizon_->clear();
    }

    QMetaObject::invokeMethod(dataProcessor_, "clearProcessing", Qt::QueuedConnection);
    QMetaObject::invokeMethod(dataProcessor_, "setSuppressResults", Qt::QueuedConnection, Q_ARG(bool, false));
    QMetaObject::invokeMethod(dataProcessor_, "setIsOpeningFile", Qt::QueuedConnection, Q_ARG(bool, false));
    if (scene3dViewPtr_) {
        scene3dViewPtr_->setIsOpeningFile(false);
    }

    if (scene3dViewPtr_) {
        scene3dViewPtr_->clear();
        scene3dViewPtr_->getNavigationArrowPtr()->resetPositionAndAngle();
    }
    if (onOpen && !tryOpenedfilePath_.isEmpty()) {
        openLogFile(tryOpenedfilePath_, false, false);
        tryOpenedfilePath_.clear();
    }
}
#else
void Core::openLogFile(const QString& filePath, bool isAppend, bool onCustomEvent)
{
    isFileOpening_ = true;
    emit sendIsFileOpening();

    QTimer::singleShot(15, this, [this, filePath, isAppend, onCustomEvent]() ->void { // 15 ms delay
        QString localfilePath = filePath;

        if (onCustomEvent) {
            fixFilePathString(localfilePath);
            filePath_ = localfilePath;
            emit filePathChanged();
        }

        linkManagerWrapperPtr_->closeOpenedLinks();
        removeLinkManagerConnections();

        QCoreApplication::processEvents(QEventLoop::AllEvents);

        if (!isAppend) {
            QMetaObject::invokeMethod(dataProcessor_, "setSuppressResults", Qt::QueuedConnection, Q_ARG(bool, true));
            resetDataProcessorConnections();
            datasetPtr_->resetDataset();
            dataHorizon_->clear();
            QMetaObject::invokeMethod(dataProcessor_, "clearProcessing", Qt::QueuedConnection);
            QMetaObject::invokeMethod(dataProcessor_, "setFilePath", Qt::QueuedConnection, Q_ARG(QString, localfilePath));
            setDataProcessorConnections();
            dataHorizon_->setIsFileOpening(isFileOpening_);
        }

        if (scene3dViewPtr_) {
            if (!isAppend) {
                scene3dViewPtr_->clear();
            }
        }

        QMetaObject::invokeMethod(dataProcessor_, "setIsOpeningFile", Qt::QueuedConnection, Q_ARG(bool, true));
        if (scene3dViewPtr_) {
            scene3dViewPtr_->setIsOpeningFile(true);
        }

        QStringList splitname = localfilePath.split(QLatin1Char('.'), Qt::SkipEmptyParts);

        if (splitname.size() > 1) {
            QString format = splitname.last();
            if (format.contains("xtf", Qt::CaseInsensitive)) {
                QFile file;
                QUrl url(localfilePath);
                url.isLocalFile() ? file.setFileName(url.toLocalFile()) : file.setFileName(url.toString());
                if (file.open(QIODevice::ReadOnly)) {
                    openXTF(file.readAll());
                }

                openedfilePath_ = localfilePath;

                onFileStopsOpening();

                QMetaObject::invokeMethod(dataProcessor_, "setIsOpeningFile", Qt::QueuedConnection, Q_ARG(bool, false));
                if (scene3dViewPtr_) {
                    scene3dViewPtr_->setIsOpeningFile(false);
                }

                return;
            }
        }

        datasetPtr_->setState(Dataset::DatasetState::kFile);

        emit deviceManagerWrapperPtr_->sendOpenFile(localfilePath);

        openedfilePath_ = localfilePath;

        if (scene3dViewPtr_) {
            scene3dViewPtr_->fitAllInView();
        }
        datasetPtr_->setRefPositionByFirstValid();
        datasetPtr_->usblProcessing();

        if (scene3dViewPtr_) {
            scene3dViewPtr_->addPoints(datasetPtr_->beaconTrack(), QColor(255, 0, 0), 10);
            scene3dViewPtr_->addPoints(datasetPtr_->beaconTrack1(), QColor(0, 255, 0), 10);
        }

        onChannelsUpdated();
    });
}

bool Core::closeLogFile()
{
    // qDebug() << "Core::closeLogFile()";
    const bool wasOpened = isOpenedFile();
    QMetaObject::invokeMethod(dataProcessor_, "prepareForFileClose", Qt::BlockingQueuedConnection, Q_ARG(int, 1500));
    if (wasOpened) {
        resetDataProcessorConnections();
    }
    if (datasetPtr_) {
        datasetPtr_->resetRenderBuffers();
    }
    if (scene3dViewPtr_) {
        scene3dViewPtr_->clear();
        scene3dViewPtr_->getNavigationArrowPtr()->resetPositionAndAngle();
    }
    dataHorizon_->clear();
    QMetaObject::invokeMethod(dataProcessor_, "clearProcessing", Qt::BlockingQueuedConnection);

    if (!wasOpened) {
        QMetaObject::invokeMethod(dataProcessor_, "setSuppressResults", Qt::QueuedConnection, Q_ARG(bool, false));
        return false;
    }

    if (datasetPtr_) {
        datasetPtr_->resetDataset();
    }
    emit deviceManagerWrapperPtr_->sendCloseFile();
    createLinkManagerConnections();
    openedfilePath_.clear();
    linkManagerWrapperPtr_->openClosedLinks();
    if (wasOpened) {
        setDataProcessorConnections();
    }
    QMetaObject::invokeMethod(dataProcessor_, "setSuppressResults", Qt::QueuedConnection, Q_ARG(bool, false));

    return true;
}

void Core::onFileOpened()
{
    qDebug() << "file opened!";

    QMetaObject::invokeMethod(dataProcessor_, "setSuppressResults", Qt::QueuedConnection, Q_ARG(bool, false));
    QMetaObject::invokeMethod(dataProcessor_, "setIsOpeningFile", Qt::QueuedConnection, Q_ARG(bool, false));
    setTimelinePosition(1.0);
    if (scene3dViewPtr_) {
        scene3dViewPtr_->setIsOpeningFile(false);
    }
}
#endif

void Core::onRequestClearing()
{
    if (isFileOpening_) {
        return;
    }

    datasetPtr_->softResetDataset();

    QTimer::singleShot(50, this, [this]() ->void {
        resetDataProcessorConnections();

        QMetaObject::invokeMethod(dataProcessor_, "clearProcessing", Qt::QueuedConnection);

        if (scene3dViewPtr_) {
            scene3dViewPtr_->clear();
            scene3dViewPtr_->getNavigationArrowPtr()->resetPositionAndAngle();
        }

        dataHorizon_->clear();
        setDataProcessorConnections();
    });
}

bool Core::openXTF(const QByteArray& data)
{
    datasetPtr_->setState(Dataset::DatasetState::kFile);
    converterXtf_.toDataset(data, getDatasetPtr());

    consoleInfo("XTF note:" + QString(converterXtf_.header.NoteString));
    consoleInfo("XTF programm name:" + QString(converterXtf_.header.RecordingProgramName));
    consoleInfo("XTF sonar name:" + QString(converterXtf_.header.SonarName));

    const QVector<DatasetChannel> channelList = datasetPtr_->channelsList();
    if (channelList.size() < 2) {
        return false;
    }

    auto linkNames = getLinkNames();
    QString fChName;
    QString sChName;
    if (linkNames.contains(channelList.at(0).channelId_.uuid)) {
        fChName = channelList.at(0).portName_;
    }
    if (linkNames.contains(channelList.at(1).channelId_.uuid)) {
        sChName = channelList.at(1).portName_;
    }

    if (!plot2dList_.isEmpty() && plot2dList_.at(0) && channelList.size() >= 2) {
        plot2dList_.at(0)->setDataChannel(false, channelList[0].channelId_, channelList[0].subChannelId_, fChName, channelList[1].channelId_, channelList[1].subChannelId_, sChName);
        plot2dList_.at(0)->plotUpdate();
    }

    for (int i = 0; i < plot2dList_.size(); i++) {
        if (plot2dList_.at(i) != NULL && i < channelList.size()) {
            if (i == 0) {
                plot2dList_.at(i)->setDataChannel(false, channelList[0].channelId_, channelList[0].subChannelId_, fChName, channelList[1].channelId_, channelList[1].subChannelId_, sChName);
                plot2dList_.at(i)->plotUpdate();
            }
        }
    }

    if (syncLoupePlot3dPtr_ && !channelList.isEmpty()) {
        if (channelList.size() >= 2) {
            const QString loupeFirstName = fChName.isEmpty() ? channelList[0].portName_ : fChName;
            const QString loupeSecondName = sChName.isEmpty() ? channelList[1].portName_ : sChName;
            syncLoupePlot3dPtr_->setDataChannel(false,
                                                channelList[0].channelId_, channelList[0].subChannelId_, loupeFirstName,
                                                channelList[1].channelId_, channelList[1].subChannelId_, loupeSecondName);
        }
        else {
            const QString loupeChannelName = fChName.isEmpty() ? channelList[0].portName_ : fChName;
            syncLoupePlot3dPtr_->setDataChannel(false, channelList[0].channelId_, channelList[0].subChannelId_, loupeChannelName);
        }
        syncLoupePlot3dPtr_->plotUpdate();
    }

    return true;
}

bool Core::openCSV(QString name, int separatorType, int firstRow, int colTime, bool isUtcTime, int colLat, int colLon, int colAltitude, int colNorth, int colEast, int colUp)
{
    QFile file;
    QUrl url(name);
    url.isLocalFile() ? file.setFileName(url.toLocalFile()) : file.setFileName(url.toString());

    if (!file.open(QIODevice::ReadOnly))
        return false;

    QString separator("");
    switch (separatorType) {
    case 0: separator = ","; break;
    case 1: separator = "	"; break;
    case 2: separator = " "; break;
    case 3: separator = ";"; break;
    default: separator = QString((char)separatorType); break;
    }

    QList<Position> track;

    QTextStream in(&file);
    int skip_rows = firstRow - 1;

    while (!in.atEnd()) {
        QString row = in.readLine();
        if (skip_rows > 0) {
            skip_rows--;
            continue;
        }

        if (row[0] == '%' || row[0] == '#')
            continue;

        QStringList columns = row.split(separator);
        track.append(Position());

        if (colTime > 0 && (colTime-1 < columns.size())) {
            int year = -1, month = -1, day = -1, hour = -1, minute = -1;
            double sec = -1;
            columns[colTime-1].replace(QLatin1Char('/'), QLatin1Char('-'));
            QStringList date_time = columns[colTime-1].split(' ');
            QString date, time;

            if (date_time.size() > 0) {
                if (date_time[0].contains('-'))
                    date = date_time[0];
            }
            if (date_time.size() == 2) {
                if (date_time[1].contains(':'))
                    time = date_time[1];
            }
            else if (date_time.size() == 1) {
                if (colTime < columns.size()) {
                    if (columns[colTime].contains(':')) {
                        time = columns[colTime];
                    }
                }
            }

            QStringList data_sep = date.split('-');
            if (data_sep.size() >= 3) {
                year = data_sep[0].toInt();
                month = data_sep[1].toInt();
                day = data_sep[2].toInt();
            }
            QStringList time_sep = time.split(':');
            if (time_sep.size() >= 3) {
                hour = time_sep[0].toInt();
                minute = time_sep[1].toInt();
                sec = time_sep[2].replace(QLatin1Char(','), QLatin1Char('.')).toDouble();
            }
            if (year >= 0 && month >= 0 && day >= 0 && hour >= 0 && minute >= 0 && sec >= 0) {
                int sec_int = (int)sec;
                double nano_sec = (sec - sec_int)*1e9;
                track.last().time = DateTime(year, month, day, hour, minute, sec_int, round(nano_sec));
                if (!isUtcTime) {
                    track.last().time.addSecs(-18);
                }

            }
        }

        if(colLat > 0 && colLat-1 < columns.size())
            track.last().lla.latitude = columns[colLat-1].replace(QLatin1Char(','), QLatin1Char('.')).toDouble();
        if(colLon > 0 && colLon-1 < columns.size())
            track.last().lla.longitude = columns[colLon-1].replace(QLatin1Char(','), QLatin1Char('.')).toDouble();
        if(colAltitude > 0 && colAltitude-1 < columns.size())
            track.last().lla.altitude = columns[colAltitude-1].replace(QLatin1Char(','), QLatin1Char('.')).toDouble();
        if(colNorth > 0 && colNorth-1 < columns.size())
            track.last().ned.n = columns[colNorth-1].replace(QLatin1Char(','), QLatin1Char('.')).toDouble();
        if(colEast > 0 && colEast-1 < columns.size())
            track.last().ned.e = columns[colEast-1].replace(QLatin1Char(','), QLatin1Char('.')).toDouble();
        if(colUp > 0 && colUp-1 < columns.size())
            track.last().ned.d = -columns[colUp-1].replace(QLatin1Char(','), QLatin1Char('.')).toDouble();;
    }

    datasetPtr_->mergeGnssTrack(track);

    return true;
}

bool Core::openProxy(const QString& address, const int port, bool isTcp)
{
    Q_UNUSED(address);
    Q_UNUSED(port);
    Q_UNUSED(isTcp);

    return false;
}

bool Core::closeProxy()
{
    return false;
}

bool Core::upgradeFW(const QString& name, QObject* dev)
{
    QUrl url(name);
    QFile file(url.isLocalFile() ? url.toLocalFile() : name);

    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    if (auto* devQProp = dynamic_cast<DevQProperty*>(dev); devQProp) {
        devQProp->sendUpdateFW(file.readAll());
    }

    return true;
}

void Core::upgradeChanged(int progressStatus)
{
    if(progressStatus == DevDriver::successUpgrade) {
        //        restoreBaudrate();
    }
}

bool Core::getKlfLogging() const
{
    return isLoggingKlf_;
}

void Core::setKlfLogging(bool isLogging)
{
    if (isLogging == this->getKlfLogging())
        return;
    bool success = true;
    if (isLogging) {
        success = logger_.startNewKlfLog();
        if (!success) {
            consoleWarning("KLF logging not started");
        }
    } else {
        logger_.stopKlfLogging();
    }
    isLoggingKlf_ = isLogging && success;

    emit loggingKlfChanged();
}

bool Core::getFixBlackStripesState() const
{
    return fixBlackStripesState_;
}

int Core::getFixBlackStripesForwardSteps() const
{
    return fixBlackStripesForwardSteps_;
}

int Core::getFixBlackStripesBackwardSteps() const
{
    return fixBlackStripesBackwardSteps_;
}

void Core::setFixBlackStripesState(bool state)
{
    fixBlackStripesState_ = state;

    if (datasetPtr_) {
        datasetPtr_->setFixBlackStripesState(state);
    }
}

void Core::setFixBlackStripesForwardSteps(int val)
{
    fixBlackStripesForwardSteps_ = val;

    if (datasetPtr_) {
        datasetPtr_->setFixBlackStripesForwardSteps(val);
    }
}

void Core::setFixBlackStripesBackwardSteps(int val)
{
    fixBlackStripesBackwardSteps_ = val;

    if (datasetPtr_) {
        datasetPtr_->setFixBlackStripesBackwardSteps(val);
    }
}

void Core::setBottomTrackRealtimeFromSettings(bool state)
{
    if (dataProcessor_) {
        QMetaObject::invokeMethod(dataProcessor_, "setUpdateBottomTrackFromSettings", Qt::QueuedConnection, Q_ARG(bool, state));
    }
}

bool Core::getCsvLogging() const
{
    return isLoggingCsv_;
}

void Core::setCsvLogging(bool isLogging)
{
    if (isLogging == this->getCsvLogging())
        return;
    bool success = true;
    if (isLogging) {
        success = logger_.startNewCsvLog();
        if (!success) {
            consoleWarning("CSV logging not started");
        }
    } else {
        logger_.stopCsvLogging();
    }
    isLoggingCsv_ = isLogging && success;
}

bool Core::getUseGPS() const
{
    return isUseGPS_;
}

void Core::setUseGPS(bool state)
{
    //qDebug() << "Core::setUseGPS" << state;
    isUseGPS_ = state;
    QMetaObject::invokeMethod(deviceManagerWrapperPtr_->getWorker(), "setUseGPS", Qt::QueuedConnection, Q_ARG(bool, isUseGPS_));
}

void Core::setNeedForceZooming(bool state)
{
    if (needForceZooming_ == state) {
        return;
    }

    needForceZooming_ = state;
    emit needForceZoomingChanged();
}

bool Core::exportComplexToCSV(QString file_path) {
    QString export_file_name = isOpenedFile() ? openedfilePath_.section('/', -1).section('.', 0, 0) : QDateTime::currentDateTime().toString("yyyy.MM.dd_hh:mm:ss").replace(':', '.');
    logger_.creatExportStream(file_path + "/" + export_file_name + ".csv");

    //auto ch_list = datasetPtr_->channelsList();
    // _dataset->setRefPosition(1518);

    for(int i = 0; i < datasetPtr_->size(); i++) {
        Epoch* epoch = datasetPtr_->fromIndex(i);

        if(epoch == NULL) { continue; }

        if(epoch->isComplexSignalAvail()) {
            ComplexSignals& sigs = epoch->complexSignals();

            for (auto dev_i = sigs.cbegin(), end = sigs.cend(); dev_i != end; ++dev_i) {
                QMap<int, QVector<ComplexSignal>> dev = dev_i.value();

                for (auto group_i = dev.cbegin(), end = dev.cend(); group_i != end; ++group_i) {
                    int group_id  = group_i.key();
                    QVector<ComplexSignal> sig = group_i.value();
                    int ch_size = sig.size();

                    for(int ch_i = 0; ch_i < ch_size; ch_i++) {
                        int ch_num = ch_i + group_id*32;

                        ComplexF* data = sig[ch_i].data.data();
                        int data_size = sig[ch_i].data.size();

                        QString row_data;
                        row_data.append(QString("%1,%2").arg(i).arg(ch_num));
                        row_data.append(QString(",%1").arg(sig[ch_i].globalOffset));
                        row_data.append(QString(",%1").arg(sig[ch_i].sampleRate));

                        if(data != NULL && data_size > 0) {
                            for(int ci = 0; ci < data_size; ci++) {
                                row_data.append(QString(",%1,%2").arg(data[ci].real).arg(data[ci].imag));
                            }
                        }

                        row_data.append("\n");
                        logger_.dataExport(row_data);
                    }
                }
            }
        }
    }

    logger_.endExportStream();

    return true;
}

bool Core::exportUSBLToCSV(QString filePath)
{
    QString export_file_name = isOpenedFile() ? openedfilePath_.section('/', -1).section('.', 0, 0) : QDateTime::currentDateTime().toString("yyyy.MM.dd_hh:mm:ss").replace(':', '.');

    logger_.creatExportStream(filePath + "/" + export_file_name + ".csv");
    //QMap<int, DatasetChannel> ch_list = datasetPtr_->channelsList();
    //Q_UNUSED(ch_list);
    // _dataset->setRefPosition(1518);

    logger_.dataExport("epoch,yaw,pitch,roll,north,east,ping_counter,carrier_counter,snr,azimuth_deg,elevation_deg,distance_m\n");

    for (int i = 0; i < datasetPtr_->size(); i += 1) {
        Epoch* epoch = datasetPtr_->fromIndex(i);

        if (epoch == NULL)
            continue;

        NED boatPosNed = epoch->getPositionGNSS().ned;

        // pos.ned.isCoordinatesValid() && epoch->isAttAvail() &&
        if( epoch->isUsblSolutionAvailable()) {
            QString row_data;

            row_data.append(QString("%1").arg(i));
            row_data.append(QString(",%1,%2,%3").arg(epoch->yaw()).arg(epoch->pitch()).arg(epoch->roll()));
            row_data.append(QString(",%1,%2").arg(boatPosNed.n).arg(boatPosNed.e));
            row_data.append(QString(",%1,%2,%3").arg(epoch->usblSolution().ping_counter).arg(epoch->usblSolution().carrier_counter).arg(epoch->usblSolution().snr));
            row_data.append(QString(",%1,%2,%3").arg(epoch->usblSolution().azimuth_deg).arg(epoch->usblSolution().elevation_deg).arg(epoch->usblSolution().distance_m));

            row_data.append("\n");
            logger_.dataExport(row_data);
        }
    }

    logger_.endExportStream();

    return true;
}

bool Core::exportPlotAsCVS(QString filePath, const ChannelId& channelId, float decimation)
{
    QString export_file_name = isOpenedFile() ? openedfilePath_.section('/', -1).section('.', 0, 0) : QDateTime::currentDateTime().toString("yyyy.MM.dd_hh:mm:ss").replace(':', '.');
    logger_.creatExportStream(filePath + "/" + export_file_name + ".csv");

    bool meas_nbr = true;
    bool event_id = true;
    bool rangefinder = false;
    bool bottom_depth = true;
    bool pos_lat_lon = true;
    bool pos_time = true;

    bool external_pos_lla = true;
    bool external_pos_neu = true;
    bool sonar_height = true;
    bool bottom_height = true;

    bool ext_pos_lla_find = false;
    bool ext_pos_ned_find = false;

    bool contactInfo = true;
    bool contactDistance = true;

    int row_cnt = datasetPtr_->size();

    auto btP = datasetPtr_->getBottomTrackParam();
    datasetPtr_->setChannelOffset(channelId, btP.offset.x, btP.offset.y, btP.offset.z);
    datasetPtr_->spatialProcessing();

    for (int i = 0; i < row_cnt; i++) {
        Epoch* epoch = datasetPtr_->fromIndex(i);

        Position position = epoch->getExternalPosition();
        ext_pos_lla_find |= position.lla.isValid();
        ext_pos_ned_find |= position.ned.isValid();
    }

    if (meas_nbr)
        logger_.dataExport("Number,");

    if (event_id) {
        logger_.dataExport("Event UNIX,");
        logger_.dataExport("Event timestamp,");
        logger_.dataExport("Event ID,");
    }

    if (rangefinder)
        logger_.dataExport("Rangefinder,");

    if (bottom_depth)
        logger_.dataExport("Beam distance,");

    if (pos_lat_lon) {
        logger_.dataExport("Latitude,");
        logger_.dataExport("Longitude,");

        if (pos_time) {
            logger_.dataExport("GNSS UTC Date,");
            logger_.dataExport("GNSS UTC Time,");
        }
    }

    if (external_pos_lla && ext_pos_lla_find) {
        logger_.dataExport("ExtLatitude,");
        logger_.dataExport("ExtLongitude,");
        logger_.dataExport("ExtAltitude,");
    }

    if (external_pos_neu && ext_pos_ned_find) {
        logger_.dataExport("ExtNorth,");
        logger_.dataExport("ExtEast,");
        logger_.dataExport("ExtHeight,");
    }

    if (sonar_height)
        logger_.dataExport("SonarHeight,");

    if (bottom_height)
        logger_.dataExport("BottomHeight,");

    if (contactInfo) {
        logger_.dataExport("ContactTitle,");
    }
    if (contactDistance) {
        logger_.dataExport("ContactDistance");
    }

    logger_.dataExport("\n");

    int prev_timestamp = 0;
    int prev_unix = 0;
    int prev_event_id = 0;
    float prev_dist_proc = 0;
    double prev_lat = 0, prev_lon = 0;

    float decimation_m = decimation;
    float decimation_path = 0;
    LLARef lla_ref;
    NED last_pos_ned;

    for (int i = 0; i < row_cnt; i++) {
        Epoch* epoch = datasetPtr_->fromIndex(i);

        if (!epoch->contact_.isValid()) {
            if (decimation_m > 0) {
                if (!epoch->isPosAvail())
                    continue;

                Position boatPos = epoch->getPositionGNSS();

                if (boatPos.lla.isCoordinatesValid()) {
                    if (!lla_ref.isInit) {
                        lla_ref = LLARef(boatPos.lla);
                        boatPos.LLA2NED(&lla_ref);
                        last_pos_ned = boatPos.ned;
                    }
                    else {
                        boatPos.LLA2NED(&lla_ref);
                        float dif_n = boatPos.ned.n - last_pos_ned.n;
                        float dif_e = boatPos.ned.e - last_pos_ned.e;
                        last_pos_ned = boatPos.ned;
                        decimation_path += sqrtf(dif_n*dif_n + dif_e*dif_e);
                        if(decimation_path < decimation_m)
                            continue;
                        decimation_path -= decimation_m;
                    }
                }
                else {
                    continue;
                }
            }
        }

        QString row_data;

        if (meas_nbr)
            row_data.append(QString("%1,").arg(i));

        if (event_id) {
            if (epoch->eventAvail()) {
                prev_timestamp = epoch->eventTimestamp();
                prev_event_id = epoch->eventID();
                prev_unix = epoch->eventUnix();
            }
            row_data.append(QString("%1,%2,%3,").arg(prev_unix).arg(prev_timestamp).arg(prev_event_id));
        }

        if (rangefinder)
            epoch->distAvail() ? row_data.append(QString("%1,").arg((float)epoch->rangeFinder())) : row_data.append("0,");

        if (bottom_depth) {
            prev_dist_proc = epoch->distProccesing(channelId);
            row_data.append(QString("%1,").arg((float)(prev_dist_proc)));
        }

        if (pos_lat_lon) {
            if (epoch->isPosAvail()) {
                prev_lat = epoch->lat();
                prev_lon = epoch->lon();
            }

            row_data.append(QString::number(prev_lat, 'f', 8));
            row_data.append(",");
            row_data.append(QString::number(prev_lon, 'f', 8));
            row_data.append(",");

            if (pos_time) {
                if (epoch->isPosAvail() && epoch->positionTimeUnix() != 0) {
                    DateTime time_epoch = *epoch->time();

                    DateTime* dt = epoch->time();
                    if (time_epoch.sec > 0) {
                        time_epoch.sec -= 18;
                        dt = &time_epoch;
                    }
                    // DateTime* dt = epoch->positionTime();
                    volatile tm t_sep = dt->getDateTime();
                    t_sep.tm_year += 1900;
                    t_sep.tm_mon += 1;

                    row_data.append(QString("%1-%2-%3").arg(t_sep.tm_year).arg(t_sep.tm_mon).arg(t_sep.tm_mday));
                    row_data.append(",");
                    row_data.append(QString("%1:%2:%3").arg(t_sep.tm_hour).arg(t_sep.tm_min).arg((double)t_sep.tm_sec+(double)dt->nanoSec/1e9));
                    row_data.append(",");
                }
                else {
                    row_data.append(",");
                    row_data.append(",");
                }
            }
        }

        Position position = epoch->getExternalPosition();

        if (external_pos_lla && ext_pos_lla_find) {
            row_data.append(QString::number(position.lla.latitude, 'f', 10));
            row_data.append(",");
            row_data.append(QString::number(position.lla.longitude, 'f', 10));
            row_data.append(",");
            row_data.append(QString::number(position.lla.altitude, 'f', 3));
            row_data.append(",");
        }

        if (external_pos_neu && ext_pos_ned_find) {
            row_data.append(QString::number(position.ned.n, 'f', 10));
            row_data.append(",");
            row_data.append(QString::number(position.ned.e, 'f', 10));
            row_data.append(",");
            row_data.append(QString::number(-position.ned.d, 'f', 3));
            row_data.append(",");
        }

        Epoch::Echogram* sensor = epoch->chart(channelId);

        if (sonar_height) {
            if (sensor != NULL && isfinite(sensor->sensorPosition.ned.d)) {
                row_data.append(QString::number(-sensor->sensorPosition.ned.d, 'f', 3));
            }
            else if (sensor != NULL && isfinite(sensor->sensorPosition.lla.altitude)) {
                row_data.append(QString::number(sensor->sensorPosition.lla.altitude, 'f', 3));
            }
            row_data.append(",");
        }

        if (bottom_height) {
            if(sensor != NULL && isfinite(sensor->bottomProcessing.bottomPoint.ned.d)) {
                row_data.append(QString::number(-sensor->bottomProcessing.bottomPoint.ned.d, 'f', 3));
            }
            else if (sensor != NULL && isfinite(sensor->bottomProcessing.bottomPoint.lla.altitude)) {
                row_data.append(QString::number(sensor->bottomProcessing.bottomPoint.lla.altitude, 'f', 3));
            }
            row_data.append(",");
        }

        auto& contact = epoch->contact_;
        if (contact.isValid()) {
            if (contactInfo) {
                row_data.append(contact.info);
                row_data.append(",");
            }
            if (contactDistance) {
                row_data.append(QString::number(contact.echogramDistance, 'f', 4));
            }
        }

        row_data.append("\n");
        logger_.dataExport(row_data);
    }

    logger_.endExportStream();

    return true;
}

bool Core::exportPlotAsXTF(QString filePath)
{
    if (plot2dList_.empty()) {
        return false;
    }

    QString export_file_name = isOpenedFile() ? openedfilePath_.section('/', -1).section('.', 0, 0) : QDateTime::currentDateTime().toString("yyyy.MM.dd_hh:mm:ss").replace(':', '.');
    logger_.creatExportStream(filePath + "/_" + export_file_name + ".xtf");

    auto ch1 = plot2dList_[0]->plotDatasetChannel();
    auto subCh1 = plot2dList_[0]->plotDatasetSubChannel();
    auto ch2 = plot2dList_[0]->plotDatasetChannel2();
    auto subCh2 = plot2dList_[0]->plotDatasetSubChannel2();

    QByteArray data_export = converterXtf_.toXTF(getDatasetPtr(), ch1, subCh1, ch2, subCh2);
    logger_.dataByteExport(data_export);
    logger_.endExportStream();
    return true;
}

void Core::setPlotStartLevel(int level)
{
    for (int i = 0; i < plot2dList_.size(); i++) {
        if (plot2dList_.at(i) != NULL) {
            plot2dList_.at(i)->setEchogramLowLevel(level);
        }
    }
    if (syncLoupePlot3dPtr_) {
        syncLoupePlot3dPtr_->setEchogramLowLevel(level);
    }
}

void Core::setPlotStopLevel(int level)
{
    for (int i = 0; i < plot2dList_.size(); i++) {
        if (plot2dList_.at(i) != NULL)
            plot2dList_.at(i)->setEchogramHightLevel(level);
    }
    if (syncLoupePlot3dPtr_) {
        syncLoupePlot3dPtr_->setEchogramHightLevel(level);
    }
}

void Core::setTimelinePosition(double position)
{
    for (int i = 0; i < plot2dList_.size(); i++) {
        if (plot2dList_.at(i) != NULL)
            plot2dList_.at(i)->setTimelinePosition(position);
    }
}

void Core::resetAim()
{
    for (int i = 0; i < plot2dList_.size(); i++) {
        if (plot2dList_.at(i) != NULL)
            plot2dList_.at(i)->resetAim();
    }
}

void Core::UILoad(QObject* object, const QUrl& url)
{
    Q_UNUSED(url)

    loadLLARefFromSettings();

#if !defined(Q_OS_ANDROID)
    HotkeysManager hotkeysManager;
    auto hotkeysMap = hotkeysManager.loadHotkeysMapping();
    auto hotkeysVariant = HotkeysManager::toVariantMap(hotkeysMap);
    qmlAppEnginePtr_->rootContext()->setContextProperty("hotkeysMapScan", hotkeysVariant);
#endif

    scene3dViewPtr_ = object->findChild<GraphicsScene3dView*> ();
    plot2dList_.clear();
    syncLoupePlot3dPtr_.clear();
    const auto allPlots = object->findChildren<qPlot2D*>();
    for (auto* plot : allPlots) {
        if (!plot) {
            continue;
        }
        if (plot->objectName() == QStringLiteral("syncLoupe3DPlot")) {
            syncLoupePlot3dPtr_ = plot;
            continue;
        }
        plot2dList_.append(plot);
    }
    scene3dViewPtr_->setDataset(datasetPtr_);
    scene3dViewPtr_->setDataProcessorPtr(dataProcessor_);
    datasetPtr_->setScene3D(scene3dViewPtr_);

    for (int i = 0; i < plot2dList_.size(); i++) {
        if (plot2dList_.at(i) != NULL) {
            plot2dList_.at(i)->setPlot(datasetPtr_);
            plot2dList_.at(i)->setDataProcessor(dataProcessor_);
            scene3dViewPtr_->bottomTrack()->installEventFilter(plot2dList_.at(i));
            scene3dViewPtr_->getBoatTrackPtr()->installEventFilter(plot2dList_.at(i));
            scene3dViewPtr_->getContactsPtr()->installEventFilter(plot2dList_.at(i));
            plot2dList_.at(i)->installEventFilter(scene3dViewPtr_->bottomTrack().get());
            plot2dList_.at(i)->installEventFilter(scene3dViewPtr_->getBoatTrackPtr().get());
            plot2dList_.at(i)->installEventFilter(scene3dViewPtr_->getContactsPtr().get());
        }
    }

    if (syncLoupePlot3dPtr_) {
        syncLoupePlot3dPtr_->setPlot(datasetPtr_);
        syncLoupePlot3dPtr_->setDataProcessor(dataProcessor_);
    }

    //if(m_scene3dView){
    //    QObject::connect(mpBottomTrackProvider.get(), &BottomTrackProvider::bottomTrackChanged,
    //        [this](QVector<QVector3D>& data){
    //            m_scene3dView->bottomTrack()->setData(data, GL_LINE_STRIP);
    //        });
    //}

    scene3dViewPtr_->setQmlRootObject(object);
    scene3dViewPtr_->setQmlAppEngine(qmlAppEnginePtr_);

    boatTrackControlMenuController_->setQmlEngine(object);
    boatTrackControlMenuController_->setGraphicsSceneView(scene3dViewPtr_);

    navigationArrowControlMenuController_->setQmlEngine(object);
    navigationArrowControlMenuController_->setGraphicsSceneView(scene3dViewPtr_);

    bottomTrackControlMenuController_->setQmlEngine(object);
    bottomTrackControlMenuController_->setGraphicsSceneView(scene3dViewPtr_);

    isobathsViewControlMenuController_->setQmlEngine(object);
    isobathsViewControlMenuController_->setDataProcessorPtr(dataProcessor_);
    isobathsViewControlMenuController_->setGraphicsSceneView(scene3dViewPtr_);

    mosaicViewControlMenuController_->setQmlEngine(object);
    mosaicViewControlMenuController_->setDataProcessorPtr(dataProcessor_);
    mosaicViewControlMenuController_->setGraphicsSceneView(scene3dViewPtr_);

    imageViewControlMenuController_->setQmlEngine(object);
    imageViewControlMenuController_->setGraphicsSceneView(scene3dViewPtr_);

    mapViewControlMenuController_->setQmlEngine(object);
    mapViewControlMenuController_->setGraphicsSceneView(scene3dViewPtr_);

    npdFilterControlMenuController_->setQmlEngine(object);
    npdFilterControlMenuController_->setGraphicsSceneView(scene3dViewPtr_);

    mpcFilterControlMenuController_->setQmlEngine(object);
    mpcFilterControlMenuController_->setGraphicsSceneView(scene3dViewPtr_);

    pointGroupControlMenuController_->setQmlEngine(object);
    pointGroupControlMenuController_->setGraphicsSceneView(scene3dViewPtr_);

    polygonGroupControlMenuController_->setQmlEngine(object);
    polygonGroupControlMenuController_->setGraphicsSceneView(scene3dViewPtr_);

    scene3dToolBarController_->setQmlEngine(object);
    scene3dToolBarController_->setDataProcessorPtr(dataProcessor_);
    scene3dToolBarController_->setGraphicsSceneView(scene3dViewPtr_);

    scene3dControlMenuController_->setQmlEngine(object);
    scene3dControlMenuController_->setGraphicsSceneView(scene3dViewPtr_);

    usblViewControlMenuController_->setQmlEngine(object);
    usblViewControlMenuController_->setGraphicsSceneView(scene3dViewPtr_);

    scene3dViewPtr_->setActiveZeroing(isActiveZeroing_);

    onChannelsUpdated();

    createMapTileManagerConnections();
    createScene3dConnections();

    QMetaObject::invokeMethod(dataProcessor_, "setBottomTrackPtr", Qt::QueuedConnection, Q_ARG(BottomTrack*, scene3dViewPtr_->bottomTrack().get()));
    QMetaObject::invokeMethod(deviceManagerWrapperPtr_->getWorker(), "createLocationReader", Qt::QueuedConnection);
}

void Core::setMosaicChannels(const QString& firstChStr, const QString& secondChStr)
{
    if (datasetPtr_ && dataProcessor_ && scene3dViewPtr_) {
        auto [ch1, sub1, name1] = datasetPtr_->channelIdFromName(firstChStr);
        auto [ch2, sub2, name2] = datasetPtr_->channelIdFromName(secondChStr);

        Q_UNUSED(name1)
        Q_UNUSED(name2)

        if (lastCh1_  != ch1  || lastSub1_ != sub1 ||
            lastCh2_  != ch2  || lastSub2_ != sub2) {
            datasetPtr_->setMosaicChannels(firstChStr, secondChStr); // for calc dim rects

            QMetaObject::invokeMethod(dataProcessor_, "setMosaicChannels",
                                      Qt::QueuedConnection, Q_ARG(ChannelId, ch1), Q_ARG(uint8_t, sub1), Q_ARG(ChannelId, ch2), Q_ARG(uint8_t, sub2));
            lastCh1_  = ch1;
            lastSub1_ = sub1;
            lastCh2_  = ch2;
            lastSub2_ = sub2;
        }
    }
}

#ifdef SEPARATE_READING
QString Core::getTryOpenedfilePath() const
{
    return tryOpenedfilePath_;
}

void Core::stopDeviceManagerThread() const
{
    emit deviceManagerWrapperPtr_->sendCloseFile(false);
}
#endif

bool Core::getIsFileOpening() const
{
    return isFileOpening_;
}

bool Core::getIsSeparateReading() const
{
#ifdef SEPARATE_READING
    return true;
#else
    return false;
#endif
}

void Core::onChannelsUpdated()
{
    auto chs = datasetPtr_->channelsList();
    int chSize = chs.size();

    if (!chSize) {
        fChName_.clear();
        sChName_.clear();
        emit channelListUpdated();

        return;
    }

    QString fChName;
    QString sChName;

    if (openedfilePath_.isEmpty()) {
        auto linkNames = getLinkNames();
        if (chSize > 0 && linkNames.contains(chs[0].channelId_.uuid)) {
            fChName = chs[0].portName_;
        }
        if (chSize > 1 && linkNames.contains(chs[1].channelId_.uuid)) {
            sChName = chs[1].portName_;
        }
    }
    else {
        if (chSize > 0) {
            fChName = chs[0].portName_;
        }
        if (chSize > 1) {
            sChName = chs[1].portName_;
        }
    }

    if (fChName.isEmpty() && sChName.isEmpty()) {
        if (syncLoupePlot3dPtr_ && chSize >= 1) {
            if (chSize >= 2) {
                syncLoupePlot3dPtr_->setDataChannel(false,
                                                    chs[0].channelId_, chs[0].subChannelId_, chs[0].portName_,
                                                    chs[1].channelId_, chs[1].subChannelId_, chs[1].portName_);
            }
            else {
                syncLoupePlot3dPtr_->setDataChannel(false, chs[0].channelId_, chs[0].subChannelId_, chs[0].portName_);
            }
            syncLoupePlot3dPtr_->plotUpdate();
        }
        emit channelListUpdated();
        return;
    }

    const int numPlots = plot2dList_.size();
    for (int i = 0; i < numPlots; i++) {
        if (chSize >= 2) {
            plot2dList_.at(i)->setDataChannel(false, chs[0].channelId_, chs[0].subChannelId_, fChName, chs[1].channelId_, chs[1].subChannelId_, sChName);
            plot2dList_.at(i)->plotUpdate();
            fChName_ = QString("%1|%2|%3").arg(fChName, QString::number(chs[0].channelId_.address), QString::number(chs[0].subChannelId_));
            sChName_ = QString("%1|%2|%3").arg(sChName, QString::number(chs[1].channelId_.address), QString::number(chs[1].subChannelId_));
        }
        if (chSize == 1) {
            plot2dList_.at(i)->setDataChannel(false, chs[0].channelId_, chs[0].subChannelId_, fChName);
            plot2dList_.at(i)->plotUpdate();
            fChName_ = QString("%1|%2|%3").arg(fChName, QString::number(chs[0].channelId_.address), QString::number(chs[0].subChannelId_));
        }
    }

    if (syncLoupePlot3dPtr_) {
        if (chSize >= 2) {
            const QString loupeFirstName = fChName.isEmpty() ? chs[0].portName_ : fChName;
            const QString loupeSecondName = sChName.isEmpty() ? chs[1].portName_ : sChName;
            syncLoupePlot3dPtr_->setDataChannel(false,
                                                chs[0].channelId_, chs[0].subChannelId_, loupeFirstName,
                                                chs[1].channelId_, chs[1].subChannelId_, loupeSecondName);
            syncLoupePlot3dPtr_->plotUpdate();
        }
        else if (chSize == 1) {
            const QString loupeChannelName = fChName.isEmpty() ? chs[0].portName_ : fChName;
            syncLoupePlot3dPtr_->setDataChannel(false, chs[0].channelId_, chs[0].subChannelId_, loupeChannelName);
            syncLoupePlot3dPtr_->plotUpdate();
        }
    }

    emit channelListUpdated();
}

void Core::onRedrawEpochs(const QSet<int>& indxs)
{
    const int numPlots = plot2dList_.size();
    for (int i = 0; i < numPlots; i++) {
        plot2dList_[i]->addReRenderPlotIndxs(indxs);
    }
    if (syncLoupePlot3dPtr_) {
        syncLoupePlot3dPtr_->addReRenderPlotIndxs(indxs);
    }
}

QString Core::getChannel1Name() const
{
    return fChName_;
}

QString Core::getChannel2Name() const
{
    return sChName_;
}

void Core::registerSyncLoupePlot(QObject* plotObj)
{
    auto* plot = qobject_cast<qPlot2D*>(plotObj);
    if (!plot) {
        return;
    }

    syncLoupePlot3dPtr_ = plot;

    if (datasetPtr_) {
        syncLoupePlot3dPtr_->setPlot(datasetPtr_);
    }
    if (dataProcessor_) {
        syncLoupePlot3dPtr_->setDataProcessor(dataProcessor_);
    }

    if (!datasetPtr_) {
        return;
    }

    const auto chs = datasetPtr_->channelsList();
    if (chs.isEmpty()) {
        return;
    }

    if (chs.size() >= 2) {
        syncLoupePlot3dPtr_->setDataChannel(false,
                                            chs[0].channelId_, chs[0].subChannelId_, chs[0].portName_,
                                            chs[1].channelId_, chs[1].subChannelId_, chs[1].portName_);
    }
    else {
        syncLoupePlot3dPtr_->setDataChannel(false, chs[0].channelId_, chs[0].subChannelId_, chs[0].portName_);
    }
    syncLoupePlot3dPtr_->plotUpdate();
}

QVariant Core::getConvertedMousePos(int indx, int mouseX, int mouseY)
{
    QVariantMap retVal;

    int currIndx = indx - 1;
    int secIndx = currIndx == 0 ? 1 : 0;

    if (plot2dList_.size() < 2) {
        return retVal;
    }

    auto& firstPlot =  plot2dList_.at(currIndx);
    auto& secondPlot =  plot2dList_.at(secIndx);

    bool isCurrHor = firstPlot->isHorizontal();
    bool isSecHor  = secondPlot->isHorizontal();

    const float currDepth = firstPlot->getDepthByMousePos(mouseX, mouseY, isCurrHor);
    const int currEpochIndx = firstPlot->getEpochIndxByMousePos(mouseX, mouseY, isCurrHor);

    if (currEpochIndx == -1) {
        retVal["x"] = mouseX;
        retVal["y"] = mouseY;
        return retVal;
    }

    const auto mousePos = secondPlot->getMousePosByDepthAndEpochIndx(currDepth, currEpochIndx, isSecHor);

    retVal["x"] = mousePos.x();
    retVal["y"] = mousePos.y();

    return retVal;
}

void Core::setIsAttitudeExpected(bool state)
{
    dataHorizon_->setIsAttitudeExpected(state);
}

void Core::setMapTileProvider(int providerId)
{
    if (!tileManager_) {
        return;
    }

    if (tileManager_->currentProviderId() == providerId) {
        return;
    }

    tileManager_->setProvider(providerId);

    if (scene3dViewPtr_) {
        scene3dViewPtr_->updateMapView();
    }

    QSettings settings("KOGGER", "KoggerApp");
    settings.setValue("Map/TileProviderId", providerId);
}

void Core::toggleMapTileProvider()
{
    if (!tileManager_) {
        return;
    }

    tileManager_->toggleProvider();

    if (scene3dViewPtr_) {
        scene3dViewPtr_->updateMapView();
    }

    QSettings settings("KOGGER", "KoggerApp");
    settings.setValue("Map/TileProviderId", tileManager_->currentProviderId());
}

int Core::getMapTileProviderId() const
{
    if (tileManager_) {
        return tileManager_->currentProviderId();
    }

    return loadSavedMapTileProviderId();
}

QString Core::getMapTileProviderName() const
{
    if (tileManager_) {
        return tileManager_->currentProviderName();
    }

    const int savedProvider = loadSavedMapTileProviderId();
    if (savedProvider == map::kOsmProviderId) {
        return QStringLiteral("OpenStreetMap");
    }

    return QStringLiteral("Google Satellite");
}

QVariantList Core::getMapTileProviders() const
{
    QVariantList providers;

    QVariantMap osm;
    osm["id"] = map::kOsmProviderId;
    osm["name"] = QStringLiteral("OpenStreetMap");
    osm["layer_type"] = QStringLiteral("street");
    providers.append(osm);

    QVariantMap google;
    google["id"] = map::kGoogleProviderId;
    google["name"] = QStringLiteral("Google Satellite");
    google["layer_type"] = QStringLiteral("satellite");
    providers.append(google);

    return providers;
}

bool Core::getInternetAvailable() const
{
    return internetAvailable_;
}

bool Core::getMapTileLoadingEnabled() const
{
    return mapTileLoadingEnabled_;
}

void Core::setMapTileLoadingEnabled(bool enabled)
{
    if (mapTileLoadingEnabled_ == enabled) {
        return;
    }

    mapTileLoadingEnabled_ = enabled;

    if (tileManager_) {
        tileManager_->setMapEnabled(mapTileLoadingEnabled_);
    }

    emit mapTileLoadingEnabledChanged();
}

int Core::loadSavedMapTileProviderId() const
{
    QSettings settings("KOGGER", "KoggerApp");
    return settings.value("Map/TileProviderId", map::kGoogleProviderId).toInt();
}

void Core::onFileStopsOpening()
{
    isFileOpening_ = false;
    emit sendIsFileOpening();
    dataHorizon_->setIsFileOpening(isFileOpening_);
    QMetaObject::invokeMethod(dataProcessor_, "setSuppressResults", Qt::QueuedConnection, Q_ARG(bool, false));
}

void Core::onSendMapTextureIdByTileIndx(const map::TileIndex &tileIndx, GLuint textureId)
{
    tileManager_->getTileSetPtr()->setTextureIdByTileIndx(tileIndx, textureId);
}

void Core::setPosZeroing(bool state)
{
    isActiveZeroing_ = state;

    datasetPtr_->setActiveZeroing(isActiveZeroing_);

    if (scene3dViewPtr_) {
        scene3dViewPtr_->setActiveZeroing(isActiveZeroing_);
    }
}

ConsoleListModel* Core::consoleList()
{
    return consolePtr_->listModel();
}

void Core::createControllers()
{
    boatTrackControlMenuController_       = std::make_shared<BoatTrackControlMenuController>();
    navigationArrowControlMenuController_ = std::make_shared<NavigationArrowControlMenuController>();
    bottomTrackControlMenuController_     = std::make_shared<BottomTrackControlMenuController>();
    mpcFilterControlMenuController_       = std::make_shared<MpcFilterControlMenuController>();
    npdFilterControlMenuController_       = std::make_shared<NpdFilterControlMenuController>();
    isobathsViewControlMenuController_    = std::make_shared<IsobathsViewControlMenuController>();
    mosaicViewControlMenuController_      = std::make_shared<MosaicViewControlMenuController>();
    imageViewControlMenuController_       = std::make_shared<ImageViewControlMenuController>();
    mapViewControlMenuController_         = std::make_shared<MapViewControlMenuController>();
    pointGroupControlMenuController_      = std::make_shared<PointGroupControlMenuController>();
    polygonGroupControlMenuController_    = std::make_shared<PolygonGroupControlMenuController>();
    scene3dControlMenuController_         = std::make_shared<Scene3DControlMenuController>();
    scene3dToolBarController_             = std::make_shared<Scene3dToolBarController>();
    usblViewControlMenuController_        = std::make_shared<UsblViewControlMenuController>();
}

#ifdef SEPARATE_READING
void Core::createDeviceManagerConnections()
{
    Qt::ConnectionType deviceManagerConnection = Qt::ConnectionType::AutoConnection;

    deviceManagerWrapperConnections_.append(QObject::connect(deviceManagerWrapperPtr_->getWorker(), &DeviceManager::sendFrameInputToLogger, this, &Core::onSendFrameInputToLogger,  deviceManagerConnection));

    //
    deviceManagerWrapperConnections_.append(QObject::connect(deviceManagerWrapperPtr_->getWorker(), &DeviceManager::sendChartSetup,         datasetPtr_, &Dataset::setChartSetup,   deviceManagerConnection));
    deviceManagerWrapperConnections_.append(QObject::connect(deviceManagerWrapperPtr_->getWorker(), &DeviceManager::sendTranscSetup,        datasetPtr_, &Dataset::setTranscSetup,  deviceManagerConnection));
    deviceManagerWrapperConnections_.append(QObject::connect(deviceManagerWrapperPtr_->getWorker(), &DeviceManager::sendSoundSpeeed,        datasetPtr_, &Dataset::setSoundSpeed,   deviceManagerConnection));

    deviceManagerWrapperConnections_.append(QObject::connect(deviceManagerWrapperPtr_->getWorker(), &DeviceManager::chartComplete,          datasetPtr_, &Dataset::addChart,        deviceManagerConnection));
    deviceManagerWrapperConnections_.append(QObject::connect(deviceManagerWrapperPtr_->getWorker(), &DeviceManager::rawDataRecieved,        datasetPtr_, &Dataset::rawDataRecieved, deviceManagerConnection));
    deviceManagerWrapperConnections_.append(QObject::connect(deviceManagerWrapperPtr_->getWorker(), &DeviceManager::distComplete,           datasetPtr_, &Dataset::addDist,         deviceManagerConnection));
    deviceManagerWrapperConnections_.append(QObject::connect(deviceManagerWrapperPtr_->getWorker(), &DeviceManager::usblSolutionComplete,   datasetPtr_, &Dataset::addUsblSolution, deviceManagerConnection));
    deviceManagerWrapperConnections_.append(QObject::connect(deviceManagerWrapperPtr_->getWorker(), &DeviceManager::dopplerBeamComlete,     datasetPtr_, &Dataset::addDopplerBeam,  deviceManagerConnection));
    deviceManagerWrapperConnections_.append(QObject::connect(deviceManagerWrapperPtr_->getWorker(), &DeviceManager::dvlSolutionComplete,    datasetPtr_, &Dataset::addDVLSolution,  deviceManagerConnection));
    deviceManagerWrapperConnections_.append(QObject::connect(deviceManagerWrapperPtr_->getWorker(), &DeviceManager::upgradeProgressChanged, this,        &Core::upgradeChanged,     deviceManagerConnection));
    deviceManagerWrapperConnections_.append(QObject::connect(deviceManagerWrapperPtr_->getWorker(), &DeviceManager::eventComplete,          datasetPtr_, &Dataset::addEvent,        deviceManagerConnection));
    deviceManagerWrapperConnections_.append(QObject::connect(deviceManagerWrapperPtr_->getWorker(), &DeviceManager::rangefinderComplete,    datasetPtr_, &Dataset::addRangefinder,  deviceManagerConnection));
    deviceManagerWrapperConnections_.append(QObject::connect(deviceManagerWrapperPtr_->getWorker(), &DeviceManager::positionComplete,       datasetPtr_, &Dataset::addPosition,     deviceManagerConnection));
    deviceManagerWrapperConnections_.append(QObject::connect(deviceManagerWrapperPtr_->getWorker(), &DeviceManager::gnssVelocityComplete,   datasetPtr_, &Dataset::addGnssVelocity, deviceManagerConnection));
    deviceManagerWrapperConnections_.append(QObject::connect(deviceManagerWrapperPtr_->getWorker(), &DeviceManager::simpleNavV2Complete,   datasetPtr_, &Dataset::addSimpleNavV2,  deviceManagerConnection));
    deviceManagerWrapperConnections_.append(QObject::connect(deviceManagerWrapperPtr_->getWorker(), &DeviceManager::boatStatusComplete,     datasetPtr_, &Dataset::addBoatStatus,   deviceManagerConnection));
    deviceManagerWrapperConnections_.append(QObject::connect(deviceManagerWrapperPtr_->getWorker(), &DeviceManager::attitudeComplete,       datasetPtr_, &Dataset::addAtt,          deviceManagerConnection));
    deviceManagerWrapperConnections_.append(QObject::connect(deviceManagerWrapperPtr_->getWorker(), &DeviceManager::tempComplete,           datasetPtr_, &Dataset::addTemp,         deviceManagerConnection));
    deviceManagerWrapperConnections_.append(QObject::connect(deviceManagerWrapperPtr_->getWorker(), &DeviceManager::fileOpened,             this,        &Core::onFileOpened,       deviceManagerConnection));
    deviceManagerWrapperConnections_.append(QObject::connect(deviceManagerWrapperPtr_->getWorker(), &DeviceManager::fileBreaked,            this,        &Core::onFileOpenBreaked,  deviceManagerConnection));
    deviceManagerWrapperConnections_.append(QObject::connect(deviceManagerWrapperPtr_->getWorker(), &DeviceManager::onFileReadEnough,       this,        &Core::onFileReadEnough,   deviceManagerConnection));
    deviceManagerWrapperConnections_.append(QObject::connect(this, &Core::sendCloseLogFile,                       deviceManagerWrapperPtr_->getWorker(), &DeviceManager::closeFile, deviceManagerConnection));
    deviceManagerWrapperConnections_.append(QObject::connect(deviceManagerWrapperPtr_->getWorker(), &DeviceManager::fileStartOpening,       this,        &Core::onFileStartOpening, deviceManagerConnection));
    deviceManagerWrapperConnections_.append(QObject::connect(deviceManagerWrapperPtr_->getWorker(), &DeviceManager::encoderComplete,        datasetPtr_, &Dataset::addEncoder,      deviceManagerConnection));
    deviceManagerWrapperConnections_.append(QObject::connect(deviceManagerWrapperPtr_->getWorker(), &DeviceManager::fileStopsOpening,       this, [this]() {
                                                                                                                                                      isFileOpening_ = false;
                                                                                                                                                      emit sendIsFileOpening();
                                                                                                                                                  }, deviceManagerConnection));
    deviceManagerWrapperConnections_.append(QObject::connect(deviceManagerWrapperPtr_->getWorker(), &DeviceManager::sendProtoFrame,        &logger_, &Logger::receiveProtoFrame,    deviceManagerConnection));
    deviceManagerWrapperConnections_.append(QObject::connect(&logger_, &Logger::loggingKlfStarted,  deviceManagerWrapperPtr_->getWorker(), &DeviceManager::onLoggingKlfStarted,     deviceManagerConnection));
}

void Core::removeDeviceManagerConnections()
{
    for (auto& itm : deviceManagerWrapperConnections_)
        disconnect(itm);
    deviceManagerWrapperConnections_.clear();
}
#else
void Core::createDeviceManagerConnections()
{
    Qt::ConnectionType deviceManagerConnection = Qt::ConnectionType::DirectConnection;

    QObject::connect(deviceManagerWrapperPtr_->getWorker(), &DeviceManager::sendFrameInputToLogger, this, &Core::onSendFrameInputToLogger,  deviceManagerConnection);

    //
    QObject::connect(deviceManagerWrapperPtr_->getWorker(), &DeviceManager::sendChartSetup,         datasetPtr_, &Dataset::setChartSetup,   deviceManagerConnection);
    QObject::connect(deviceManagerWrapperPtr_->getWorker(), &DeviceManager::sendTranscSetup,        datasetPtr_, &Dataset::setTranscSetup,  deviceManagerConnection);
    QObject::connect(deviceManagerWrapperPtr_->getWorker(), &DeviceManager::sendSoundSpeeed,        datasetPtr_, &Dataset::setSoundSpeed,   deviceManagerConnection);

    QObject::connect(deviceManagerWrapperPtr_->getWorker(), &DeviceManager::chartComplete,          datasetPtr_, &Dataset::addChart,        deviceManagerConnection);
    QObject::connect(deviceManagerWrapperPtr_->getWorker(), &DeviceManager::rawDataRecieved,        datasetPtr_, &Dataset::rawDataRecieved, deviceManagerConnection);
    QObject::connect(deviceManagerWrapperPtr_->getWorker(), &DeviceManager::distComplete,           datasetPtr_, &Dataset::addDist,         deviceManagerConnection);
    QObject::connect(deviceManagerWrapperPtr_->getWorker(), &DeviceManager::usblSolutionComplete,   datasetPtr_, &Dataset::addUsblSolution, deviceManagerConnection);
    QObject::connect(deviceManagerWrapperPtr_->getWorker(), &DeviceManager::dopplerBeamComlete,     datasetPtr_, &Dataset::addDopplerBeam,  deviceManagerConnection);
    QObject::connect(deviceManagerWrapperPtr_->getWorker(), &DeviceManager::dvlSolutionComplete,    datasetPtr_, &Dataset::addDVLSolution,  deviceManagerConnection);
    QObject::connect(deviceManagerWrapperPtr_->getWorker(), &DeviceManager::upgradeProgressChanged, this,        &Core::upgradeChanged,     deviceManagerConnection);
    QObject::connect(deviceManagerWrapperPtr_->getWorker(), &DeviceManager::eventComplete,          datasetPtr_, &Dataset::addEvent,        deviceManagerConnection);
    QObject::connect(deviceManagerWrapperPtr_->getWorker(), &DeviceManager::rangefinderComplete,    datasetPtr_, &Dataset::addRangefinder,  deviceManagerConnection);
    QObject::connect(deviceManagerWrapperPtr_->getWorker(), &DeviceManager::positionComplete,       datasetPtr_, &Dataset::addPosition,     deviceManagerConnection);
    QObject::connect(deviceManagerWrapperPtr_->getWorker(), &DeviceManager::positionCompleteRTK,    datasetPtr_, &Dataset::addPositionRTK,  deviceManagerConnection);

    QObject::connect(deviceManagerWrapperPtr_->getWorker(), &DeviceManager::depthComplete,          datasetPtr_, &Dataset::addDepth,        deviceManagerConnection);

    QObject::connect(deviceManagerWrapperPtr_->getWorker(), &DeviceManager::gnssVelocityComplete,   datasetPtr_, &Dataset::addGnssVelocity, deviceManagerConnection);
    QObject::connect(deviceManagerWrapperPtr_->getWorker(), &DeviceManager::simpleNavV2Complete,    datasetPtr_, &Dataset::addSimpleNavV2,  deviceManagerConnection);
    QObject::connect(deviceManagerWrapperPtr_->getWorker(), &DeviceManager::boatStatusComplete,     datasetPtr_, &Dataset::addBoatStatus,   deviceManagerConnection);
    QObject::connect(deviceManagerWrapperPtr_->getWorker(), &DeviceManager::attitudeComplete,       datasetPtr_, &Dataset::addAtt,          deviceManagerConnection);
    QObject::connect(deviceManagerWrapperPtr_->getWorker(), &DeviceManager::tempComplete,           datasetPtr_, &Dataset::addTemp,         deviceManagerConnection);
    QObject::connect(deviceManagerWrapperPtr_->getWorker(), &DeviceManager::fileOpened,             this,        &Core::onFileOpened,       deviceManagerConnection);
    QObject::connect(deviceManagerWrapperPtr_->getWorker(), &DeviceManager::encoderComplete,        datasetPtr_, &Dataset::addEncoder,      deviceManagerConnection);
    QObject::connect(deviceManagerWrapperPtr_->getWorker(), &DeviceManager::fileStopsOpening,       this,        &Core::onFileStopsOpening, deviceManagerConnection);
    QObject::connect(deviceManagerWrapperPtr_->getWorker(), &DeviceManager::sendProtoFrame,         &logger_, &Logger::receiveProtoFrame, deviceManagerConnection);
    QObject::connect(&logger_, &Logger::loggingKlfStarted, deviceManagerWrapperPtr_->getWorker(), &DeviceManager::onLoggingKlfStarted, deviceManagerConnection);

}
#endif

void Core::createLinkManagerConnections()
{
    Qt::ConnectionType linkManagerConnection = Qt::ConnectionType::AutoConnection;
    linkManagerWrapperConnections_.append(QObject::connect(linkManagerWrapperPtr_->getWorker(), &LinkManager::frameReady,  deviceManagerWrapperPtr_->getWorker(), &DeviceManager::frameInput,     linkManagerConnection));
    linkManagerWrapperConnections_.append(QObject::connect(linkManagerWrapperPtr_->getWorker(), &LinkManager::linkClosed,  deviceManagerWrapperPtr_->getWorker(), &DeviceManager::onLinkClosed,   linkManagerConnection));
    linkManagerWrapperConnections_.append(QObject::connect(linkManagerWrapperPtr_->getWorker(), &LinkManager::linkOpened,  deviceManagerWrapperPtr_->getWorker(), &DeviceManager::onLinkOpened,   linkManagerConnection));
    linkManagerWrapperConnections_.append(QObject::connect(linkManagerWrapperPtr_->getWorker(), &LinkManager::linkDeleted, deviceManagerWrapperPtr_->getWorker(), &DeviceManager::onLinkDeleted,  linkManagerConnection));

    linkManagerWrapperConnections_.append(QObject::connect(linkManagerWrapperPtr_->getWorker(), &LinkManager::linkOpened,  this, [this]() {
#ifdef SEPARATE_READING
                                                                                                                                     tryOpenedfilePath_.clear();
#endif
                                                                                                                                     datasetPtr_->setState(Dataset::DatasetState::kConnection);
                                                                                                                                 }, linkManagerConnection));

    linkManagerWrapperConnections_.append(QObject::connect(linkManagerWrapperPtr_->getWorker(), &LinkManager::linkClosed,  this, [this]() {
                                                                                                                                     if (scene3dViewPtr_) {
                                                                                                                                         scene3dViewPtr_->getNavigationArrowPtr()->resetPositionAndAngle();
                                                                                                                                     }
                                                                                                                                 }, linkManagerConnection));

    linkManagerWrapperConnections_.append(QObject::connect(linkManagerWrapperPtr_->getWorker(), &LinkManager::sendDoRequestAll, deviceManagerWrapperPtr_->getWorker(), &DeviceManager::onSendRequestAll, linkManagerConnection));
}

void Core::removeLinkManagerConnections()
{
    for (auto& itm : linkManagerWrapperConnections_)
        disconnect(itm);
    linkManagerWrapperConnections_.clear();
}

QHash<QUuid, QString> Core::getLinkNames() const
{
    QHash<QUuid, QString> retVal;

    if (isFileOpening_) {
        retVal[deviceManagerWrapperPtr_->getFileUuid()] = QObject::tr("File");
    }

    const auto linkNames = linkManagerWrapperPtr_->getLinkNames();
    for (auto it = linkNames.constBegin(); it != linkNames.constEnd(); ++it) {
        retVal.insert(it.key(), it.value());
    }

    return retVal;
}

void Core::shutdownDataProcessor()
{
    QMetaObject::invokeMethod(dataProcessor_, "shutdown", Qt::BlockingQueuedConnection);
}

bool Core::isOpenedFile() const
{
    return !openedfilePath_.isEmpty();
}

bool Core::isFactoryMode() const
{
#ifdef FLASHER
        return true;
#else
        return false;
#endif
}

QString Core::getFilePath() const
{
    return filePath_;
}

void Core::fixFilePathString(QString& filePath) const
{
    Q_UNUSED(filePath);
#ifdef Q_OS_WINDOWS
    filePath.remove("'");

    DWORD size = GetLongPathNameW(reinterpret_cast<LPCWSTR>(filePath.utf16()), nullptr, 0);
    std::wstring buffer(size, L'\0');
    size = GetLongPathNameW(reinterpret_cast<LPCWSTR>(filePath.utf16()), &buffer[0], size);
    buffer.resize(size);
    filePath = QString::fromStdWString(buffer.c_str());

    filePath.replace("\\", "/");
#endif
}

void Core::saveLLARefToSettings()
{
    if (!datasetPtr_) {
        return;
    }

    try {
        auto ref = datasetPtr_->getLlaRef();

        QSettings settings("KOGGER", "KoggerApp");
        QString group{"LLARef"};

        settings.beginGroup(group);
        settings.setValue("refLatSin", ref.refLatSin);
        settings.setValue("refLatCos", ref.refLatCos);
        settings.setValue("refLatRad", ref.refLatRad);
        settings.setValue("refLonRad", ref.refLonRad);
        settings.setValue("refLlaLatitude", ref.refLla.latitude);
        settings.setValue("refLlaLongitude", ref.refLla.longitude);
        settings.setValue("refLlaAltitude", ref.refLla.altitude);
        settings.setValue("isInit", ref.isInit);
        settings.endGroup();

        settings.sync();

        //qDebug() << "saved: " << ref.refLla.latitude << ref.refLla.longitude;
    }
    catch (const std::exception& e) {
        qCritical() << "Core::saveLLARefToSettings throw exception:" << e.what();
    }
    catch (...) {
        qCritical() << "Core::saveLLARefToSettings throw unknown exception";
    }
}

void Core::loadLLARefFromSettings()
{
    if (!datasetPtr_) {
        return;
    }

    try {
        QSettings settings("KOGGER", "KoggerApp");
        QString group{"LLARef"};

        settings.beginGroup(group);
        LLARef ref;
        ref.refLatSin = settings.value("refLatSin", NAN).toDouble();
        ref.refLatCos = settings.value("refLatCos", NAN).toDouble();
        ref.refLatRad = settings.value("refLatRad", NAN).toDouble();
        ref.refLonRad = settings.value("refLonRad", NAN).toDouble();
        ref.refLla.latitude = settings.value("refLlaLatitude", NAN).toDouble();
        ref.refLla.longitude = settings.value("refLlaLongitude", NAN).toDouble();
        ref.refLla.altitude = settings.value("refLlaAltitude", 0.0).toDouble();

        ref.isInit = settings.value("isInit", false).toBool();
        settings.endGroup();

        if (!std::isfinite(ref.refLla.altitude)) {
            //qWarning() << "Core::loadLLARefFromSettings: refLla.altitude is NaN, forcing 0";
            ref.refLla.altitude = 0.0;
        }

        if (!std::isfinite(ref.refLla.latitude) || !std::isfinite(ref.refLla.longitude)) {
            //qWarning() << "Core::loadLLARefFromSettings: invalid lat/lon, disabling isInit";
            ref.isInit = false;
        }

        datasetPtr_->setLlaRef(ref, Dataset::LlaRefState::kUndefined/*kSettings*/); // TODO!!!

        //qDebug() << "loaded: " << ref.refLla.latitude << ref.refLla.longitude;
    }
    catch (const std::exception& e) {
        qCritical() << "Core::loadLLARefFromSettings throw exception:" << e.what();
    }
    catch (...) {
        qCritical() << "Core::loadLLARefFromSettings throw unknown exception";
    }
}

void Core::createMapTileManagerConnections()
{
    tileManager_ = std::make_unique<map::TileManager>(this);
    tileManager_->setInternetAvailable(getInternetAvailable());
    tileManager_->setMapEnabled(mapTileLoadingEnabled_);

    QObject::connect(scene3dViewPtr_, &GraphicsScene3dView::sendRectRequest, tileManager_.get(), &map::TileManager::getRectRequest, Qt::DirectConnection);
    QObject::connect(scene3dViewPtr_, &GraphicsScene3dView::sendLlaRef, tileManager_.get(), &map::TileManager::getLlaRef, Qt::DirectConnection);
    QObject::connect(tileManager_.get(), &map::TileManager::providerChanged, this, &Core::mapTileProviderChanged, Qt::DirectConnection);

    auto connType = Qt::DirectConnection;
    QObject::connect(tileManager_->getTileSetPtr().get(),    &map::TileSet::mvAppendTile,         scene3dViewPtr_->getMapViewPtr().get(), &MapView::onTileAppend,             connType);
    QObject::connect(tileManager_->getTileSetPtr().get(),    &map::TileSet::mvDeleteTile,         scene3dViewPtr_->getMapViewPtr().get(), &MapView::onTileDelete,             connType);
    QObject::connect(tileManager_->getTileSetPtr().get(),    &map::TileSet::mvUpdateTileImage,    scene3dViewPtr_->getMapViewPtr().get(), &MapView::onTileImageUpdated,       connType);
    QObject::connect(tileManager_->getTileSetPtr().get(),    &map::TileSet::mvUpdateTileVertices, scene3dViewPtr_->getMapViewPtr().get(), &MapView::onTileVerticesUpdated,    connType);
    QObject::connect(tileManager_->getTileSetPtr().get(),    &map::TileSet::mvClearAppendTasks,   scene3dViewPtr_->getMapViewPtr().get(), &MapView::onClearAppendTasks,       connType);
    QObject::connect(scene3dViewPtr_->getMapViewPtr().get(), &MapView::deletedFromAppend,         tileManager_->getTileSetPtr().get(),    &map::TileSet::onDeletedFromAppend, connType);

    QObject::connect(scene3dViewPtr_, &GraphicsScene3dView::sendMapTextureIdByTileIndx, this, &Core::onSendMapTextureIdByTileIndx, Qt::DirectConnection);

    const int savedProvider = loadSavedMapTileProviderId();
    if (savedProvider != tileManager_->currentProviderId()) {
        tileManager_->setProvider(savedProvider);
        if (scene3dViewPtr_) {
            scene3dViewPtr_->updateMapView();
        }
    }
}

void Core::onDataProcesstorStateChanged(const DataProcessorType& state)
{
    dataProcessorState_ = state;
    emit dataProcessorStateChanged();
}

void Core::onSendFrameInputToLogger(QUuid uuid, Link *link, const Parsers::FrameParser& frame)
{
    // qDebug() << "Core::onSendFrameInputToLogger" << frame.availContext();
    if (getKlfLogging()) {
        logger_.onFrameParserReceiveKlf(uuid, link, frame);
    }
}

void Core::createDatasetConnections()
{
    QObject::connect(datasetPtr_, &Dataset::channelsUpdated, this,               &Core::onChannelsUpdated);
    QObject::connect(datasetPtr_, &Dataset::redrawEpochs,    this,               &Core::onRedrawEpochs);
}

void Core::createInternetManager()
{
    if (!internetThread_) {
        internetThread_ = new QThread(this);
        internetThread_->setObjectName("InternetThread");
    }

    if (!internetManager_) {
        internetManager_ = new InternetManager();
        internetManager_->moveToThread(internetThread_);

        QObject::connect(internetThread_, &QThread::started, internetManager_, &InternetManager::start, Qt::QueuedConnection);
        QObject::connect(internetThread_, &QThread::finished, internetManager_, &QObject::deleteLater, Qt::QueuedConnection);
        QObject::connect(internetManager_, &InternetManager::internetAvailabilityChanged, this,
                         [this](bool available) {
                             internetAvailable_ = available;
                             if (tileManager_) {
                                 tileManager_->setInternetAvailable(available);
                             }
                             emit internetAvailableChanged();
                         },
                         Qt::QueuedConnection);
    }

    if (!internetThread_->isRunning()) {
        internetThread_->start();
    }
}

void Core::destroyInternetManager()
{
    if (internetManager_ && internetThread_ && internetThread_->isRunning()) {
        QMetaObject::invokeMethod(internetManager_, "stop", Qt::BlockingQueuedConnection);
    }
    if (internetThread_ && internetThread_->isRunning()) {
        internetThread_->quit();
        internetThread_->wait();
    }
    internetManager_ = nullptr;
    internetThread_ = nullptr;
}

int Core::getDataProcessorState() const
{
    return static_cast<int>(dataProcessorState_);
}

void Core::initStreamList()
{
    deviceManagerWrapperPtr_->initStreamList();
}

void Core::createDataProcessor()
{
    dataProcThread_ = new QThread(this);
    dataProcessor_  = new DataProcessor(nullptr, datasetPtr_);

    dataProcessor_->moveToThread(dataProcThread_);

    QObject::connect(dataProcThread_, &QThread::finished, dataProcessor_,  &QObject::deleteLater);
    QObject::connect(dataProcThread_, &QThread::finished, dataProcThread_, &QObject::deleteLater);

    dataProcThread_->setObjectName("DataProcThread");

    setDataProcessorConnections();

    dataProcThread_->start();
}

void Core::destroyDataProcessor()
{
    resetDataProcessorConnections();

    if (dataProcThread_ && dataProcThread_->isRunning()) {
        dataProcThread_->quit();
        dataProcThread_->wait();
    }

    delete dataProcessor_;

    dataProcessor_ = nullptr;
    dataProcThread_ = nullptr;
}

void Core::createScene3dConnections()
{
    QObject::connect(dataHorizon_.get(), &DataHorizon::positionAdded, scene3dViewPtr_, &GraphicsScene3dView::onPositionAdded);
    QObject::connect(scene3dViewPtr_->bottomTrack().get(), &BottomTrack::updatedPoints, dataHorizon_.get(), &DataHorizon::onAddedBottomTrack3D);

    // res work proc
    auto connType = Qt::QueuedConnection;
    // Surface
    QObject::connect(dataProcessor_, &DataProcessor::sendSurfaceTextureTask,        scene3dViewPtr_->getSurfaceViewPtr().get(),     &SurfaceView::setTextureTask,                 connType);
    QObject::connect(dataProcessor_, &DataProcessor::sendSurfaceMinZ,               scene3dViewPtr_->getSurfaceViewPtr().get(),     &SurfaceView::setMinZ,                        connType);
    QObject::connect(dataProcessor_, &DataProcessor::sendSurfaceMaxZ,               scene3dViewPtr_->getSurfaceViewPtr().get(),     &SurfaceView::setMaxZ,                        connType);
    QObject::connect(dataProcessor_, &DataProcessor::sendSurfaceStepSize,           scene3dViewPtr_->getSurfaceViewPtr().get(),     &SurfaceView::setSurfaceStep,                 connType);
    QObject::connect(dataProcessor_, &DataProcessor::sendSurfaceColorIntervalsSize, scene3dViewPtr_->getSurfaceViewPtr().get(),     &SurfaceView::setColorIntervalsSize,          connType);
    // IsobathsView
    //QObject::connect(dataProcessor_, &DataProcessor::sendIsobathsLabels,            scene3dViewPtr_->getIsobathsViewPtr().get(),    &IsobathsView::setLabels,                     connType);
    //QObject::connect(dataProcessor_, &DataProcessor::sendIsobathsLineSegments,      scene3dViewPtr_->getIsobathsViewPtr().get(),    &IsobathsView::setLineSegments,               connType);
    //QObject::connect(dataProcessor_, &DataProcessor::sendIsobathsLineStepSize,      scene3dViewPtr_->getIsobathsViewPtr().get(),    &IsobathsView::setLineStepSize,               connType);
    // Mosaic
    QObject::connect(dataProcessor_, &DataProcessor::sendMosaicColorTable,          scene3dViewPtr_->getSurfaceViewPtr().get(),     &SurfaceView::setMosaicColorTableTextureTask, connType);
    QObject::connect(dataProcessor_, &DataProcessor::sendSurfaceTiles,              scene3dViewPtr_->getSurfaceViewPtr().get(),     &SurfaceView::setTiles,                       connType); // TODO: del
    QObject::connect(dataProcessor_, &DataProcessor::sendSurfaceTilesIncremental,   scene3dViewPtr_->getSurfaceViewPtr().get(),     &SurfaceView::setTilesIncremental,            connType);
    QObject::connect(dataProcessor_, &DataProcessor::sendTraceLines,                scene3dViewPtr_->getSurfaceViewPtr().get(),     &SurfaceView::setTraceLines,                  connType);

    // clear render
    QObject::connect(dataProcessor_, &DataProcessor::bottomTrackProcessingCleared,  scene3dViewPtr_->bottomTrack().get(),           &BottomTrack::clearData,                      connType);
    //QObject::connect(dataProcessor_, &DataProcessor::isobathsProcessingCleared,     scene3dViewPtr_->getIsobathsViewPtr().get(),    &IsobathsView::clear,                         connType);
    QObject::connect(dataProcessor_, &DataProcessor::mosaicProcessingCleared,       this, [](){ /*qDebug() << "TODO: mosaicProcessingCleared";*/ },                               connType);
    QObject::connect(dataProcessor_, &DataProcessor::surfaceProcessingCleared,      this, [](){ /*qDebug() << "TODO: mosaicProcessingCleared";*/ },                               connType);
    QObject::connect(dataProcessor_, &DataProcessor::allProcessingCleared,          this, [](){ /*qDebug() << "TODO: allProcessingCleared";*/    },                               connType);

    // data tiles request
    QObject::connect(scene3dViewPtr_,    &GraphicsScene3dView::sendDataZoom,          dataProcessor_, &DataProcessor::onUpdateDataZoom,      connType); // отправляет зум и чекает в кеше/бд наличие (совместить?)
    QObject::connect(scene3dViewPtr_,    &GraphicsScene3dView::sendDataRectRequest,   dataProcessor_, &DataProcessor::onSendDataRectRequest, connType); // отправляет запрос на рендер данных

    QMetaObject::invokeMethod(dataProcessor_, "askColorTableForMosaic", Qt::QueuedConnection);
}

void Core::createDataHorizonConnections()
{
    dataHorizonConnections_.append(QObject::connect(datasetPtr_, &Dataset::epochAdded,       dataHorizon_.get(), &DataHorizon::onAddedEpoch));
    dataHorizonConnections_.append(QObject::connect(datasetPtr_, &Dataset::positionAdded,    dataHorizon_.get(), &DataHorizon::onAddedPosition));
    dataHorizonConnections_.append(QObject::connect(datasetPtr_, &Dataset::chartAdded,       dataHorizon_.get(), &DataHorizon::onAddedChart));
    dataHorizonConnections_.append(QObject::connect(datasetPtr_, &Dataset::attitudeAdded,    dataHorizon_.get(), &DataHorizon::onAddedAttitude));
    dataHorizonConnections_.append(QObject::connect(datasetPtr_, &Dataset::artificalAttitudeAdded, dataHorizon_.get(), &DataHorizon::onAddedArtificalAttitude));
    dataHorizonConnections_.append(QObject::connect(datasetPtr_, &Dataset::bottomTrackAdded, dataHorizon_.get(), &DataHorizon::onAddedBottomTrack));
}

void Core::destroyDataHorizonConnections()
{
    for (auto& itm : dataHorizonConnections_) {
        disconnect(itm);
    }

    dataHorizonConnections_.clear();
}

void Core::setDataProcessorConnections()
{
    // from dataHorizon
    auto connType = Qt::QueuedConnection;
    dataProcessorConnections_.append(QObject::connect(dataHorizon_.get(), &DataHorizon::chartAdded,                    dataProcessor_, &DataProcessor::onChartsAdded,           connType));
    dataProcessorConnections_.append(QObject::connect(dataHorizon_.get(), &DataHorizon::bottomTrack3DAdded,            dataProcessor_, &DataProcessor::onBottomTrack3DAdded,    connType));

    dataProcessorConnections_.append(QObject::connect(dataHorizon_.get(), &DataHorizon::sonarPosCanCalc,               datasetPtr_,    &Dataset::onSonarPosCanCalc,             Qt::DirectConnection));
    dataProcessorConnections_.append(QObject::connect(dataHorizon_.get(), &DataHorizon::dimRectsCanCalc,               datasetPtr_,    &Dataset::onDimensionRectCanCalc,        Qt::DirectConnection));

    dataProcessorConnections_.append(QObject::connect(datasetPtr_,        &Dataset::sendTilesByZoom,                   dataProcessor_, &DataProcessor::onSendTilesByZoom,       connType));
    dataProcessorConnections_.append(QObject::connect(datasetPtr_,        &Dataset::datasetStateChanged,               dataProcessor_, &DataProcessor::onDatasetStateChanged,       connType));

    dataProcessorConnections_.append(QObject::connect(dataHorizon_.get(), &DataHorizon::epochAdded,                    dataProcessor_, &DataProcessor::onEpochAdded,            connType));
    dataProcessorConnections_.append(QObject::connect(dataHorizon_.get(), &DataHorizon::mosaicCanCalc,                 dataProcessor_, &DataProcessor::onMosaicCanCalc,         connType));

    dataProcessorConnections_.append(QObject::connect(dataProcessor_,     &DataProcessor::distCompletedByProcessing,      datasetPtr_,    &Dataset::onDistCompleted,               connType));
    dataProcessorConnections_.append(QObject::connect(dataProcessor_,     &DataProcessor::distCompletedByProcessingBatch, datasetPtr_,    &Dataset::onDistCompletedBatch,          connType));
    dataProcessorConnections_.append(QObject::connect(dataProcessor_,     &DataProcessor::lastBottomTrackEpochChanged, datasetPtr_,    &Dataset::onLastBottomTrackEpochChanged, connType));
    dataProcessorConnections_.append(QObject::connect(dataProcessor_,     &DataProcessor::sendState,                   this,           &Core::onDataProcesstorStateChanged,     connType));
}

void Core::resetDataProcessorConnections()
{
    for (auto& itm : dataProcessorConnections_) {
        disconnect(itm);
    }

    dataProcessorConnections_.clear();
}

#ifdef FLASHER
void Core::dev_flasher_rcv(QString msg, int num) {
    dev_flasher_msg_ = msg;
    dev_flasher_msg_id_ = num;
    emit dev_flasher_changed();
}

void Core::connectOpenedLinkAsFlasher(QString pn) {
    dev_flasher_.setLinkAsFlasher(getLinkManagerWrapperPtr(), getDeviceManagerWrapperPtr()->getWorker(), pn);
}

void Core::setFlasherData(QString data) {
    dev_flasher_.setData(data);
}

void Core::releaseFlasherLink() {
    dev_flasher_.releaseLink();
}
#endif
