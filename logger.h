#ifndef LOGGER_H
#define LOGGER_H

#include <QObject>
#include <QFile>
#include <QDataStream>
#include <QTextStream>
#include "QUrl"
#include <QDir>
#include <QDateTime>


class Logger : public QObject {
    Q_OBJECT
public:
    Logger() : m_logFile(new QFile(this)), _exportFile(new QFile(this))  {
    }

public slots:
    bool startNewLog();
    bool stopLogging();
    void loggingStream(const QByteArray &data);
    bool isOpen() { return m_logFile->isOpen();}


    bool creatExportStream(QString str);
    bool dataExport(QString str);
    bool dataByteExport(QByteArray data);
    bool endExportStream();

protected:
    QFile* m_logFile;

    QFile* _exportFile;
    QTextStream _exportStream();
};

#endif // LOGGER_H
