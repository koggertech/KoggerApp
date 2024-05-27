#include "FileReader.h"

#include <QDebug>
#include <QFile>
#include <QThread>


FileReader::FileReader(QObject *parent) :
    QObject(parent),
    progress_(0),
    break_(false)
{

}

FileReader::~FileReader()
{
    cleanUp();
}

void FileReader::cleanUp()
{
    progress_ = 0;
    break_ = false;
}

void FileReader::startRead()
{
    qDebug() << "FileReader::doActions start";
    qDebug() << "fr th_id: " << QThread::currentThreadId();

    for (int i = 0; i < 100; ++i) {
        if (break_) {
            cleanUp();
            qDebug() << "stopped!";
            break;
        }


        QThread::msleep(10);
        ++progress_;
        emit progressUpdated(progress_);
    }

    qDebug() << "FileReader::doActions end";

    emit completed();
}

void FileReader::stopRead()
{
    qDebug() << "FileReader::stopRead";

    break_ = true;
}
