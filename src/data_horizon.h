#pragma once

#include <QObject>

#include "dataset_defs.h"


class DataHorizon : public QObject
{
    Q_OBJECT

public:
    DataHorizon();

    void clear();

    void setEmitChanges(bool state);
    void setIsFileOpening(bool state);

    uint64_t getEpochSize() const { return epochIndx_; };
    uint64_t getPositionIndx() const { return positionIndx_; };
    uint64_t getChartIndx() const { return chartIndx_; };
    uint64_t getAttitudeIndx() const { return attitudeIndx_; };
    QVector<int> getBottomTrackIndx() const { return bottomTrackIndxs_; };

signals:
    void epochAdded(uint64_t indx);
    void positionAdded(uint64_t indx);
    void chartAdded(uint64_t indx);
    void attitudeAdded(uint64_t indx);
    void bottomTrackAdded(const QVector<int>& indx);

public slots:
    // Dataset
    void onAddedEpoch(uint64_t indx);
    void onAddedPosition(uint64_t indx);
    void onAddedChart(uint64_t indx);
    void onAddedAttitude(uint64_t indx);
    void onAddedBottomTrack(const QVector<int>& indx);

private:
    bool canEmitHorizon(bool beenChanged) const;

private:
    bool emitChanges_;
    bool isFileOpening_;
    bool isSeparateReading_;

    uint64_t epochIndx_;
    uint64_t positionIndx_;
    uint64_t chartIndx_;
    uint64_t attitudeIndx_;
    QVector<int> bottomTrackIndxs_;
};
