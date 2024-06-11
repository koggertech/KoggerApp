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
    lastCsvPos_(Position())
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

    return true;
}

void Logger::loggingKlfStream(const QByteArray &data)
{
    if (isOpenKlf()) {
        klfLogFile_->write(data);
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
    qDebug() << "Logger::startNewCsvLog";

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
    qDebug() << "Logger::stopCsvLogging";

    for (auto& itm : csvConnections_)
        disconnect(itm);
    csvConnections_.clear();

    lastCsvPos_ = Position();

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


    // TODO
    auto a = datasetPtr_->lastlast();
    //qDebug() << a->time()->getDateTime().tm_sec;
    //if (lastCsvPos_.ned.n != a->getPositionGNSS().ned.n) {

    //}


    // write to file
    //QByteArray res;
    //klfLogFile_->write(res);
}

bool Logger::isOpenCsv()
{
    qDebug() << "Logger::isOpenCsv";

    return csvLogFile_->isOpen();
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


