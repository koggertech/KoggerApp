#include <logger.h>

#include <QDateTime>
#include <QDir>
#include <QStandardPaths>
#include <QUrl>
#include "core.h"
extern Core core;


Logger::Logger() :
    klfLogFile_(std::make_unique<QFile>(this)),
    csvLogFile_(std::make_unique<QFile>(this)),
    exportFile_(std::make_unique<QFile>(this)),
    datasetPtr_(nullptr),
    lastCsvPos_(Position()),
    klfCurrentIteration_(0),
    csvCurrentIteration_(0),
    csvHatWrited_(false)
{

}

void Logger::setDatasetPtr(Dataset *datasetPtr)
{
    if (!datasetPtr)
        return;

    datasetPtr_ = datasetPtr;
}

bool Logger::startNewKlfLog()
{
    stopKlfLogging();

    bool isOpen = false;
    QDir dir;

#ifdef Q_OS_ANDROID
    QString logPath =  QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/KoggerApp";
#else
    QString logPath = QCoreApplication::applicationDirPath() + "/logs";
#endif

    if (dir.mkpath(logPath)) {
        dir.setPath(logPath);

        QString fileName = QDateTime::currentDateTime().toString("yyyy.MM.dd_hh:mm:ss") + ".klf";
        fileName.replace(':', '.');

        klfLogFile_->setFileName(logPath + "/" + fileName);
        isOpen = klfLogFile_->open(QIODevice::WriteOnly);

        if (isOpen) {
            core.consoleInfo("Logger dir: " + dir.path());
            core.consoleInfo("Logger make file: " + klfLogFile_->fileName());
        }
        else {
            core.consoleInfo("Logger can't make file: " + klfLogFile_->fileName());
        }
    }
    else {
        core.consoleInfo("Logger can't make dir");
    }

    return isOpen;
}

bool Logger::stopKlfLogging()
{
    qDebug() << "Logger::stopKlfLogging";

    if (isOpenKlf()) {
        core.consoleInfo("Logger klf stoped");
    }

    klfLogFile_->close();
    klfCurrentIteration_ = 0;

    return true;
}

void Logger::loggingKlfStream(const QByteArray &data)
{
    if (isOpenKlf()) {
        klfLogFile_->write(data);

        if (klfCurrentIteration_ > klfFlushInterval_) {
            klfLogFile_->flush();
            klfCurrentIteration_ = 0;
        }
        else {
            ++klfCurrentIteration_;
        }
    }
}

bool Logger::isOpenKlf()
{
    return klfLogFile_->isOpen();
}

void Logger::onFrameParserReceiveKlf(QUuid uuid, Link* linkPtr, FrameParser frame)
{
    Q_UNUSED(uuid);
    Q_UNUSED(linkPtr);

    if (frame.isNested()) {
        return;
    }

    loggingKlfStream(QByteArray((const char*)frame.frame(), frame.frameLen()));
}









bool Logger::startNewCsvLog()
{
    //qDebug() << "Logger::startNewCsvLog";

    stopCsvLogging();

    // open file
    bool isOpen = false;
    QDir dir;

#ifdef Q_OS_ANDROID
    QString logPath =  QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/KoggerApp";
#else
    QString logPath = QCoreApplication::applicationDirPath() + "/logs";
#endif

    if (dir.mkpath(logPath)) {
        dir.setPath(logPath);

        QString fileName = QDateTime::currentDateTime().toString("yyyy.MM.dd_hh:mm:ss") + ".csv";
        fileName.replace(':', '.');

        csvLogFile_->setFileName(logPath + "/" + fileName);
        isOpen = csvLogFile_->open(QIODevice::WriteOnly);

        if (isOpen) {
            core.consoleInfo("Logger csv dir: " + dir.path());
            core.consoleInfo("Logger csv make file: " + csvLogFile_->fileName());

            // connects
            csvConnections_.append(QObject::connect(datasetPtr_, &Dataset::dataUpdate, this, &Logger::loggingCsvStream, Qt::AutoConnection));
        }
        else {
            core.consoleInfo("Logger csv can't make file: " + csvLogFile_->fileName());
        }
    }
    else {
        core.consoleInfo("Logger csv can't make dir");
    }

    return isOpen;
}

bool Logger::stopCsvLogging()
{
    //qDebug() << "Logger::stopCsvLogging";

    for (auto& itm : csvConnections_)
        disconnect(itm);
    csvConnections_.clear();

    lastCsvPos_ = Position();
    csvCurrentIteration_ = 0;
    csvHatWrited_ = false;

    if (isOpenCsv()) {
        csvLogFile_->close();
    }


    return true;
}

void Logger::loggingCsvStream()
{
    //qDebug() << "Logger::loggingCsvStream";

    if (!isOpenCsv())
        return;

    if (!csvHatWrited_)
        writeCsvHat();


    /*
    // TODO
    auto a = datasetPtr_->lastlast();
    auto currPos = a->getPositionGNSS();
    auto rangeFinder = a->rangeFinder();



    /////////////////////////////////////////////////////////

    int prev_timestamp = 0;
    int prev_unix = 0;
    int prev_event_id = 0;
    float prev_dist_proc = 0;
    double prev_lat = 0, prev_lon = 0;

    float decimation_m = decimation;
    float decimation_path = 0;
    LLARef lla_ref;
    NED last_pos_ned;



    //for (int i = 0; i < row_cnt; i++) {


        Epoch* epoch = datasetPtr_->fromIndex(i);

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


        QString row_data;

        if (csvData_.meas_nbr)
            row_data.append(QString("%1,").arg(i));

        if (csvData_.event_id) {
            if (epoch->eventAvail()) {
                prev_timestamp = epoch->eventTimestamp();
                prev_event_id = epoch->eventID();
                prev_unix = epoch->eventUnix();
            }
            row_data.append(QString("%1,%2,%3,").arg(prev_unix).arg(prev_timestamp).arg(prev_event_id));
        }

        if (csvData_.rangefinder)
            epoch->distAvail() ? row_data.append(QString("%1,").arg((float)epoch->rangeFinder())) : row_data.append("0,");

        if (csvData_.bottom_depth) {
            prev_dist_proc = epoch->distProccesing(channel);
            row_data.append(QString("%1,").arg((float)(prev_dist_proc)));
        }

        if (csvData_.pos_lat_lon) {
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
                    //                    DateTime* dt = epoch->positionTime();
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

        if (csvData_.external_pos_lla && csvData_.ext_pos_lla_find) {
            row_data.append(QString::number(position.lla.latitude, 'f', 10));
            row_data.append(",");
            row_data.append(QString::number(position.lla.longitude, 'f', 10));
            row_data.append(",");
            row_data.append(QString::number(position.lla.altitude, 'f', 3));
            row_data.append(",");
        }

        if (csvData_.external_pos_neu && csvData_.ext_pos_ned_find) {
            row_data.append(QString::number(position.ned.n, 'f', 10));
            row_data.append(",");
            row_data.append(QString::number(position.ned.e, 'f', 10));
            row_data.append(",");
            row_data.append(QString::number(-position.ned.d, 'f', 3));
            row_data.append(",");
        }

        Epoch::Echogram* sensor = epoch->chart(channel);

        if (csvData_.sonar_height) {
            if (sensor != NULL && isfinite(sensor->sensorPosition.ned.d)) {
                row_data.append(QString::number(-sensor->sensorPosition.ned.d, 'f', 3));
            }
            else if (sensor != NULL && isfinite(sensor->sensorPosition.lla.altitude)) {
                row_data.append(QString::number(sensor->sensorPosition.lla.altitude, 'f', 3));
            }
            row_data.append(",");
        }

        if (csvData_.bottom_height) {
            if(sensor != NULL && isfinite(sensor->bottomProcessing.bottomPoint.ned.d)) {
                row_data.append(QString::number(-sensor->bottomProcessing.bottomPoint.ned.d, 'f', 3));
            }
            else if (sensor != NULL && isfinite(sensor->bottomProcessing.bottomPoint.lla.altitude)) {
                row_data.append(QString::number(sensor->bottomProcessing.bottomPoint.lla.altitude, 'f', 3));
            }
            row_data.append(",");
        }

        row_data.append("\n");
        logger_.dataExport(row_data);



    //}

    // write to file
    //QByteArray res;
    //klfLogFile_->write(res);





    //logger_.endExportStream();

    return true;*/
}

bool Logger::isOpenCsv()
{
    //qDebug() << "Logger::isOpenCsv";
    return csvLogFile_->isOpen();
}


void Logger::writeCsvHat()
{
    /////////////////////////////////////////////////////////
    // write table hat ?


    /*
    int row_cnt = datasetPtr_->size();
    datasetPtr_->spatialProcessing(); // emitting data upd

    for (int i = 0; i < row_cnt; i++) {
        Epoch* epoch = datasetPtr_->fromIndex(i);

        Position position = epoch->getExternalPosition();
        csvData_.ext_pos_lla_find |= position.lla.isValid();
        csvData_.ext_pos_ned_find |= position.ned.isValid();
    }

    if (csvData_.meas_nbr)
        logger_.dataExport("Number,");

    if (csvData_.event_id) {
        logger_.dataExport("Event UNIX,");
        logger_.dataExport("Event timestamp,");
        logger_.dataExport("Event ID,");
    }

    if (csvData_.rangefinder)
        logger_.dataExport("Rangefinder,");

    if (csvData_.bottom_depth)
        logger_.dataExport("Beam distance,");

    if (csvData_.pos_lat_lon) {
        logger_.dataExport("Latitude,");
        logger_.dataExport("Longitude,");

        if (csvData_.pos_time) {
            logger_.dataExport("GNSS UTC Date,");
            logger_.dataExport("GNSS UTC Time,");
        }
    }

    if (csvData_.external_pos_lla && csvData_.ext_pos_lla_find) {
        logger_.dataExport("ExtLatitude,");
        logger_.dataExport("ExtLongitude,");
        logger_.dataExport("ExtAltitude,");
    }

    if (csvData_.external_pos_neu && csvData_.ext_pos_ned_find) {
        logger_.dataExport("ExtNorth,");
        logger_.dataExport("ExtEast,");
        logger_.dataExport("ExtHeight,");
    }

    if (csvData_.sonar_height)
        logger_.dataExport("SonarHeight,");

    if (csvData_.bottom_height)
        logger_.dataExport("BottomHeight,");

    logger_.dataExport("\n");



    /////////////////////////////////////////////////////////


*/

    csvHatWrited_ = true;
}










bool Logger::creatExportStream(QString name)
{
    bool isOpen = false;

    QUrl url(name);
    exportFile_->setFileName(url.toLocalFile());
    isOpen = exportFile_->open(QIODevice::WriteOnly);

    if (isOpen) {
        core.consoleInfo("Export make file: " + exportFile_->fileName());
    }
    else {
        core.consoleInfo("Export can't make file: " + exportFile_->fileName());
    }

    return isOpen;
}

bool Logger::dataExport(QString str)
{
    if (exportFile_->isOpen()) {
        exportFile_->write(str.toUtf8());
    }

    return true;
}

bool Logger::dataByteExport(QByteArray data)
{
    if (exportFile_->isOpen()) {
        exportFile_->write(data);
    }

    return true;
}

bool Logger::endExportStream()
{
    exportFile_->close();
    return true;
}


