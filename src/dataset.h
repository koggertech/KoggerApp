#pragma once

#include <math.h>
#include <stdint.h>
#include <time.h>
#include <QObject>
#include <QVector>
#include <QVector3D>
#include <QReadWriteLock>

#include "black_stripes_processor.h"
#include "dataset_defs.h"
#include "data_interpolator.h"
#include "epoch.h"
#include "id_binnary.h"
#include "usbl_view.h"


class Dataset : public QObject
{
    Q_OBJECT

public:
    /*structures*/
    enum class DatasetState {
        kUndefined = 0,
        kFile,
        kConnection
    };
    enum class LlaRefState {
        kUndefined = 0,
        kSettings,
        kFile,
        kConnection
    };

    Q_PROPERTY(float boatLatitude             READ getBoatLatitude          NOTIFY lastPositionChanged)
    Q_PROPERTY(float boatLongitude            READ getBoatLongitude         NOTIFY lastPositionChanged)
    Q_PROPERTY(float distToContact            READ getDistToContact         NOTIFY lastPositionChanged)
    Q_PROPERTY(float angleToContact           READ getAngleToContact        NOTIFY lastPositionChanged)
    Q_PROPERTY(bool  isActiveContactIndxValid READ isValidActiveContactIndx NOTIFY activeContactChanged)
    Q_PROPERTY(bool  isBoatCoordinateValid    READ isValidBoatCoordinate    NOTIFY lastPositionChanged)
    Q_PROPERTY(float isLastDepthValid         READ isValidLastDepth         NOTIFY lastDepthChanged)
    Q_PROPERTY(float depth                    READ getLastDepth             NOTIFY lastDepthChanged)
    Q_PROPERTY(float isSpeedValid             READ isValidSpeed             NOTIFY speedChanged)
    Q_PROPERTY(float speed                    READ getSpeed                 NOTIFY speedChanged)

    /*methods*/
    Dataset();
    ~Dataset();

    void setState(DatasetState state);

#if defined(FAKE_COORDS)
    void setActiveZeroing(bool state);
#endif

    DatasetState getState() const;
    LLARef getLlaRef() const;
    void setLlaRef(const LLARef& val, LlaRefState state);

    inline int size() const {
        return pool_.size();
    }

    Epoch* fromIndex(int index_offset = 0) {
        int index = validIndex(index_offset);
        if(index >= 0) {
            return &pool_[index];
        }

        return NULL;
    }

    Epoch fromIndexCopy(int index_offset = 0) {
        QReadLocker rl(&poolMtx_);

        const int index = validIndex(index_offset);
        if (channelsSetup_.empty() || index < 0) {
            return Epoch{};
        }

        const Epoch &src = pool_.at(index);
        Epoch copy = src;

        return copy;
    }

    Epoch::Echogram fromIndexCopyEchogram(int index_offset, const ChannelId& channelId) {
        QReadLocker rl(&poolMtx_);

        const int currSize = pool_.size();
        if (channelsSetup_.empty() || currSize == 0)
            return {};

        int indx = validIndex(index_offset);
        if (indx == -1) {
            return {};
        }

        const Epoch &ep = pool_.at(indx);

        if (!ep.chartAvail(channelId, 0))
            return {};

        return ep.chartCopy(channelId, 0);
    }

    Epoch* last() {
        if(size() > 0) {
            return fromIndex(endIndex());
        }
        return addNewEpoch();
    }

    Epoch* lastlast() {
        if(size() > 1) {
            return fromIndex(endIndex()-1);
        }
        return NULL;
    }

    int endIndex() const {
        return size() - 1;
    }

    int validIndex(int index_offset = 0) {
        int index = index_offset;
        if(index >= size()) { index = endIndex(); }
        else if(index < 0) { index = -1; }
        return index;
    }

    void getMaxDistanceRange(float* from, float* to, const ChannelId& channel, uint8_t subAddressCh1, const ChannelId& channel2 = CHANNEL_NONE, uint8_t subAddressCh2 = 0);

    bool channelsListIsEmpty() const {
        QReadLocker locker(&lock_);

        return channelsSetup_.isEmpty();
    }

    QVector<DatasetChannel> channelsList() const {
        QReadLocker locker(&lock_);

        return channelsSetup_;
    }

    bool isContainsChannelInChannelSetup(const ChannelId& channelId) const {
        QReadLocker locker(&lock_);

        for (int16_t i = 0; i < channelsSetup_.size(); ++i) {
            if (channelsSetup_.at(i).channelId_ == channelId) {
                return true;
            }
        }

        return false;
    }

    int getLastBottomTrackEpoch() const;

    float getLastArtificalYaw();

    float getLastYaw() {
        return _lastYaw;
    }

    float getLastTemp() {
        return lastTemp_;
    }

    BottomTrackParam getBottomTrackParam() {
        QReadLocker rl(&lock_);

        return bottomTrackParam_;
    }

    BottomTrackParam* getBottomTrackParamPtr() {
        return &bottomTrackParam_;
    }

    BottomTrackParam& getBottomTrackParamRef() {
        return bottomTrackParam_;
    }

    std::tuple<ChannelId, uint8_t, QString> channelIdFromName(const QString& name) const;

    void setActiveContactIndx(int64_t indx);
    int64_t getActiveContactIndx() const;

public slots:
    friend class DataProcessor;
    void onSonarPosCanCalc(uint64_t indx);
    bool  isValidActiveContactIndx() const { return activeContactIndx_ != -1;  };
    bool  isValidBoatCoordinate() const    { return !qFuzzyIsNull(boatLatitute_) || !qFuzzyIsNull(boatLongitude_); };
    bool  isValidLastDepth() const         { return !qFuzzyIsNull(lastDepth_); };
    bool isValidSpeed() const              { return qFuzzyIsNull(speed_);      };
    float getBoatLatitude() const          { return boatLatitute_;             };
    float getBoatLongitude() const         { return boatLongitude_;            };
    float getDistToContact() const         { return distToActiveContact_;      };
    float getAngleToContact() const        { return angleToActiveContact_;     };
    float getLastDepth() const             { return lastDepth_;                };
    float getSpeed() const                 { return speed_;                    };
    void addEvent(int timestamp, int id, int unixt = 0);
    void addEncoder(float angle1_deg, float angle2_deg = NAN, float angle3_deg = NAN);
    void addTimestamp(int timestamp);

    //
    void setChartSetup (const ChannelId& channelId, uint16_t resol, uint16_t count, uint16_t offset);
    void setTranscSetup(const ChannelId& channelId, uint16_t freq, uint8_t pulse, uint8_t boost);
    void setSoundSpeed (const ChannelId& channelId, uint32_t soundSpeed);
    void setSonarOffset(float x, float y, float z);
    void setFixBlackStripesState(bool state);
    void setFixBlackStripesForwardSteps(int val);
    void setFixBlackStripesBackwardSteps(int val);
    void addChart(const ChannelId& channelId, const ChartParameters& chartParams, const QVector<QVector<uint8_t>>& data, float resolution, float offset);
    void rawDataRecieved(const ChannelId& channelId, RawData raw_data);
    void addDist(const ChannelId& channelId, int dist);
    void addRangefinder(const ChannelId& channelId, float distance);
    void addUsblSolution(IDBinUsblSolution::UsblSolution data);
    void addDopplerBeam(IDBinDVL::BeamSolution *beams, uint16_t cnt);
    void addDVLSolution(IDBinDVL::DVLSolution dvlSolution);
    void addAtt(float yaw, float pitch, float roll);
    void addPosition(double lat, double lon, uint32_t unix_time = 0, int32_t nanosec = 0);
    void addArtificalYaw();
    void addPositionRTK(Position position);

    void addDepth(float depth);

    void addGnssVelocity(double h_speed, double course);

//    void addDateTime(int year, );
    void addTemp(float temp_c);

    void mergeGnssTrack(QList<Position> track);

    void resetDataset();
    void resetRenderBuffers();
    void resetDistProcessing();

    void setChannelOffset(const ChannelId& channelId, float x, float y, float z);
    void spatialProcessing();

    void usblProcessing();
    QVector<QVector3D> beaconTrack() {
        return _beaconTrack;
    }

    QVector<QVector3D> beaconTrack1() {
        return _beaconTrack1;
    }

    void setScene3D(GraphicsScene3dView* scene3dViewPtr) { scene3dViewPtr_ = scene3dViewPtr; };

    void setRefPosition(int epoch_index);
    void setRefPosition(Epoch* ref_epoch);
    void setRefPosition(Position position);
    void setRefPositionByFirstValid();
    Epoch* getFirstEpochByValidPosition();

    QStringList channelsNameList();

    void onDistCompleted(int epIndx, const ChannelId& channelId, float dist);
    void onLastBottomTrackEpochChanged(const ChannelId& channelId, int val, const BottomTrackParam& btP, bool manual, bool redrawAll);

signals:
    // data horizon
    void epochAdded(uint64_t indx);
    void positionAdded(uint64_t indx);
    void chartAdded(uint64_t indx); // without ChartId
    void attitudeAdded(uint64_t indx);
    void bottomTrackAdded(uint64_t indx);
    //void interpYaw(int epIndx);
    //void interpPos(int epIndx);
    void dataUpdate();
    void bottomTrackUpdated(const ChannelId& channelId, int lEpoch, int rEpoch, bool manual, bool redrawAll);
    void updatedLlaRef();
    void channelsUpdated();
    void redrawEpochs(const QSet<int>& indxs);
    void lastPositionChanged();
    void activeContactChanged();
    void lastDepthChanged();
    void speedChanged();

protected:

    int lastEventTimestamp = 0;
    int lastEventId = 0;
    float _lastEncoder = 0;

#if defined(FAKE_COORDS)
    bool activeZeroing_ = false;
    uint64_t testTime_ = 1740466541;
#endif

    DatasetChannel firstChannelId_ = DatasetChannel(); // TODO: temp solution
    QVector<DatasetChannel> channelsSetup_;

    void validateChannelList(const ChannelId& channelId, uint8_t subChannelId);

    QVector<QVector3D> _beaconTrack;
    QVector<QVector3D> _beaconTrack1;

    QMap<int, UsblView::UsblObjectParams> tracks;

    //enum {
    //    AutoRangeNone,
    //    AutoRangeLast,
    //    AutoRangeMax,
    //    AutoRangeMaxVis
    //} _autoRange = AutoRangeLast;


    QVector<Epoch> pool_;

    float lastAYaw_ = NAN;
    float _lastYaw = NAN, _lastPitch = NAN, _lastRoll = NAN;
    float lastTemp_ = NAN;

    Epoch* addNewEpoch();

    GraphicsScene3dView* scene3dViewPtr_ = nullptr;

private:
    friend class DataInterpolator;

    /*methods*/
    LlaRefState getCurrentLlaRefState() const;
    bool shouldAddNewEpoch(const ChannelId& channelId, uint8_t numSubChannels) const;
    void updateEpochWithChart(const ChannelId& channelId, const ChartParameters& chartParams, const QVector<QVector<uint8_t>>& data, float resolution, float offset);
    void setLastDepth(float val);

    /*data*/
    mutable QReadWriteLock lock_;
    mutable QReadWriteLock poolMtx_;

    LLARef _llaRef;
    LlaRefState llaRefState_ = LlaRefState::kUndefined;
    DatasetState state_ = DatasetState::kUndefined;
    DataInterpolator interpolator_;
    int lastBottomTrackEpoch_;
    BottomTrackParam bottomTrackParam_;
    QMap<ChannelId, RecordParameters> usingRecordParameters_;
    BlackStripesProcessor* bSProc_;
    QMap<ChannelId, int> lastAddChartEpochIndx_;
    QSet<ChannelId> channelsToResizeEthData_;

    // for GUI
    QList<QString> channelsNames_;
    QList<ChannelId> channelsIds_;
    QList<uint8_t> subChannelIds_;
    int64_t activeContactIndx_  = -1;
    float boatLatitute_         = 0.0f;
    float boatLongitude_        = 0.0f;
    float distToActiveContact_  = 0.0f;
    float angleToActiveContact_ = 0.0f;
    float lastDepth_            = 0.0f;
    float speed_                = 0.0f;
    QVector3D sonarOffset_;
    uint64_t sonarPosIndx_;
};
