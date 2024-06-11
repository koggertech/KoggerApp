#pragma once

#include <QObject>
#include <QByteArray>
#include <QFile>
#include <QString>
#include <QUuid>
#include "Link.h"
#include "ProtoBinnary.h"


class Logger : public QObject
{
    Q_OBJECT

public:
    Logger();

public slots:
    bool startNewLog();
    bool stopLogging();
    void loggingStream(const QByteArray &data);
    bool isOpen();
    bool creatExportStream(QString str);
    bool dataExport(QString str);
    bool dataByteExport(QByteArray data);
    bool endExportStream();
    void onFrameParserReceive(QUuid uuid, Link* link, FrameParser frame);

private:
    QFile* logFile_;
    QFile* exportFile_;
};
