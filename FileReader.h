#pragma once

#include <QObject>
#include <QByteArray>


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

signals:
    void progressUpdated(int);
    void completed();
    void interrupted();
    void receiveData(const QByteArray &data);

public slots:
    void startRead(const QString& filePath);
    void stopRead();

};
