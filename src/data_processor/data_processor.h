#pragma once

#include <QObject>
#include <QVector>
#include <QVector3D>
#include <QDateTime>
#include "dataset.h"
#include "bottom_track_processor.h"


enum class DataProcessorState {
    kWaiting = 0,
    kBottomTrackCalculation,
    kIsobathsCalculation,
    kMosaicCalculation
};

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
    void sendState(DataProcessorState state);
    void finished();

public slots:
    // BottomTrackProcessor
    void bottomTrackProcessing(const ChannelId& channel1, const ChannelId& channel2, const BottomTrackParam& bottomTrackParam_); // wraper, external calling not realtime
    void onChartsAdded(const ChannelId& channelId, uint64_t indx); // external calling realtme

    void clear();
    void setUpdateBottomTrack(bool state) { updateBottomTrack_ = state; };
    void setUpdateIsobaths(bool state) { updateIsobaths_ = state; };
    void setUpdateMosaic(bool state) { updateMosaic_ = state; };
    void setIsOpeningFile(bool state) { isOpeningFile_ = state; };

private:
    void changeState(const DataProcessorState& state);

private:
    friend class BottomTrackProcessor;

    BottomTrackProcessor bottomTrackProcessor_;
    Dataset* datasetPtr_ = nullptr;
    DataProcessorState state_ = DataProcessorState::kWaiting;
    bool updateBottomTrack_ = false;
    bool updateIsobaths_ = false;
    bool updateMosaic_ = false;
    int bottomTrackWindowCounter_ = 0;
    bool isOpeningFile_ = false;
};
