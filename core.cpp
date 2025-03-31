#include "core.h"

#include <QSettings>
#include <ctime>
#include "bottomtrack.h"
#include "hotkeys_manager.h"
#ifdef Q_OS_WINDOWS
#include <Windows.h>
#endif


Core::Core() :
    QObject(),
    consolePtr_(new Console),
    deviceManagerWrapperPtr_(std::make_unique<DeviceManagerWrapper>(this)),
    linkManagerWrapperPtr_(std::make_unique<LinkManagerWrapper>(this)),
    qmlAppEnginePtr_(nullptr),
    datasetPtr_(new Dataset),
    scene3dViewPtr_(nullptr),
    openedfilePath_(),
    isLoggingKlf_(false),
    isLoggingCsv_(false),
    filePath_(),
    isFileOpening_(false),
    isMosaicUpdatingInThread_(false),
    isSideScanPerformanceMode_(false)
{
    logger_.setDatasetPtr(datasetPtr_);
    createDeviceManagerConnections();
    createLinkManagerConnections();
    createControllers();
    QObject::connect(datasetPtr_, &Dataset::channelsUpdated, this, &Core::onChannelsUpdated, Qt::AutoConnection);
}

Core::~Core()
{

}

void Core::setEngine(QQmlApplicationEngine *engine)
{
    qmlAppEnginePtr_ = engine;
    QObject::connect(qmlAppEnginePtr_, &QQmlApplicationEngine::objectCreated, this, &Core::UILoad, Qt::QueuedConnection);
    qmlAppEnginePtr_->rootContext()->setContextProperty("BoatTrackControlMenuController",       boatTrackControlMenuController_.get());
    qmlAppEnginePtr_->rootContext()->setContextProperty("NavigationArrowControlMenuController", navigationArrowControlMenuController_.get());
    qmlAppEnginePtr_->rootContext()->setContextProperty("BottomTrackControlMenuController",     bottomTrackControlMenuController_.get());
    qmlAppEnginePtr_->rootContext()->setContextProperty("SurfaceControlMenuController",         surfaceControlMenuController_.get());
    qmlAppEnginePtr_->rootContext()->setContextProperty("SideScanViewControlMenuController",    sideScanViewControlMenuController_.get());
    qmlAppEnginePtr_->rootContext()->setContextProperty("ImageViewControlMenuController",       imageViewControlMenuController_.get());
    qmlAppEnginePtr_->rootContext()->setContextProperty("MapViewControlMenuController",         mapViewControlMenuController_.get());
    qmlAppEnginePtr_->rootContext()->setContextProperty("PointGroupControlMenuController",      pointGroupControlMenuController_.get());
    qmlAppEnginePtr_->rootContext()->setContextProperty("PolygonGroupControlMenuController",    polygonGroupControlMenuController_.get());
    qmlAppEnginePtr_->rootContext()->setContextProperty("MpcFilterControlMenuController",       mpcFilterControlMenuController_.get());
    qmlAppEnginePtr_->rootContext()->setContextProperty("NpdFilterControlMenuController",       npdFilterControlMenuController_.get());
    qmlAppEnginePtr_->rootContext()->setContextProperty("Scene3DControlMenuController",         scene3dControlMenuController_.get());
    qmlAppEnginePtr_->rootContext()->setContextProperty("Scene3dToolBarController",             scene3dToolBarController_.get());
    qmlAppEnginePtr_->rootContext()->setContextProperty("UsblViewControlMenuController",        usblViewControlMenuController_.get());
}

Console* Core::getConsolePtr()
{
    return consolePtr_;
}

Dataset* Core::getDatasetPtr()
{
    return datasetPtr_;
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
        consoleInfo(QString("%1KG[%2]: id %3 v%4, %5, len %6; %7 [ %8 ]").arg(str_dir).arg(parser.route()).arg(parser.id()).arg(parser.ver()).arg(str_mode).arg(parser.payloadLen()).arg(comment).arg(str_data));
    }
    catch(std::bad_alloc& ex) {
        qCritical().noquote() << __func__ << " --> " << ex.what();
    }
}

#ifdef FLASHER
void Core::getFlasherPtr() const
{
    return &flasher;
}
#endif


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

    if (!isAppend)
        datasetPtr_->resetDataset();

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

    if (scene3dViewPtr_) {
        scene3dViewPtr_->getSideScanViewPtr()->setWorkMode(SideScanView::Mode::kRealtime);
    }

    datasetPtr_->setState(Dataset::DatasetState::kFile);

    emit deviceManagerWrapperPtr_->sendOpenFile(localfilePath);
}

bool Core::closeLogFile(bool onOpen)
{
    if (isOpenedFile()) {
        emit sendCloseLogFile(onOpen ? !tryOpenedfilePath_.isEmpty() : false);
        openedfilePath_.clear();

        if (datasetPtr_) {
            datasetPtr_->resetDataset();
        }
        if (scene3dViewPtr_) {
            scene3dViewPtr_->clear();
            scene3dViewPtr_->getNavigationArrowPtr()->resetPositionAndAngle();
            //scene3dViewPtr_->getSideScanViewPtr()->setWorkMode(SideScanView::Mode::kUndefined);
        }
        if (!onOpen) {
            createLinkManagerConnections();
            linkManagerWrapperPtr_->openClosedLinks();
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

    if (scene3dViewPtr_) {
        scene3dViewPtr_->getSideScanViewPtr()->updateChannelsIds(); // TODO: not effect(
        scene3dViewPtr_->forceUpdateDatasetRef();
        //scene3dViewPtr_->setMapView();
    }
}

void Core::onFileOpened()
{
    qDebug() << "file opened!";

    tryOpenedfilePath_.clear();
    fileIsCompleteOpened_ = true;

    if (scene3dViewPtr_) {
        scene3dViewPtr_->forceUpdateDatasetRef();
        //scene3dViewPtr_->getSideScanViewPtr()->setWorkMode(SideScanView::Mode::kUndefined);
    };
}

void Core::onFileReadEnough()
{
    datasetPtr_->setRefPositionByFirstValid();
    // datasetPtr_->usblProcessing();
    if (scene3dViewPtr_) {
        scene3dViewPtr_->forceUpdateDatasetRef();
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
    }
    if (scene3dViewPtr_) {
        scene3dViewPtr_->clear();
        scene3dViewPtr_->getNavigationArrowPtr()->resetPositionAndAngle();
        //scene3dViewPtr_->getSideScanViewPtr()->setWorkMode(SideScanView::Mode::kUndefined);
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

        if (!isAppend)
            datasetPtr_->resetDataset();

        if (scene3dViewPtr_) {
            if (!isAppend) {
                scene3dViewPtr_->clear();
            }
            scene3dViewPtr_->getSideScanViewPtr()->setWorkMode(SideScanView::Mode::kPerformance);
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

        datasetPtr_->setState(Dataset::DatasetState::kFile);

        emit deviceManagerWrapperPtr_->sendOpenFile(localfilePath);

        openedfilePath_ = localfilePath;

        if (scene3dViewPtr_)
            scene3dViewPtr_->fitAllInView();

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
    if (!isOpenedFile())
        return false;

    emit deviceManagerWrapperPtr_->sendCloseFile();

    createLinkManagerConnections();

    if (datasetPtr_)
        datasetPtr_->resetDataset();

    if (scene3dViewPtr_) {
        scene3dViewPtr_->clear();
        scene3dViewPtr_->getNavigationArrowPtr()->resetPositionAndAngle();
        //scene3dViewPtr_->getSideScanViewPtr()->setWorkMode(SideScanView::Mode::kUndefined);
    }

    openedfilePath_.clear();

    linkManagerWrapperPtr_->openClosedLinks();

    return true;
}

void Core::onFileOpened()
{
    qDebug() << "file opened!";

    if (scene3dViewPtr_) {
        //scene3dViewPtr_->getSideScanViewPtr()->setWorkMode(SideScanView::Mode::kUndefined);
    };
}
#endif

bool Core::openXTF(QByteArray data)
{
    datasetPtr_->resetDataset();
    datasetPtr_->setState(Dataset::DatasetState::kFile);
    converterXtf_.toDataset(data, getDatasetPtr());

    consoleInfo("XTF note:" + QString(converterXtf_.header.NoteString));
    consoleInfo("XTF programm name:" + QString(converterXtf_.header.RecordingProgramName));
    consoleInfo("XTF sonar name:" + QString(converterXtf_.header.SonarName));

    QMap<int, DatasetChannel> chs = datasetPtr_->channelsList();

    for (int i = 0; i < plot2dList_.size(); i++) {
        if (plot2dList_.at(i) != NULL && i < chs.size()) {
            if (i == 0) {
                plot2dList_.at(i)->setDataChannel(chs[0].channel, chs[1].channel);
            }
        }
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
    QFile m_file;
    url.isLocalFile() ? m_file.setFileName(url.toLocalFile()) : m_file.setFileName(name);

    bool is_open = false;
    is_open = m_file.open(QIODevice::ReadOnly);

    if(is_open == false) {  return false;  }

    DevQProperty* dev_q = (DevQProperty*)(dev);
    dev_q->sendUpdateFW(m_file.readAll());

    return true;
}

void Core::upgradeChanged(int progressStatus)
{
    if(progressStatus == DevDriver::successUpgrade) {
        //        restoreBaudrate();
    }
}

void Core::setKlfLogging(bool isLogging)
{
    if (isLogging == this->getIsKlfLogging())
        return;
    this->getIsKlfLogging() ? logger_.stopKlfLogging() : logger_.startNewKlfLog();
    isLoggingKlf_ = isLogging;
}

void Core::setFixBlackStripesState(bool state)
{
    if (datasetPtr_) {
        datasetPtr_->setFixBlackStripesState(state);
    }
}

void Core::setFixBlackStripesRange(int val)
{
    if (datasetPtr_) {
        datasetPtr_->setFixBlackStripesRange(val);
    }
}

void Core::setFixBlackStripesBackSteps(int val)
{
    if (datasetPtr_) {
        datasetPtr_->setFixBlackStripesBackSteps(val);
    }
}

bool Core::getIsKlfLogging()
{
    return isLoggingKlf_;
}

void Core::setCsvLogging(bool isLogging)
{
    if (isLogging == this->getIsCsvLogging())
        return;
    this->getIsCsvLogging() ? logger_.stopCsvLogging() : logger_.startNewCsvLog();
    isLoggingCsv_ = isLogging;
}

bool Core::getIsCsvLogging()
{
    return isLoggingCsv_;
}

bool Core::exportComplexToCSV(QString file_path) {
    QString export_file_name = isOpenedFile() ? openedfilePath_.section('/', -1).section('.', 0, 0) : QDateTime::currentDateTime().toString("yyyy.MM.dd_hh:mm:ss").replace(':', '.');
    logger_.creatExportStream(file_path + "/" + export_file_name + ".csv");

    //QMap<int, DatasetChannel> ch_list = datasetPtr_->channelsList();
    // _dataset->setRefPosition(1518);

    for(int i = 0; i < datasetPtr_->size(); i++) {
        Epoch* epoch = datasetPtr_->fromIndex(i);

        if(epoch == NULL) { continue; }

        if(epoch->isComplexSignalAvail()) {
            ComplexSignals sigs = epoch->complexSignals();

            for (auto ch = sigs.cbegin(), end = sigs.cend(); ch != end; ++ch) {
                ComplexSignal signal = ch.value();

                ComplexF* data = signal.data.data();
                int data_size = signal.data.size();

                QString row_data;
                row_data.append(QString("%1,%2").arg(i).arg(ch.key()));
                row_data.append(QString(",%1").arg(signal.globalOffset));
                row_data.append(QString(",%1").arg(signal.sampleRate));

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

        Position pos = epoch->getPositionGNSS();

        // pos.ned.isCoordinatesValid() && epoch->isAttAvail() &&
        if( epoch->isUsblSolutionAvailable()) {
            QString row_data;

            row_data.append(QString("%1").arg(i));
            row_data.append(QString(",%1,%2,%3").arg(epoch->yaw()).arg(epoch->pitch()).arg(epoch->roll()));
            row_data.append(QString(",%1,%2").arg(pos.ned.n).arg(pos.ned.e));
            row_data.append(QString(",%1,%2,%3").arg(epoch->usblSolution().ping_counter).arg(epoch->usblSolution().carrier_counter).arg(epoch->usblSolution().snr));
            row_data.append(QString(",%1,%2,%3").arg(epoch->usblSolution().azimuth_deg).arg(epoch->usblSolution().elevation_deg).arg(epoch->usblSolution().distance_m));

            row_data.append("\n");
            logger_.dataExport(row_data);
        }
    }

    logger_.endExportStream();

    return true;
}

bool Core::exportPlotAsCVS(QString filePath, int channel, float decimation)
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

                Position pos = epoch->getPositionGNSS();

                if (pos.lla.isCoordinatesValid()) {
                    if (!lla_ref.isInit) {
                        lla_ref = LLARef(pos.lla);
                        pos.LLA2NED(&lla_ref);
                        last_pos_ned = pos.ned;
                    }
                    else {
                        pos.LLA2NED(&lla_ref);
                        float dif_n = pos.ned.n - last_pos_ned.n;
                        float dif_e = pos.ned.e - last_pos_ned.e;
                        last_pos_ned = pos.ned;
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
            prev_dist_proc = epoch->distProccesing(channel);
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

        Epoch::Echogram* sensor = epoch->chart(channel);

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
                row_data.append(QString::number(contact.distance, 'f', 4));
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
    QString export_file_name = isOpenedFile() ? openedfilePath_.section('/', -1).section('.', 0, 0) : QDateTime::currentDateTime().toString("yyyy.MM.dd_hh:mm:ss").replace(':', '.');
    logger_.creatExportStream(filePath + "/_" + export_file_name + ".xtf");
    //QMap<int, DatasetChannel> chs = datasetPtr_->channelsList();
    //Q_UNUSED(chs);
    QByteArray data_export = converterXtf_.toXTF(getDatasetPtr(), plot2dList_[0]->plotDatasetChannel(), plot2dList_[0]->plotDatasetChannel2());
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
}

void Core::setPlotStopLevel(int level)
{
    for (int i = 0; i < plot2dList_.size(); i++) {
        if (plot2dList_.at(i) != NULL)
            plot2dList_.at(i)->setEchogramHightLevel(level);
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
    plot2dList_ = object->findChildren<qPlot2D*>();
    scene3dViewPtr_->setDataset(datasetPtr_);
    datasetPtr_->setScene3D(scene3dViewPtr_);

    for (int i = 0; i < plot2dList_.size(); i++) {
        if (plot2dList_.at(i) != NULL) {
            plot2dList_.at(i)->setPlot(datasetPtr_);
            scene3dViewPtr_->bottomTrack()->installEventFilter(plot2dList_.at(i));
            scene3dViewPtr_->boatTrack()->installEventFilter(plot2dList_.at(i));
            scene3dViewPtr_->getContactsPtr()->installEventFilter(plot2dList_.at(i));
            plot2dList_.at(i)->installEventFilter(scene3dViewPtr_->bottomTrack().get());
            plot2dList_.at(i)->installEventFilter(scene3dViewPtr_->boatTrack().get());
            plot2dList_.at(i)->installEventFilter(scene3dViewPtr_->getContactsPtr().get());
        }
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

    surfaceControlMenuController_->setQmlEngine(object);
    surfaceControlMenuController_->setGraphicsSceneView(scene3dViewPtr_);

    sideScanViewControlMenuController_->setQmlEngine(object);
    sideScanViewControlMenuController_->setGraphicsSceneView(scene3dViewPtr_);

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
    scene3dToolBarController_->setGraphicsSceneView(scene3dViewPtr_);

    scene3dControlMenuController_->setQmlEngine(object);
    scene3dControlMenuController_->setGraphicsSceneView(scene3dViewPtr_);

    usblViewControlMenuController_->setQmlEngine(object);
    usblViewControlMenuController_->setGraphicsSceneView(scene3dViewPtr_);
}

void Core::setSideScanChannels(int firstChId, int secondChId)
{
    if (scene3dViewPtr_ && scene3dViewPtr_->getSideScanViewPtr()) {
        scene3dViewPtr_->getSideScanViewPtr()->setChannels(firstChId, secondChId);
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

void Core::setIsMosaicUpdatingInThread(bool state)
{
    isMosaicUpdatingInThread_ = state;

    emit isMosaicUpdatingInThreadUpdated();
}

void Core::setSideScanWorkMode(SideScanView::Mode mode)
{
    switch (mode) {
    case SideScanView::Mode::kUndefined:   { isSideScanPerformanceMode_ = false; break; }
    case SideScanView::Mode::kPerformance: { isSideScanPerformanceMode_ = true;  break; }
    case SideScanView::Mode::kRealtime:    { isSideScanPerformanceMode_ = false; break; }
    default:
        break;
    }

    emit isSideScanPerformanceModeUpdated();
}

bool Core::getIsMosaicUpdatingInThread() const
{
    return isMosaicUpdatingInThread_;
}

bool Core::getIsSideScanPerformanceMode() const
{
    return isSideScanPerformanceMode_;
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
    QList<DatasetChannel> chs = datasetPtr_->channelsList().values();
    for (int i = 0; i < plot2dList_.size(); i++) {
        if (i == 0 &&plot2dList_.at(i) != NULL) {
            if (chs.size() >= 2) {
                plot2dList_.at(i)->setDataChannel(chs[0].channel, chs[1].channel);
            }
            if (chs.size() == 1) {
                plot2dList_.at(i)->setDataChannel(chs[0].channel);
            }
        }
    }
}

#if defined(FAKE_COORDS)
void Core::setPosZeroing(bool state)
{
    datasetPtr_->setActiveZeroing(state);
}
#endif

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
    surfaceControlMenuController_         = std::make_shared<SurfaceControlMenuController>();
    sideScanViewControlMenuController_    = std::make_shared<SideScanViewControlMenuController>();
    imageViewControlMenuController_       = std::make_shared<ImageViewControlMenuController>();
    mapViewControlMenuController_         = std::make_shared<MapViewControlMenuController>();
    pointGroupControlMenuController_      = std::make_shared<PointGroupControlMenuController>();
    polygonGroupControlMenuController_    = std::make_shared<PolygonGroupControlMenuController>();
    scene3dControlMenuController_         = std::make_shared<Scene3DControlMenuController>();
    scene3dToolBarController_             = std::make_shared<Scene3dToolBarController>();
    usblViewControlMenuController_        = std::make_shared<UsblViewControlMenuController>();

    sideScanViewControlMenuController_->setCorePtr(this);
}

#ifdef SEPARATE_READING
void Core::createDeviceManagerConnections()
{
    Qt::ConnectionType deviceManagerConnection = Qt::ConnectionType::AutoConnection;

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
    deviceManagerWrapperConnections_.append(QObject::connect(deviceManagerWrapperPtr_->getWorker(), &DeviceManager::attitudeComplete,       datasetPtr_, &Dataset::addAtt,          deviceManagerConnection));
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
    QObject::connect(deviceManagerWrapperPtr_->getWorker(), &DeviceManager::gnssVelocityComplete,   datasetPtr_, &Dataset::addGnssVelocity, deviceManagerConnection);
    QObject::connect(deviceManagerWrapperPtr_->getWorker(), &DeviceManager::attitudeComplete,       datasetPtr_, &Dataset::addAtt,          deviceManagerConnection);
    QObject::connect(deviceManagerWrapperPtr_->getWorker(), &DeviceManager::fileOpened,             this,        &Core::onFileOpened,       deviceManagerConnection);
    QObject::connect(deviceManagerWrapperPtr_->getWorker(), &DeviceManager::encoderComplete,        datasetPtr_, &Dataset::addEncoder,      deviceManagerConnection);
    QObject::connect(deviceManagerWrapperPtr_->getWorker(), &DeviceManager::fileStopsOpening,       this, [this]() {
                                                                                                              isFileOpening_ = false;
                                                                                                              emit sendIsFileOpening();
                                                                                                          }, deviceManagerConnection);
    QObject::connect(deviceManagerWrapperPtr_->getWorker(), &DeviceManager::sendProtoFrame, &logger_, &Logger::receiveProtoFrame, deviceManagerConnection);    
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
    linkManagerWrapperConnections_.append(QObject::connect(linkManagerWrapperPtr_->getWorker(), &LinkManager::frameReady,  this, [this](QUuid uuid, Link* link, FrameParser frame) {
                                                                                                                                    if (getIsKlfLogging()) {
                                                                                                                                        QMetaObject::invokeMethod(&logger_, [this, uuid, link, frame]() {
                                                                                                                                                logger_.onFrameParserReceiveKlf(uuid, link, frame);
                                                                                                                                            }, Qt::QueuedConnection);
                                                                                                                                    }
                                                                                                                                 }, linkManagerConnection));

    linkManagerWrapperConnections_.append(QObject::connect(linkManagerWrapperPtr_->getWorker(), &LinkManager::linkOpened,  this, [this]() {
#ifdef SEPARATE_READING
                                                                                                                                     tryOpenedfilePath_.clear();
#endif
                                                                                                                                     datasetPtr_->setState(Dataset::DatasetState::kConnection);
                                                                                                                                     if (scene3dViewPtr_) {
                                                                                                                                         scene3dViewPtr_->getSideScanViewPtr()->setWorkMode(SideScanView::Mode::kRealtime);
                                                                                                                                     }
                                                                                                                                 }, linkManagerConnection));
    linkManagerWrapperConnections_.append(QObject::connect(linkManagerWrapperPtr_->getWorker(), &LinkManager::linkClosed,  this, [this]() {
                                                                                                                                     if (scene3dViewPtr_) {
                                                                                                                                         scene3dViewPtr_->getNavigationArrowPtr()->resetPositionAndAngle();
                                                                                                                                         //scene3dViewPtr_->getSideScanViewPtr()->setWorkMode(SideScanView::Mode::kUndefined);
                                                                                                                                     }
                                                                                                                                 }, linkManagerConnection));
}

void Core::removeLinkManagerConnections()
{
    for (auto& itm : linkManagerWrapperConnections_)
        disconnect(itm);
    linkManagerWrapperConnections_.clear();
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

bool Core::isMotorControlMode() const
{
#ifdef MOTOR
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
        ref.isInit = settings.value("isInit", false).toBool();
        settings.endGroup();

        datasetPtr_->setLlaRef(ref, Dataset::LlaRefState::kSettings);

        //qDebug() << "loaded: " << ref.refLla.latitude << ref.refLla.longitude;
    }
    catch (const std::exception& e) {
        qCritical() << "Core::loadLLARefFromSettings throw exception:" << e.what();
    }
    catch (...) {
        qCritical() << "Core::loadLLARefFromSettings throw unknown exception";
    }
}
