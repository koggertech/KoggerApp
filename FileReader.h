#pragma once

#include <QObject>
#include <QByteArray>
#include <QUuid>

#include "Link.h"
#include "ProtoBinnary.h"


class FileReader : public QObject
{
    Q_OBJECT
public:
    /*methods*/
    explicit FileReader(QObject *parent = nullptr);
    ~FileReader();

private:
    /*methods*/
    void cleanUp();
    /*data*/
    volatile bool break_;
    int progress_;

signals:
    void progressUpdated(int);
    void completed();
    void interrupted();
    void frameReady(QUuid uuid, Link* link, Parsers::FrameParser frame);

public slots:
    void startRead(const QString& filePath);
    void stopRead();

};
