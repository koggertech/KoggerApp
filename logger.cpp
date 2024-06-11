#include <logger.h>

#include <QDateTime>
#include <QDir>
#include <QStandardPaths>
#include <QUrl>
#include "core.h"
extern Core core;


Logger::Logger() :
    logFile_(new QFile(this)),
    exportFile_(new QFile(this))
{

}

bool Logger::startNewLog()
{
    stopLogging();

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

        logFile_->setFileName(logPath + "/" + fileName);
        isOpen = logFile_->open(QIODevice::WriteOnly);

        if (isOpen) {
            core.consoleInfo("Logger dir: " + dir.path());
            core.consoleInfo("Logger make file: " + logFile_->fileName());
        }
        else {
            core.consoleInfo("Logger can't make file: " + logFile_->fileName());
        }
    }
    else {
        core.consoleInfo("Logger can't make dir");
    }

    return isOpen;
}

bool Logger::stopLogging()
{
    if (isOpen()) {
        core.consoleInfo("Logger stoped");
    }

    logFile_->close();
    return true;
}

void Logger::loggingStream(const QByteArray &data)
{
    if (isOpen()) {
        logFile_->write(data);
    }
}

bool Logger::isOpen()
{
    return logFile_->isOpen();
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

void Logger::onFrameParserReceive(QUuid uuid, Link* linkPtr, FrameParser frame)
{
    Q_UNUSED(uuid);
    Q_UNUSED(linkPtr);

    if (frame.isNested()) {
        return;
    }

    loggingStream(QByteArray((const char*)frame.frame(), frame.frameLen()));
}
