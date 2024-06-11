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
    void writeCsvHat();

    // export
    bool creatExportStream(QString str);
    bool dataExport(QString str);
    bool dataByteExport(QByteArray data);
    bool endExportStream();

private:
    static const int klfFlushInterval_ = 100; // frames
    static const int csvFlushInterval_ = 3; // epoch

    struct {
        bool meas_nbr           = true;
        bool event_id           = true;
        bool rangefinder        = false;
        bool bottom_depth       = true;
        bool pos_lat_lon        = true;
        bool pos_time           = true;

        bool external_pos_lla   = true;
        bool external_pos_neu   = true;
        bool sonar_height       = true;
        bool bottom_height      = true;

        bool ext_pos_lla_find   = false;
        bool ext_pos_ned_find   = false;

    } csvData_;

    std::unique_ptr<QFile> klfLogFile_;
    std::unique_ptr<QFile> csvLogFile_;
    std::unique_ptr<QFile> exportFile_;
    Dataset* datasetPtr_;
    QList<QMetaObject::Connection> csvConnections_;
    Position lastCsvPos_;
    int klfCurrentIteration_;
    int csvCurrentIteration_;
    bool csvHatWrited_;
};
