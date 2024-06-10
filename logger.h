#ifndef LOGGER_H
#define LOGGER_H

#include <QObject>
#include <QFile>
#include <QDataStream>
#include <QTextStream>
#include <QUrl>
#include <QDir>
#include <QDateTime>
#include <QUuid>
#include "Link.h"
#include "ProtoBinnary.h"


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
    void onFrameParserReceive(QUuid uuid, Link* link, FrameParser frame);

protected:
    QFile* m_logFile;

    QFile* _exportFile;
    QTextStream _exportStream();
};

#endif // LOGGER_H
