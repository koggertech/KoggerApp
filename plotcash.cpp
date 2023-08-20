#include "plotcash.h"
#include <QPainterPath>

#include <core.h>
extern Core core;

Epoch::Epoch() {
    _charts.clear();
    flags.distAvail = false;
}

void Epoch::setEvent(int timestamp, int id, int unixt) {
    _eventTimestamp_us = timestamp;
    _eventUnix = unixt;
    _eventId = id;
    flags.eventAvail = true;
}

void Epoch::setEncoder(float encoder) {
    _encoder.validMask |= 1;
    _encoder.e1 = encoder;
    flags.encoderAvail = true;
}


void Epoch::setChart(int16_t channel, QVector<int16_t> data, float resolution, int offset) {
    _charts[channel].amplitude = data;
    _charts[channel].resolution = resolution;
    _charts[channel].offset = offset;
    _charts[channel].type = 1;
}

void Epoch::setIQ(QByteArray data, uint8_t type) {
    _iq = data;
    _iq_type = type;
    flags.iqAvail = true;

    //    doppler.velocityX = dopplerProcessing(16*15/2, 16*36/2, 2);
    //    doppler.isAvai = true;
}

void Epoch::setDist(int dist) {
    m_dist = dist;
    flags.distAvail = true;
}

void Epoch::setRangefinder(int channel, float distance) {
    _rangeFinders[channel] = distance;
}

void Epoch::setDopplerBeam(IDBinDVL::BeamSolution *beams, uint16_t cnt) {
    for(uint16_t i = 0; i < cnt; i++) {
        _dopplerBeams[i] = beams[i];
    }
    //    setDist(beams[0].distance*1000.0f);
    _dopplerBeamCount = cnt;
}

void Epoch::setDVLSolution(IDBinDVL::DVLSolution dvlSolution) {
    _dvlSolution = dvlSolution;
    flags.isDVLSolutionAvail = true;
}

void Epoch::setPositionLLA(double lat, double lon, LLARef* ref, uint32_t unix_time, int32_t nanosec) {
    _positionGNSS.time.unix = unix_time;
    _positionGNSS.time.nanoSec = nanosec;
    _positionGNSS.lla.latitude = lat;
    _positionGNSS.lla.longitude = lon;
    flags.posAvail = true;
}

void Epoch::setExternalPosition(Position position) {
    _positionExternal = position;
}

void Epoch::setTime(DateTime time) {

}

void Epoch::setTime(int year, int month, int day, int hour, int min, int sec, int nanosec) {

}


void Epoch::setTemp(float temp_c) {
    m_temp_c = temp_c;
    flags.tempAvail = true;
}

void Epoch::setEncoders(int16_t enc1, int16_t enc2, int16_t enc3, int16_t enc4, int16_t enc5, int16_t enc6) {
    _encoder.e1 = enc1;
    _encoder.e2 = enc2;
    _encoder.e3 = enc3;
    _encoder.e4 = enc4;
    _encoder.e5 = enc5;
    _encoder.e6 = enc6;
    _encoder.validMask = (uint16_t)0x111111;
}

void Epoch::setAtt(float yaw, float pitch, float roll) {
    _attitude.yaw = yaw;
    _attitude.pitch = pitch;
    _attitude.roll = roll;
}

void Epoch::doBottomTrack2D(DataChart &chart, bool is_update_dist) {
}

void Epoch::doBottomTrackSideScan(DataChart &chart, bool is_update_dist) {
}

Dataset::Dataset() {
    resetDataset();
}

void Dataset::getMaxDistanceRange(float *from, float *to, int channel1, int channel2) {
    const int sz = size();
    float channel1_max = 0;
    float channel2_max = 0;
    for(int iepoch = 0; iepoch < sz; iepoch++) {
        Epoch* epoch = fromIndex(iepoch);
        if(epoch != NULL) {
            if(epoch->chartAvail(channel1)) {
                float range = epoch->chart(channel1)->range();
                if(channel1_max < range) {
                    channel1_max = range;
                }
            }

            if(epoch->chartAvail(channel2)) {
                float range = epoch->chart(channel2)->range();
                if(channel2_max < range) {
                    channel2_max = range;
                }
            }
        }
    }

    if(channel1_max > 0) {
        if(channel2_max > 0) {
            *from = -channel1_max;
            *to = channel2_max;
        } else {
            *from = 0;
            *to = channel1_max;
        }

    } else {
        *from = NAN;
        *to = NAN;
    }
}

void Dataset::set3DSceneModel(const ModelPointer pModel)
{
    mp3DSceneModel = pModel;
}

void Dataset::addEvent(int timestamp, int id, int unixt) {
    lastEventTimestamp = timestamp;
    lastEventId = id;

    //    if(poolLastIndex() < 0) {
    makeNewEpoch();
    //    }

    _pool[endIndex()].setEvent(timestamp, id, unixt);
}

void Dataset::addEncoder(float encoder) {
    _lastEncoder = encoder;
    if(endIndex() < 0) {
        makeNewEpoch();
    }
    //    poolAppend();
    _pool[endIndex()].setEncoder(_lastEncoder);
}

void Dataset::addTimestamp(int timestamp) {
}

void Dataset::addChart(int16_t channel, QVector<int16_t> data, float resolution, float offset) {
    if(data.size() <= 0 || resolution == 0) { return; }

    int pool_index = endIndex();

    if(pool_index < 0
            //             || _pool[pool_index].eventAvail() == false
            || _pool[pool_index].chartAvail(channel)) {
        makeNewEpoch();
        pool_index = endIndex();
    }

    _pool[endIndex()].setChart(channel, data, resolution, offset);

    validateChannelList(channel);
}

void Dataset::addIQ(QByteArray data, uint8_t type) {
    int pool_index = endIndex();

    if(pool_index < 0 || _pool[pool_index].isIqAvail()) {
        makeNewEpoch();
        pool_index = endIndex();
    }

    _pool[endIndex()].setIQ(data, type);
    //    updateImage(true);
}

void Dataset::addDist(int dist) {
    int pool_index = endIndex();
    if(pool_index < 0 || (_pool[pool_index].eventAvail() == false && _pool[pool_index].chartAvail() == false) || _pool[pool_index].distAvail() == true) {
        makeNewEpoch();
        pool_index = endIndex();
    }

    _pool[endIndex()].setDist(dist);
}

void Dataset::addDopplerBeam(IDBinDVL::BeamSolution *beams, uint16_t cnt) {
    int pool_index = endIndex();

    makeNewEpoch();
    pool_index = endIndex();

    _pool[endIndex()].setDopplerBeam(beams, cnt);
}

void Dataset::addDVLSolution(IDBinDVL::DVLSolution dvlSolution) {
    int pool_index = endIndex();

    if(pool_index < 0 || (_pool[pool_index].isDopplerBeamAvail() == false)) {
        makeNewEpoch();
        pool_index = endIndex();
    }

    _pool[endIndex()].setDVLSolution(dvlSolution);
}

void Dataset::addAtt(float yaw, float pitch, float roll) {
    int pool_index = endIndex();
    if(pool_index < 0) {
        makeNewEpoch();
        pool_index = endIndex();
    }
    _pool[endIndex()].setAtt(yaw, pitch, roll);
    _lastYaw = yaw;
    _lastPitch = pitch;
    _lastRoll = roll;
}

void Dataset::addPosition(double lat, double lon, uint32_t unix_time, int32_t nanosec) {

    int pool_index = endIndex();
    if(pool_index < 0) {
        makeNewEpoch();
        pool_index = endIndex();
    }

    _pool[pool_index].setPositionLLA(lat, lon, &_llaRef, unix_time, nanosec);
}

void Dataset::addTemp(float temp_c) {

    lastTemperature = temp_c;

    int pool_index = endIndex();
    if(pool_index < 0) {
        makeNewEpoch();
        pool_index = endIndex();
    }
    _pool[pool_index].setTemp(temp_c);
}

void Dataset::mergeGnssTrack(QList<Position> track) {
    const int64_t max_difference_ns = 1e9;
    const int psize = size();
    const int tsize = track.size();
    int track_pos_save = 0;

    for(int iepoch = 0; iepoch < psize; iepoch++) {
        Epoch* epoch =  fromIndex(iepoch);
        Position p_internal = epoch->getPositionGNSS();
        int64_t internal_ns  = p_internal.time.unix*1e9+p_internal.time.nanoSec;

        if(internal_ns > 0) {
            int64_t min_dif_ns = max_difference_ns;
            int min_ind = -1;
            for(int track_pos = track_pos_save; track_pos < tsize;track_pos++) {
                int64_t track_ns  = track[track_pos].time.unix*1e9+track[track_pos].time.nanoSec;
                if(track_ns > 0) {
                    int64_t dif_ns = track_ns - internal_ns;
                    if(min_dif_ns > abs(dif_ns)) {
                        min_dif_ns = abs(dif_ns);
                        min_ind = track_pos;
                    }

                    if(dif_ns > max_difference_ns) { break; }
                }
            }

            if(min_ind > 0) {
                track_pos_save = min_ind;
                epoch->setExternalPosition(track[min_ind]);
            }
        }
    }
}


void Dataset::resetDataset() {
    _pool.clear();
    _llaRef.isInit = false;
    _channelsSetup.clear();
    resetDistProcessing();

    clearTrack();
}

void Dataset::resetDistProcessing() {
    int pool_size = size();
    for(int i = 0; i < pool_size; i++) {
//        Epoch* dataset = fromIndex(i);
//        dataset->resetDistProccesing();
    }
}

void Dataset::bottomTrackProcessing(int channel1, int channel2, BottomTrackParam param) {
    if(param.indexFrom < 0 || param.indexTo < 0) { return; }

    int epoch_min_index = param.indexFrom - param.windowSize/2;

    if(epoch_min_index < 0) {
        epoch_min_index = 0;
    }

    int epoch_max_index = param.indexTo + param.windowSize/2;
    if(epoch_max_index >= size()) {
        epoch_max_index = size();
    }

    QVector<int32_t> summ;

    float gain_slope = param.gainSlope;
    float threshold = param.threshold;

    int istart = 4;
    int init_win = 6;
    int scale_win = 20;

    int16_t c1 = -6, c2 = 6, c3 = 4, c4 = 2, c5 = 0;
    float s2 = 1.04f, s3 = 1.06f, s4 = 1.10f, s5 = 1.15f;
    float t1 = 1.07;

    if(param.preset == BottomTrackSideScan) {
        istart = 4;
        init_win = 6;
        scale_win = 12;

        c1 = -3, c2 = -3, c3 = 4, c4 = 2, c5 = 1;
        s2 = 1.1f, s3 = 1.2f, s4 = 1.3f, s5 = 1.4f;
        t1 = 1.2;
    }

    const int gain_slope_inv = 1000/(gain_slope);
    const int threshold_int = 10*gain_slope_inv*1000*threshold;

    typedef  struct {
        float min = NAN, max = NAN;
    } EpochConstrants;

    QVector<QVector<int32_t>> cash(param.windowSize);
    QVector<EpochConstrants> constr(param.windowSize);

    QVector<float> bottom_track(epoch_max_index - epoch_min_index);
    bottom_track.fill(NAN);

    int epoch_counter = 0;

    for(int iepoch = epoch_min_index; iepoch < epoch_max_index; iepoch++) {
        Epoch* epoch = fromIndex(iepoch);
        if(epoch == NULL || !epoch->chartAvail(channel1)) { continue; }

        epoch_counter++;

        Epoch::DataChart* chart = epoch->chart(channel1);

        int16_t* data = (int16_t*)chart->amplitude.constData();
        const int data_size = chart->amplitude.size();

        int cash_ind = (epoch_counter-1)%param.windowSize;

        int back_cash_ind = ((epoch_counter)%param.windowSize);
        int32_t* back_cash_data = (int32_t*)cash[back_cash_ind].constData();
        const int back_cash_size = cash[back_cash_ind].size();

        int32_t* summ_data = (int32_t*)summ.constData();

        if(epoch_counter >= param.windowSize) {
            for(int i = istart; i < back_cash_size; i++) { summ_data[i] -= back_cash_data[i]; }
        }

        if(cash[cash_ind].size() != data_size) {
            cash[cash_ind].resize(data_size);
        }

        int32_t* cash_data = (int32_t*)cash[cash_ind].constData();


        int16_t* data_from = &data[istart];
        int16_t* data_to = &data[(istart+init_win)];

        int data_acc = 0;
        for(int idata = istart; idata < (istart+init_win); idata++) {
            data_acc += data[idata];
        }

        int avrg_range = init_win;
        for(int idata = istart; (idata + avrg_range) < data_size; idata++) {
            data_acc -= *data_from; data_from++;
            data_acc += *data_to; data_to++;
            while((idata+(init_win*scale_win)) >= ((avrg_range = data_to - data_from)*scale_win)) {
                data_acc += *data_to; data_to++;
            }
            cash_data[idata] = 10*data_acc / (avrg_range);
        }

        const int data_conv_size = data_size - avrg_range;
        for(int idata = istart; ; idata++) {
            const float fidata = idata + init_win+1;
            int di1 =  idata;
            int di2 =  fidata*s2;
            int di3 =  fidata*s3;
            int di4 =  fidata*s4;
            int di5 =  fidata*s5;

            if(di5 >= data_conv_size) { break; }

            int calc = (c1*cash_data[di1] + c2*cash_data[di2] + c3*cash_data[di3] + c4*cash_data[di4] + c5*cash_data[di5]);
            calc = calc*gain_slope_inv + calc*(idata);

            cash_data[idata] = calc;
        }

        const int col_size = cash[cash_ind].size();
        if(summ.size() < col_size) { summ.resize(col_size); }
        summ_data = (int32_t*)summ.constData();
        for(int i = istart; i < col_size; i++) { summ_data[i] += cash_data[i]; }


        constr[cash_ind].min = chart->bottomProcessing.getMin();
        constr[cash_ind].max = chart->bottomProcessing.getMax();

        const int win_center_index = (epoch_counter - 1 + param.windowSize/2)%param.windowSize;

        float search_from_distance = param.minDistance;
        float search_to_distance = param.maxDistance;

        if(search_from_distance < constr[win_center_index].min) {
            search_from_distance = constr[win_center_index].min;
        }

        if(search_to_distance > constr[win_center_index].max) {
            search_to_distance = constr[win_center_index].max;
        }

        int start_search_index = search_from_distance/t1/chart->resolution;
        int end_search_index = search_to_distance/t1/chart->resolution;

        if(start_search_index < 0) { start_search_index = 0; }
        if(start_search_index > summ.size()) { start_search_index = summ.size(); }

        if(end_search_index < 0) { end_search_index = 0; }
        if(end_search_index > summ.size()) { end_search_index = summ.size(); }


        int max_val = threshold_int*param.windowSize;
        int max_ind = -1;
        for(int i = start_search_index; i < end_search_index ; i++) {
            if(max_val < summ_data[i]) {
                max_val = summ_data[i];
                max_ind = i;
            }
        }

        if(max_ind > 0) {
            float distance = ((max_ind+init_win+1)*t1)*chart->resolution;

            if(epoch_counter >= param.windowSize) {
                if(param.verticalGap > 0) {
                    int32_t* center_cash_data = (int32_t*)cash[win_center_index].constData();
                    const int center_cash_size = cash[win_center_index].size();

                    int start_gap_index = max_ind*(1.0f-param.verticalGap);
                    int end_gap_index = max_ind*(1.0f+param.verticalGap);

                    if(start_gap_index < start_search_index) { start_gap_index = start_search_index; }
                    if(start_gap_index > center_cash_size) { start_gap_index = center_cash_size; }

                    if(end_gap_index < 0) { end_gap_index = 0; }
                    if(end_gap_index > center_cash_size) { end_gap_index = center_cash_size; }
                    if(end_gap_index > end_search_index) { end_gap_index = end_search_index; }

                    int max_gap_val = 0;
                    int max_gap_ind = -1;
                    for(int i = start_gap_index; i < end_gap_index ; i++) {
                        if(max_gap_val < center_cash_data[i]) {
                            max_gap_val = center_cash_data[i];
                            max_gap_ind = i;
                        }
                    }


                    if(max_gap_ind > 0) {
                        distance = ((max_gap_ind+init_win+1)*t1)*chart->resolution;
                    }
                }

                bottom_track[iepoch - epoch_min_index - param.windowSize/2] = distance;
            } else {
                bottom_track[iepoch - epoch_min_index - epoch_counter/2] = distance;
            }
        }
    }


    int epoch_start_index = param.indexFrom;

    if(epoch_start_index < 0) {
        epoch_start_index = 0;
    }

    int epoch_stop_index = param.indexTo;
    if(epoch_stop_index >= size()) {
        epoch_stop_index = size();
    }

    for(int iepoch = epoch_start_index; iepoch < epoch_stop_index; iepoch++) {
        Epoch* epoch = fromIndex(iepoch);

        if(epoch == NULL) { continue; }

        if(epoch->chartAvail(channel1)) {
            Epoch::DataChart* chart = epoch->chart(channel1);
            if(chart->bottomProcessing.source < Epoch::DistProcessing::DistanceSourceDirectHand) {
                float dist = bottom_track[iepoch - epoch_min_index];
                chart->bottomProcessing.setDistance(dist, Epoch::DistProcessing::DistanceSourceProcessing);
            }
        }

        if(epoch->chartAvail(channel2)) {
            Epoch::DataChart* chart = epoch->chart(channel2);
            if(chart->bottomProcessing.source < Epoch::DistProcessing::DistanceSourceDirectHand) {
                float dist = bottom_track[iepoch - epoch_min_index];
                chart->bottomProcessing.setDistance(dist, Epoch::DistProcessing::DistanceSourceProcessing);
            }
        }
    }

    setChannelOffset(channel1, param.offset.x, param.offset.y, param.offset.z);
    spatialProcessing();

    updateTrack(true);
    updateRender3D();
}

void Dataset::spatialProcessing() {
    QList<DatasetChannel> ch_list = channelsList().values();
    for (const auto& channel : ch_list) {
        int ich = channel.channel;

        for(int iepoch = 0; iepoch < size(); iepoch++) {
            Epoch* epoch = fromIndex(iepoch);
            if(epoch == NULL) { continue; }

            Position ext_pos = epoch->getExternalPosition();

            if(epoch->chartAvail(ich)) {
                Epoch::DataChart* data = epoch->chart(ich);

                if(data == NULL) { continue; }

                if(ext_pos.ned.isValid()) {
                    ext_pos.ned.d += channel.localPosition.z;
                }

                if(ext_pos.lla.isValid()) {
                    ext_pos.lla.altitude -= channel.localPosition.z;
                }

                data->sensorPosition = ext_pos;

                if(ext_pos.ned.isValid()) {
                    ext_pos.ned.d += data->bottomProcessing.getDistance();
                }

                if(ext_pos.lla.isValid()) {
                    ext_pos.lla.altitude -= data->bottomProcessing.getDistance();
                }

                data->bottomProcessing.bottomPoint = ext_pos;
            }
        }
    }
}

void Dataset::clearTrack() {
    _lastTrackEpoch = 0;
    _bottomTrack.clear();
}

void Dataset::updateTrack(bool update_all) {
    const int to_size = size();
    int from_index = 0;

    if(update_all) {
        _bottomTrack.clear();
    } else {
        from_index = _lastTrackEpoch;
    }

    for(int i = from_index; i < to_size; i+=1) {
        Epoch* epoch = fromIndex(i);
        Position pos = epoch->getPositionGNSS();

        if(pos.lla.isCoordinatesValid() && !pos.ned.isCoordinatesValid()) {
            if(!_llaRef.isInit) {
                _llaRef = LLARef(pos.lla);
            }
            pos.LLA2NED(&_llaRef);
        }

        float distance = epoch->distProccesing(1);
        if(pos.ned.isCoordinatesValid() && isfinite(distance)) {
            _bottomTrack.append(QVector3D(pos.ned.n,pos.ned.e, -distance));
        }
    }

    _lastTrackEpoch = to_size;

    if (mp3DSceneModel) {
        mp3DSceneModel->setBottomTrack(_bottomTrack);
    }
}

QStringList Dataset::channelsNameList() {
    QStringList ch_names;
    QList<DatasetChannel> ch_list = channelsList().values();
    ch_names.append(QString("None"));
    for (const auto& channel : ch_list) {
        ch_names.append(QString("%1").arg(channel.channel));
    }
    return ch_names;

}
