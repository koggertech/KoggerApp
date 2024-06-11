#pragma once

#include <memory>
#include <QObject>
#include <QByteArray>
#include <QFile>
#include <QString>
#include <QUuid>
#include "plotcash.h"
#include "Link.h"
#include "ProtoBinnary.h"


class Logger : public QObject
{
    Q_OBJECT

public:
    Logger();
    void setDatasetPtr(Dataset* datasetPtr);

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
    void loggingCsvStream();
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
    Dataset* datasetPtr_;
    QList<QMetaObject::Connection> csvConnections_;
    Position lastCsvPos_;
};
