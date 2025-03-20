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

signals:
    void loggingKlfStarted();

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
    void writeCsvHat();

    // export
    bool creatExportStream(QString str);
    bool dataExport(QString str);
    bool dataByteExport(QByteArray data);
    bool endExportStream();

    void receiveProtoFrame(ProtoBinOut protoBinOut);

private:
    struct {
        QList<QMetaObject::Connection> csvConnections;
        Position lastCsvPos;
        const int csvFlushInterval = 3; // num epoch
        int csvCurrentIteration    = 0;
        int counter                = 0;
        const bool measNbr         = true;
        const bool eventId         = true;
        const bool rangefinder     = true; // been false ?!
        const bool bottomDepth     = true;
        const bool posLatLon       = true;
        const bool posTime         = true;
        const bool externalPosla   = true;
        const bool externalPosNeu  = true;
        const bool sonarHeight     = true;
        const bool bottomHeight    = true;
        bool csvHatWrited          = false;

    } csvData_;

    static const int klfFlushInterval_ = 100; // num frames
    std::unique_ptr<QFile> klfLogFile_;
    std::unique_ptr<QFile> csvLogFile_;
    std::unique_ptr<QFile> exportFile_;
    Dataset* datasetPtr_;
    int klfCurrentIteration_;
};
