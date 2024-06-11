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
    exportFile_(std::make_unique<QFile>(this))
{

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
    return true;
}

bool Logger::stopCsvLogging()
{
    if (isOpenCsv()) {
        csvLogFile_->close();
    }

    return true;
}

void Logger::loggingCsvStream(const QByteArray &data)
{

}

bool Logger::isOpenCsv()
{
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


