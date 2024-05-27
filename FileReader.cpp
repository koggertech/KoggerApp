#include "FileReader.h"

#include <QDebug>
#include <QFile>
#include <QThread>
#include <QUrl>


FileReader::FileReader(QObject *parent) :
    QObject(parent),
    break_(false)
{
    qRegisterMetaType<Parsers::FrameParser>("Parsers::FrameParser");
}

FileReader::~FileReader()
{
    cleanUp();
}

void FileReader::cleanUp()
{
    break_ = false;
}

void FileReader::startRead(const QString& filePath)
{
    qDebug() << "FileReader::startRead: th_id: " << QThread::currentThreadId();

    QByteArray data;
    QFile file;
    QUrl url(filePath);
    url.isLocalFile() ? file.setFileName(url.toLocalFile()) : file.setFileName(url.toString());

    qDebug() << QString("File path: %1").arg(file.fileName());

    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "FileReader::startRead file not opened!";
        emit interrupted();
        return;
    }

    //setType(ConnectionFile);
    //emit openedEvent(false);

    qint64 totalSize = file.size();
    qint64 bytesRead = 0;
    Parsers::FrameParser frameParser;
    QUuid aaa = QUuid::createUuid();

    while (true) {
        if (break_) {
            qDebug() << "FileReader::startRead interrupted!";
            file.close();
            emit interrupted();
            return;
        }

        QByteArray chunk = file.read(1024 * 1024);
        qint64 chunkSize = chunk.size();

        if (chunkSize == 0)
            break;

        data.append(chunk);
        bytesRead += chunkSize;

        int percentage = static_cast<int>((static_cast<double>(bytesRead) / totalSize) * 100);

        emit progressUpdated(percentage);


///
        frameParser.setContext((uint8_t*)data.data(), data.size());

        while (frameParser.availContext() > 0) {
            frameParser.process();
            if(frameParser.isComplete()) {
                emit frameReady(aaa, nullptr, frameParser);
            }
        }


        //emit receiveData(data);

        data.clear();
    }

    data.clear();
    file.close();

    emit completed();
}

void FileReader::stopRead()
{
    qDebug() << "FileReader::stopRead";
    break_ = true;
}
