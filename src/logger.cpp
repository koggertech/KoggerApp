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
    klfCurrentIteration_(0)
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
    QString logPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/KoggerApp";
#else
    QString logPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/KoggerApp/logs";
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

    if (isOpen) {
        emit loggingKlfStarted();
    }

    return isOpen;
}

bool Logger::stopKlfLogging()
{
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
    QString logPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/KoggerApp";
#else
    QString logPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/KoggerApp/logs";
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
            csvData_.csvConnections.append(QObject::connect(datasetPtr_, &Dataset::dataUpdate, this, &Logger::loggingCsvStream, Qt::AutoConnection));
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
    for (auto& itm : csvData_.csvConnections) {
        disconnect(itm);
    }

    csvData_.csvConnections.clear();
    csvData_.lastCsvPos = Position();
    csvData_.csvCurrentIteration = 0;
    csvData_.csvHatWrited = false;
    csvData_.counter = 0;

    if (isOpenCsv()) {
        csvLogFile_->close();
    }

    return true;
}

void Logger::loggingCsvStream()
{
    auto epoch = datasetPtr_->lastlast();
    if (!epoch || !isOpenCsv()) {
        return;
    }

    if (!csvData_.csvHatWrited)
        writeCsvHat();

    if (epoch->getPositionGNSS().lla.isCoordinatesValid()) {
        csvData_.lastCsvPos = epoch->getPositionGNSS();
    }

    if (epoch->rangeFinder()) {
        int prevTimestamp = 0;
        int prevUnix = 0;
        int prevEventId = 0;
        double prevLat = 0, prevLon = 0;

        LLARef llaRef;
        NED lastPosNed;
        QString rowData;

        if (csvData_.measNbr)
            rowData.append(QString("%1,").arg(csvData_.counter));

        if (csvData_.eventId) {
            if (epoch->eventAvail()) {
                prevTimestamp = epoch->eventTimestamp();
                prevEventId = epoch->eventID();
                prevUnix = epoch->eventUnix();
            }
            rowData.append(QString("%1,%2,%3,").arg(prevUnix).arg(prevTimestamp).arg(prevEventId));
        }

        if (csvData_.rangefinder)
            epoch->distAvail() ? rowData.append(QString("%1,").arg((float)epoch->rangeFinder())) : rowData.append("0,");

        if (csvData_.bottomDepth) {
            rowData.append(QString("%1,").arg(NAN));
        }

        if (csvData_.posLatLon) {
            prevLat = csvData_.lastCsvPos.lla.latitude;
            prevLon = csvData_.lastCsvPos.lla.longitude;
            rowData.append(QString::number(prevLat, 'f', 8));
            rowData.append(",");
            rowData.append(QString::number(prevLon, 'f', 8));
            rowData.append(",");

            if (csvData_.posTime) {
                if (epoch->isPosAvail() && epoch->positionTimeUnix() != 0) {
                    QDateTime dateTime = QDateTime::fromSecsSinceEpoch(epoch->getPositionGNSS().time.sec);

                    int year = dateTime.date().year();
                    int month = dateTime.date().month();
                    int day = dateTime.date().day();
                    int hour = dateTime.time().hour();
                    int minute = dateTime.time().minute();
                    int second = dateTime.time().second();

                    QString dateStr = QString("%1-%2-%3").arg(year).arg(month).arg(day);
                    QString timeStr = QString("%1:%2:%3").arg(hour).arg(minute).arg(second);

                    rowData.append(dateStr);
                    rowData.append(",");
                    rowData.append(timeStr);
                    rowData.append(",");
                }
                else {
                    rowData.append(",");
                    rowData.append(",");
                }
            }
        }

        //Position position = epoch->getExternalPosition();
        //if (csvData_.external_pos_lla && csvData_.ext_pos_lla_find) {
            rowData.append(QString::number(NAN, 'f', 10));
            rowData.append(",");
            rowData.append(QString::number(NAN, 'f', 10));
            rowData.append(",");
            rowData.append(QString::number(NAN, 'f', 3));
            rowData.append(",");
        //}

        //if (csvData_.external_pos_neu && csvData_.ext_pos_ned_find) {
            rowData.append(QString::number(NAN, 'f', 10));
            rowData.append(",");
            rowData.append(QString::number(NAN, 'f', 10));
            rowData.append(",");
            rowData.append(QString::number(NAN, 'f', 3));
            rowData.append(",");
        //}

        if (csvData_.sonarHeight) {
            rowData.append(QString::number(NAN, 'f', 3));
            rowData.append(",");
        }
        if (csvData_.bottomHeight) {
            rowData.append(QString::number(NAN, 'f', 3));
            rowData.append(",");
        }
        rowData.append("\n");

        // write to file
        csvLogFile_->write(rowData.toUtf8());
        if (csvData_.csvCurrentIteration > csvData_.csvFlushInterval) {
            csvLogFile_->flush();
            csvData_.csvCurrentIteration = 0;
        }
        else {
            ++csvData_.csvCurrentIteration;
        }
        ++csvData_.counter;
    }
}

bool Logger::isOpenCsv()
{
    return csvLogFile_->isOpen();
}

void Logger::writeCsvHat()
{
    if (!isOpenCsv())
        return;

    if (csvData_.measNbr)
        csvLogFile_->write(QString("Number,").toUtf8());

    if (csvData_.eventId) {
        csvLogFile_->write(QString("Event UNIX,").toUtf8());
        csvLogFile_->write(QString("Event timestamp,").toUtf8());
        csvLogFile_->write(QString("Event ID,").toUtf8());
    }

    if (csvData_.rangefinder)
        csvLogFile_->write(QString("Rangefinder,").toUtf8());


    if (csvData_.bottomDepth)
        csvLogFile_->write(QString("Beam distance,").toUtf8());


    if (csvData_.posLatLon) {
        csvLogFile_->write(QString("Latitude,").toUtf8());
        csvLogFile_->write(QString("Longitude,").toUtf8());

        if (csvData_.posTime) {
            csvLogFile_->write(QString("GNSS UTC Date,").toUtf8());
            csvLogFile_->write(QString("GNSS UTC Time,").toUtf8());
        }
    }

    //if (csvData_.external_pos_lla && csvData_.ext_pos_lla_find) {
    csvLogFile_->write(QString("ExtLatitude,").toUtf8());
    csvLogFile_->write(QString("ExtLongitude,").toUtf8());
    csvLogFile_->write(QString("ExtAltitude,").toUtf8());
    //}

    //if (csvData_.external_pos_neu && csvData_.ext_pos_ned_find) {
    csvLogFile_->write(QString("ExtNorth,").toUtf8());
    csvLogFile_->write(QString("ExtEast,").toUtf8());
    csvLogFile_->write(QString("ExtHeight,").toUtf8());
    //}

    if (csvData_.sonarHeight)
        csvLogFile_->write(QString("SonarHeight,").toUtf8());

    if (csvData_.bottomHeight)
        csvLogFile_->write(QString("BottomHeight,").toUtf8());

    csvLogFile_->write(QString("\n").toUtf8());

    csvData_.csvHatWrited = true;
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

void Logger::receiveProtoFrame(ProtoBinOut protoBinOut)
{
    QByteArray data = QByteArray((const char*)protoBinOut.frame(), protoBinOut.frameLen());

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

