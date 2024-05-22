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

signals:

public slots:

};
