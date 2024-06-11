#pragma once

#include <memory>
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
    // .klf
    bool startNewKlfLog();
    bool stopKlfLogging();
    void loggingKlfStream(const QByteArray &data);
    bool isOpenKlf();
    void onFrameParserReceiveKlf(QUuid uuid, Link* link, FrameParser frame);

    // .csv
    bool startNewCsvLog();
    bool stopCsvLogging();
    void loggingCsvStream(const QByteArray &data);
    bool isOpenCsv();

    // export
    bool creatExportStream(QString str);
    bool dataExport(QString str);
    bool dataByteExport(QByteArray data);
    bool endExportStream();

private:
    std::unique_ptr<QFile> klfLogFile_;
    std::unique_ptr<QFile> csvLogFile_;
    std::unique_ptr<QFile> exportFile_;
};
