#include "FileReader.h"

#include <QDebug>
#include <QFile>
#include <QThread>
#include <QUrl>


FileReader::FileReader(QObject *parent) :
    QObject(parent),
    break_(false),
    progress_(0)
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
    progress_ = 0;
}

void FileReader::startRead(const QString& filePath)
{
    qDebug() << "FileReader::startRead: th_id: " << QThread::currentThreadId();

    QByteArray data;
    QFile file;
    const QUrl url(filePath);
    url.isLocalFile() ? file.setFileName(url.toLocalFile()) : file.setFileName(url.toString());

    qDebug() << QString("File path: %1").arg(file.fileName());

    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "FileReader::startRead file not opened!";
        emit interrupted();
        return;
    }

    //setType(ConnectionFile);
    //emit openedEvent(false);

    const qint64 totalSize = file.size();
    qint64 bytesRead = 0;
    Parsers::FrameParser frameParser;

    const QUuid someUuid = QUuid::createUuid();
    Link someLink;
    someLink.setUuid(someUuid);

    while (true) {
        if (break_) {
            qDebug() << "FileReader::startRead interrupted!";
            file.close();
            emit interrupted();
            return;
        }

        const QByteArray chunk = file.read(1024 * 1024);
        const qint64 chunkSize = chunk.size();

        if (chunkSize == 0)
            break;

        data.append(chunk);
        bytesRead += chunkSize;

        auto currProgress = static_cast<int>((static_cast<float>(bytesRead) / static_cast<float>(totalSize)) * 100.0f);
        currProgress = std::max(0, currProgress);
        currProgress = std::min(100, currProgress);

        if (progress_ != currProgress) {
            progress_ = currProgress;
            emit progressUpdated(progress_);
        }

///
        frameParser.setContext((uint8_t*)data.data(), data.size());

        while (frameParser.availContext() > 0) {
            frameParser.process();
            if (frameParser.isComplete()) {
                emit frameReady(someUuid, &someLink, frameParser);
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
