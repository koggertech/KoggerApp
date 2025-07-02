#pragma once

#include <QObject>
#include <QVector>
#include <QVector3D>
#include <QDateTime>
#include "dataset.h"


class DataProcessor : public QObject
{
    Q_OBJECT
public:
    explicit DataProcessor(QObject* parent = nullptr);
    ~DataProcessor() override;

signals:
    void finished();

public slots:
    void init();
    void doAction();


private:

};
