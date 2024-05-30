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
    _time = DateTime(unixt, timestamp*1000);
    flags.eventAvail = true;
}

void Epoch::setEncoder(float encoder) {
    _encoder.validMask |= 1;
    _encoder.e1 = encoder;
    flags.encoderAvail = true;
}


void Epoch::setChart(int16_t channel, QVector<uint8_t> data, float resolution, int offset) {
    _charts[channel].amplitude = data;
    _charts[channel].resolution = resolution;
    _charts[channel].offset = offset;
    _charts[channel].type = 1;
}

// void Epoch::setComplexSignal16(int channel, QVector<Complex16> data) {
//     // _complex[channel].data = QByteArray((const char*)data.constData(), data.size()*sizeof(Complex16));
//     // _complex[channel].type = 2;
// }

void Epoch::setComplexF(int channel, ComplexSignal signal) {
    _complex[channel] = signal;
}
void Epoch::setDist(int dist) {
    _rangeFinders[0] = dist*0.001;
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
    Q_UNUSED(ref);

    _positionGNSS.time = DateTime(unix_time, nanosec);
    _positionGNSS.lla.latitude = lat;
    _positionGNSS.lla.longitude = lon;

    flags.posAvail = true;
}

void Epoch::setPositionLLA(Position position) {
    _positionGNSS = position;
    flags.posAvail = true;
}

void Epoch::setExternalPosition(Position position) {
    _positionExternal = position;
}

void Epoch::setPositionRef(LLARef* ref) {
    if(ref != NULL && ref->isInit) {
        _positionGNSS.LLA2NED(ref);
    }
}

void Epoch::setGnssVelocity(double h_speed, double course) {
    _GnssData.hspeed = h_speed;
    _GnssData.course = course;
}

void Epoch::setTime(DateTime time) {
    Q_UNUSED(time);
}

void Epoch::setTime(int year, int month, int day, int hour, int min, int sec, int nanosec) {
    Q_UNUSED(year);
    Q_UNUSED(month);
    Q_UNUSED(day);
    Q_UNUSED(hour);
    Q_UNUSED(min);
    Q_UNUSED(sec);
    Q_UNUSED(nanosec);
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

void Epoch::doBottomTrack2D(Echogram &chart, bool is_update_dist) {
    Q_UNUSED(chart);
    Q_UNUSED(is_update_dist);
}

void Epoch::doBottomTrackSideScan(Echogram &chart, bool is_update_dist) {
    Q_UNUSED(chart);
    Q_UNUSED(is_update_dist);
}

void Epoch::moveComplexToEchogram(float offset_m) {
    for (auto i = _complex.cbegin(), end = _complex.cend(); i != end; ++i) {
        // cout << qPrintable(i.key()) << ": " << i.value() << endl;
        QVector<ComplexF> data = i.value().data;


        int size = data.size();
        ComplexF* compelex_data = data.data();

        QVector<uint8_t> chart(size);
        uint8_t* chart_data = chart.data();

        for(int i  = 0; i < size; i++) {
            float amp = (compelex_data[i].logPow() - 86)*2.5;

            if(amp < 0) {
                amp = 0;
            } else if(amp > 255) {
                amp = 255;
            }
            chart_data[i] = amp;
        }

        setChart(i.key(), chart, 1500.0f/i.value().sampleRate, offset_m);
    }
}

Dataset::Dataset() : lastBoatTrackEpoch_(0), lastBottomTrackEpoch_(0) {
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

QVector<QVector3D> Dataset::boatTrack() const
{
    return _boatTrack;
}

int Dataset::getLastBottomTrackEpoch() const
{
    return lastBottomTrackEpoch_;
}

void Dataset::addEvent(int timestamp, int id, int unixt) {
    lastEventTimestamp = timestamp;
    lastEventId = id;

    //    if(poolLastIndex() < 0) {
    addNewEpoch();
    //    }

    _pool[endIndex()].setEvent(timestamp, id, unixt);
    emit dataUpdate();
}

void Dataset::addEncoder(float encoder) {
    _lastEncoder = encoder;
    if(endIndex() < 0) {
        addNewEpoch();
    }
    //    poolAppend();
    _pool[endIndex()].setEncoder(_lastEncoder);
    emit dataUpdate();
}

void Dataset::addTimestamp(int timestamp) {
    Q_UNUSED(timestamp);
}

void Dataset::addChart(int16_t channel, QVector<uint8_t> data, float resolution, float offset) {
    if(data.size() <= 0 || resolution == 0) { return; }

    int pool_index = endIndex();

    if(pool_index < 0
            //             || _pool[pool_index].eventAvail() == false
            || _pool[pool_index].chartAvail(channel)) {
        addNewEpoch();
        pool_index = endIndex();
    }

    _pool[endIndex()].setChart(channel, data, resolution, offset);

    validateChannelList(channel);

    emit dataUpdate();
}

void Dataset::rawDataRecieved(RawData raw_data) {
    Epoch* last_epoch = last();

    RawData::RawDataHeader header = raw_data.header;

    ComplexF* compelex_data = (ComplexF*)raw_data.data.data();
    int16_t* real16_data = (int16_t*)raw_data.data.data();

    int size = raw_data.data.size()/(header.dataSize + 1)/header.channelCount;

    if(header.localOffset == 0) {
        float offset_m = 0;
        // if(last_epoch->isUsblSolutionAvailable()) {
        //     offset_m = last_epoch->usblSolution().distance_m;
        //     offset_m -= (last_epoch->usblSolution().carrier_counter - header.globalOffset)*1500.0f/header.sampleRate;
        // }
        last_epoch->moveComplexToEchogram(offset_m);
        last_epoch = addNewEpoch();
        ComplexSignals compex_signals = last_epoch->complexSignals();

        for(int ich = 0; ich < header.channelCount; ich++) {
            ComplexSignal signal = compex_signals[ich];

            signal.globalOffset = header.globalOffset;
            signal.sampleRate = header.sampleRate;

            signal.data.resize(size);

            ComplexF* signal_data = signal.data.data();

            if(header.dataType == 0) {
                signal.isComplex = true;

                for(int i  = 0; i < size; i++) {
                    signal_data[i] = compelex_data[i*header.channelCount + ich];
                }
            } else if(header.dataType == 1) {
                signal.isComplex = false;

                for(int i  = 0; i < size; i++) {
                    signal_data[i] = ComplexF(real16_data[i*header.channelCount + ich], 0);
                }
            }

            last_epoch->setComplexF(ich, signal);
            validateChannelList(ich);
        }
    } else {
        ComplexSignals compex_signals = last_epoch->complexSignals();

        for(int ich = 0; ich < header.channelCount; ich++) {
            ComplexSignal signal = compex_signals[ich];
            uint32_t inbuf_localOffset = signal.data.size();
            signal.data.resize(header.localOffset + size);

            // signal.data.fill(ComplexF(0,0))

            if(inbuf_localOffset == header.localOffset) {
                ComplexF* signal_data = signal.data.data() + inbuf_localOffset;
                if(header.dataType == 0) {
                    for(int i  = 0; i < size; i++) {
                        signal_data[i] = compelex_data[i*header.channelCount + ich];
                    }
                } else if(header.dataType == 1) {
                    for(int i  = 0; i < size; i++) {
                        signal_data[i] = ComplexF(real16_data[i*header.channelCount + ich], 0);
                    }
                }
            } else {
                qDebug("raw data has broken");
            }


            last_epoch->setComplexF(ich, signal);
        }
    }

    emit dataUpdate();
}

void Dataset::addDist(int dist) {
    int pool_index = endIndex();
    if(pool_index < 0 || _pool[pool_index].distAvail() == true) {
        addNewEpoch();
        pool_index = endIndex();
    }

    _pool[endIndex()].setDist(dist);
    emit dataUpdate();
}

void Dataset::addUsblSolution(IDBinUsblSolution::UsblSolution data) {
    int pool_index = endIndex();
    if(pool_index < 0 || _pool[pool_index].isUsblSolutionAvailable() == true) {
        addNewEpoch();
        pool_index = endIndex();
    }

    _pool[endIndex()].set(data);
    emit dataUpdate();
}

void Dataset::addDopplerBeam(IDBinDVL::BeamSolution *beams, uint16_t cnt) {
    int pool_index = endIndex();

    if(pool_index < 0 || (_pool[pool_index].isDopplerBeamAvail() == true)) { //
        addNewEpoch();
    }

    pool_index = endIndex();

    _pool[endIndex()].setDopplerBeam(beams, cnt);
    emit dataUpdate();
}

void Dataset::addDVLSolution(IDBinDVL::DVLSolution dvlSolution) {
    int pool_index = endIndex();

    if(pool_index < 0 || (_pool[pool_index].isDopplerBeamAvail() == false)) { //
        addNewEpoch();
        pool_index = endIndex();
    }

    _pool[endIndex()].setDVLSolution(dvlSolution);
    emit dataUpdate();
}

void Dataset::addAtt(float yaw, float pitch, float roll) {
    Epoch* last_epoch = last();
    if(!last_epoch->isAttAvail()) {
        last_epoch->setAtt(yaw, pitch, roll);
    }

    _lastYaw = yaw;
    _lastPitch = pitch;
    _lastRoll = roll;
    emit dataUpdate();
}

void Dataset::addPosition(double lat, double lon, uint32_t unix_time, int32_t nanosec) {
    Epoch* last_epoch = last();

    Position pos;
    pos.lla = LLA(lat, lon);
    pos.time = DateTime(unix_time, nanosec);

    if(pos.lla.isCoordinatesValid()) {
        if(last_epoch->getPositionGNSS().lla.isCoordinatesValid()) {
            last_epoch = addNewEpoch();
        }

        if(!_llaRef.isInit) {
            _llaRef = LLARef(pos.lla);
        }

        last_epoch->setPositionLLA(pos);
        last_epoch->setPositionRef(&_llaRef);
        _lastPositionGNSS = pos;
    }

    emit dataUpdate();
    updateBoatTrack();
}

void Dataset::addGnssVelocity(double h_speed, double course) {
    int pool_index = endIndex();
    if(pool_index < 0) {
        addNewEpoch();
        pool_index = endIndex();
    }

//    if(isfinite(_pool[pool_index].gnssHSpeed())) {
//        makeNewEpoch();
//        pool_index = endIndex();
//    }


    _pool[pool_index].setGnssVelocity(h_speed, course);
    emit dataUpdate();
}

void Dataset::addTemp(float temp_c) {

    lastTemperature = temp_c;

    int pool_index = endIndex();
    if(pool_index < 0) {
        addNewEpoch();
        pool_index = endIndex();
    }
    _pool[pool_index].setTemp(temp_c);
    emit dataUpdate();
}

void Dataset::mergeGnssTrack(QList<Position> track) {
    const int64_t max_difference_ns = 1e9;
    const int psize = size();
    const int tsize = track.size();
    int track_pos_save = 0;
    volatile int sync_count = 0;

    for(int iepoch = 0; iepoch < psize; iepoch++) {
        Epoch* epoch =  fromIndex(iepoch);
        Position p_internal = epoch->getPositionGNSS();

        DateTime time_epoch = *epoch->time();
        if(time_epoch.sec > 0) {
            p_internal.time = *epoch->time();
            p_internal.time.sec -= 18;
        }

        int64_t internal_ns  = p_internal.time.sec*1e9+p_internal.time.nanoSec;


        if(internal_ns > 0) {
            int64_t min_dif_ns = max_difference_ns;
            int min_ind = -1;
            for(int track_pos = track_pos_save; track_pos < tsize;track_pos++) {
                int64_t track_ns  = track[track_pos].time.sec*1e9+track[track_pos].time.nanoSec;
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
                sync_count++;
            }
        }
    }
    emit dataUpdate();
}


void Dataset::resetDataset() {
    _pool.clear();
    _llaRef.isInit = false;
    _channelsSetup.clear();
    lastBottomTrackEpoch_ = 0;
    resetDistProcessing();

    clearBoatTrack();
    emit dataUpdate();
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



    if(param.preset == BottomTrackOneBeamNarrow) {
        istart = 4;
        init_win = 6;
        scale_win = 35;

        c1 = -3, c2 = 8, c3 = 5, c4 = -1, c5 = -1;
        s2 = 1.015f, s3 = 1.035f, s4 = 1.08f, s5 = 1.12f;
        t1 = 1.04;
    }


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

        Epoch::Echogram* chart = epoch->chart(channel1);

        uint8_t* data = (uint8_t*)chart->amplitude.constData();
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


        uint8_t* data_from = &data[istart];
        uint8_t* data_to = &data[(istart+init_win)];

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
            Epoch::Echogram* chart = epoch->chart(channel1);
            if(chart->bottomProcessing.source < Epoch::DistProcessing::DistanceSourceDirectHand) {
                float dist = bottom_track[iepoch - epoch_min_index];
                chart->bottomProcessing.setDistance(dist, Epoch::DistProcessing::DistanceSourceProcessing);
            }
        }

        if(epoch->chartAvail(channel2)) {
            Epoch::Echogram* chart = epoch->chart(channel2);
            if(chart->bottomProcessing.source < Epoch::DistProcessing::DistanceSourceDirectHand) {
                float dist = bottom_track[iepoch - epoch_min_index];
                chart->bottomProcessing.setDistance(dist, Epoch::DistProcessing::DistanceSourceProcessing);
            }
        }
    }

    setChannelOffset(channel1, param.offset.x, param.offset.y, param.offset.z);
    spatialProcessing();
    emit dataUpdate();
    lastBottomTrackEpoch_ = size();
    emit bottomTrackUpdated(epoch_min_index, epoch_max_index);
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
                Epoch::Echogram* data = epoch->chart(ich);

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

    emit dataUpdate();
}

void Dataset::usblProcessing() {
    const int to_size = size();
    int from_index = 0;

    _beaconTrack.clear();
    _beaconTrack1.clear();

    for(int i = from_index; i < to_size; i+=1) {
        Epoch* epoch = fromIndex(i);
        Position pos = epoch->getPositionGNSS();

        // if(pos.lla.isCoordinatesValid() && !pos.ned.isCoordinatesValid()) {
        //     if(!_llaRef.isInit) {
        //         _llaRef = LLARef(pos.lla);
        //     }
        //     pos.LLA2NED(&_llaRef);
        // }

        if(pos.ned.isCoordinatesValid() && epoch->isAttAvail() && epoch->isUsblSolutionAvailable()) {
            double n = pos.ned.n, e = pos.ned.e;

            double yaw = epoch->yaw();
            double azimuth = epoch->usblSolution().azimuth_deg-180;
            double dist = epoch->usblSolution().distance_m;
            double dir = ((yaw + azimuth) + 120);
            double rel_n = dist*cos(qDegreesToRadians(dir));
            double rel_e = dist*sin(qDegreesToRadians(dir));
            // if(i > 4000 && i < 4500) {
            //     _beaconTrack.append(QVector3D(n+rel_n, e + rel_e, 0));
            // }
            // if(dist > 250 && (abs(azimuth)  > 170)) {
            //     _beaconTrack1.append(QVector3D(n+rel_n, e + rel_e, 0));
            // }
            // if(dist > 50 && azimuth < 10  && azimuth > -10) {
            //     _beaconTrack.append(QVector3D(n+rel_n, e + rel_e, 0));
            // }
            // if(dist > 250 && (abs(azimuth)  > 170)) {
            //     _beaconTrack1.append(QVector3D(n+rel_n, e + rel_e, 0));
            // }

        }
    }
}

void Dataset::setRefPosition(int epoch_index) {
    Epoch*  ref_epoch = fromIndex(epoch_index);
    setRefPosition(ref_epoch);
}

void Dataset::setRefPosition(Epoch* epoch) {
    if(epoch == NULL) { return; }

    setRefPosition(epoch->getPositionGNSS());
}

void Dataset::setRefPosition(Position ref_pos) {
    if(ref_pos.lla.isCoordinatesValid()) {
        _llaRef = LLARef(ref_pos.lla);

        for(int iepoch = 0; iepoch < size(); iepoch++) {
            Epoch* epoch = fromIndex(iepoch);
            if(epoch == NULL) { continue; }
            epoch->setPositionRef(&_llaRef);
        }
    }

    emitPositionsUpdated();
}

void Dataset::setRefPositionByFirstValid() {
    Epoch* epoch = getFirstEpochByValidPosition();
    if(epoch == NULL) { return; }

    setRefPosition(epoch);
}

Epoch *Dataset::getFirstEpochByValidPosition() {
    for(int iepoch = 0; iepoch < size(); iepoch++) {
        Epoch* epoch = fromIndex(iepoch);
        if(epoch == NULL) { continue; }
        if(epoch->getPositionGNSS().lla.isCoordinatesValid()) {
            return epoch;
        }
    }

    return NULL;
}

void Dataset::clearBoatTrack() {
    lastBoatTrackEpoch_ = 0;
    _boatTrack.clear();
    emit dataUpdate();
}

void Dataset::updateBoatTrack(bool update_all) {
    const int to_size = size();
    int from_index = 0;

    if(update_all) {
        _boatTrack.clear();
    } else {
        from_index = lastBoatTrackEpoch_;
    }

    QMap<int, DatasetChannel> ch_list = channelsList();

    for(int i = from_index; i < to_size; i+=1) {
        Epoch* epoch = fromIndex(i);
        Position pos = epoch->getPositionGNSS();

        // if(pos.lla.isCoordinatesValid() && !pos.ned.isCoordinatesValid()) {
        //     if(!_llaRef.isInit) {
        //         _llaRef = LLARef(pos.lla);
        //     }
        //     pos.LLA2NED(&_llaRef);
        // }

        if(pos.ned.isCoordinatesValid()) {
            _boatTrack.append(QVector3D(pos.ned.n,pos.ned.e, 0));
        }
    }

    lastBoatTrackEpoch_ = to_size;

    emit boatTrackUpdated();
}

QStringList Dataset::channelsNameList() {
    QStringList ch_names;
    QList<DatasetChannel> ch_list = channelsList().values();
    ch_names.append(QString("None"));
    ch_names.append(QString("First"));
    for (const auto& channel : ch_list) {
        ch_names.append(QString("%1").arg(channel.channel));
    }
    return ch_names;

}
