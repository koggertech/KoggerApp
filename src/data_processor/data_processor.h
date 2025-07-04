#pragma once

#include <QObject>
#include <QVector>
#include <QVector3D>
#include <QDateTime>
#include "dataset.h"
#include <QReadWriteLock>


class DataProcessor : public QObject
{
    Q_OBJECT
public:
    explicit DataProcessor(QObject* parent = nullptr);
    ~DataProcessor() override;

    void setDatasetPtr(Dataset* datasetPtr);

signals:
    void distCompletedByProcessing(int epIndx, const ChannelId& channelId, float dist);
    void lastBottomTrackEpochChanged(const ChannelId& channelId, int val, const BottomTrackParam& btP);

    void finished();

public slots:
    void init();
    void doAction();
    void onChartsUpdated(int n);
    void clear();

    void bottomTrackProcessing(const ChannelId& channel1, const ChannelId& channel2, const BottomTrackParam& bottomTrackParam_);

    void setUpdateBottomTrack(bool state) { updateBottomTrack_ = state; };
    void setUpdateIsobaths(bool state) { updateIsobaths_ = state; };
    void setUpdateMosaic(bool state) { updateMosaic_ = state; };

    void setIsOpeningFile(bool state) { isOpeningFile_ = state; };


private:
    QReadWriteLock lock_;

    Dataset* datasetPtr_ = nullptr;
    bool updateBottomTrack_ = false;
    bool updateIsobaths_ = false;
    bool updateMosaic_ = false;
    int bottomTrackWindowCounter_ = 0;
    bool isOpeningFile_ = false;
};
