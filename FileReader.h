#pragma once

#include <QObject>


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
    int progress_;
    volatile bool break_;

signals:
    void progressUpdated(int);
    void completed();

public slots:
    void startRead();
    void stopRead();

};
