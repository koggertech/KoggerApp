#pragma once

#include <memory>
#include <QThread>
#include "FileReader.h"


class FileReaderWrapper : public QObject // wrapper for FileReader in main thread
{
    Q_OBJECT
public:
    /*methods*/
    FileReaderWrapper(QObject* parent);
    ~FileReaderWrapper();

    std::shared_ptr<FileReader> getWorker();

private:
    /*data*/
    std::unique_ptr<QThread> workerThread_;
    std::shared_ptr<FileReader> workerObject_;

signals:



public slots:


};
