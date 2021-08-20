#include <logger.h>
#include <core.h>
extern Core core;


bool Logger::startNewLog() {
    stopLogging();

    bool is_open = false;
    QDir dir;
    QString str_log_path = QCoreApplication::applicationDirPath() + "/logs";
    if(dir.mkpath(str_log_path)) {
        dir.setPath(str_log_path);

        QString file_name = QDateTime::currentDateTime().toString("yyyy.MM.dd_hh:mm:ss") + ".klf";
        file_name.replace(':', '.');

        m_logFile->setFileName(str_log_path + "/" + file_name);
        is_open = m_logFile->open(QIODevice::WriteOnly);

        if(is_open) {
            core.consoleInfo("Logger dir: " + dir.path());
            core.consoleInfo("Logger make file: " + m_logFile->fileName());
        } else {
            core.consoleInfo("Logger can't make file: " + m_logFile->fileName());
        }
    } else {
        core.consoleInfo("Logger can't make dir");
    }

    return is_open;
}

bool Logger::stopLogging() {
    if(isOpen()) { core.consoleInfo("Logger stoped"); }
    m_logFile->close();
    return true;
}

void Logger::loggingStream(const QByteArray &data) {
    if(isOpen()) { m_logFile->write(data); }
}

bool Logger::creatExportStream(QString name) {
    bool is_open = false;

    QUrl url(name);
    _exportFile->setFileName(url.toLocalFile());
    is_open = _exportFile->open(QIODevice::WriteOnly);

    if(is_open) {
        core.consoleInfo("Export make file: " + _exportFile->fileName());
    } else {
        core.consoleInfo("Export can't make file: " + _exportFile->fileName());
    }

    return is_open;
}

bool Logger::dataExport(QString str) {
    if(_exportFile->isOpen()) {
        _exportFile->write(str.toUtf8());
    }
    return true;
}

bool Logger::endExportStream() {
    _exportFile->close();
    return true;
}
