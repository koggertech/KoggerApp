#pragma once

#include <QObject>


class DataHorizon : public QObject
{
    Q_OBJECT

public:
    DataHorizon();

    void clear();

    void setEmitChanges(bool state);
    void setIsFileOpening(bool state);
    void setIsAttitudeExpected(bool state);

    uint64_t getEpochSize() const { return epochIndx_; };
    uint64_t getPositionIndx() const { return positionIndx_; };
    uint64_t getChartIndx() const { return chartIndx_; };
    uint64_t getAttitudeIndx() const { return attitudeIndx_; };
    uint64_t getBottomTrackIndx() const { return bottomTrackIndx_; };

signals:
    void epochAdded(uint64_t indx);
    void positionAdded(uint64_t indx);
    void chartAdded(uint64_t indx);
    void attitudeAdded(uint64_t indx);
    void bottomTrackAdded(uint64_t indx);
    void bottomTrack3DAdded(const QVector<int>& epIndxs, const QVector<int>& vertIndxs, bool isManual);
    void mosaicCanCalc(uint64_t indx);
    void sonarPosCanCalc(uint64_t indx);

public slots:
    // Dataset
    void onAddedEpoch(uint64_t indx);
    void onAddedPosition(uint64_t indx);
    void onAddedChart(uint64_t indx);
    void onAddedAttitude(uint64_t indx);
    void onAddedBottomTrack(uint64_t indx); // from bottom track algorithm
    void onAddedBottomTrack3D(const QVector<int>& epIndxs, const QVector<int>& vertIndxs, bool isManual); // from 2D (editing), 3D

private:
    bool canEmitHorizon(bool beenChanged) const;
    void tryCalcAndEmitMosaicIndx();
    void tryCalcAndEmitSonarPosIndx();

private:
    bool emitChanges_;
    bool isFileOpening_;
    bool isSeparateReading_;
    bool isAttitudeExpected_;

    uint64_t epochIndx_;
    uint64_t positionIndx_;
    uint64_t chartIndx_;
    uint64_t attitudeIndx_;
    uint64_t bottomTrackIndx_;
    uint64_t mosaicIndx_;
    uint64_t sonarIndx_;
};
