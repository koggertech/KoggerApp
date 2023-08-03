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

    if(ref != NULL && ref->isInit) {
        _positionGNSS.LLA2NED(ref);
    }
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
    //    const int raw_size = chart.amplitude.size();
    //    const int16_t* src = chart.amplitude.data();

    //    int16_t* procData = NULL;

    //    if(raw_size > 0 && (!chart.bottomProcessing.isData() || is_update_dist)) {
    //        chart.bottomProcessing.amplitude.resize(raw_size+(( raw_size) >> 6)*3);
    //        chart.bottomProcessing.amplitude.fill(0);
    //        procData = chart.bottomProcessing.amplitude.data();

    //        int inc_offset = 0;
    //        int dec_offset = 0;
    //        int summ = 0;

    //        for(int i = dec_offset; i < 4; i ++) {
    //            summ += src[i];
    //            inc_offset++;
    //        }

    ////        for(int i = raw_size - (( raw_size) >> 6); i < raw_size; i ++) {
    ////            procData[i] = 0;
    ////        }

    //        for(dec_offset = 0; inc_offset < raw_size; dec_offset ++) {
    //            summ -= src[dec_offset];
    //            summ += src[inc_offset];
    //            inc_offset++;
    //            if(dec_offset % 128 == 0) {
    //                inc_offset++;
    //                summ += src[inc_offset];
    //            }
    //            procData[dec_offset] = summ/(inc_offset - dec_offset + 14);
    //        }

    //        for(dec_offset; dec_offset < raw_size - 8; dec_offset ++) {
    //            inc_offset--;
    //            summ -= src[dec_offset];
    //            procData[dec_offset] = summ/(raw_size - dec_offset);
    //        }

    //        for(dec_offset; dec_offset < raw_size + (( raw_size) >> 6)*2; dec_offset ++) {
    //            procData[dec_offset] = summ/(9);
    //        }

    //        for(int i = 0; i < 4; i ++) {
    //            procData[i] = -1000;
    //        }

    //        int att = procData[4];
    //        for(int i = 4; i < raw_size - (( raw_size) >> 6); i ++) {
    //            int wsize = ((i) >> 8) + 5;

    //            procData[i] = ((-procData[i] + procData[i+wsize]*5 + procData[i+wsize*2]*1 - procData[i+wsize*3] - procData[i+wsize*4])) - att;
    //            att -= att/32;
    //        }
    //    }

    //    if(chart.bottomProcessing.isData() && (!chart.bottomProcessing.isDist() || is_update_dist)) {
    //        procData = chart.bottomProcessing.amplitude.data();
    //        int index_max = 0;
    //        int16_t val_max = -32766;
    //        int max_index = chart.bottomProcessing.max/chart.resolution;
    //        int min_index = chart.bottomProcessing.min/chart.resolution;

    //        if(max_index > raw_size) {
    //            max_index = raw_size;
    //        }

    //        if(min_index > raw_size) {
    //            min_index = raw_size;
    //        }

    //        if(min_index < 0) {
    //            min_index = 0;
    //        }

    //        for(int i = min_index; i < max_index; i ++) {
    //            if(procData[i] > val_max) {
    //                val_max = procData[i];
    //                index_max = i;
    //            }
    //        }

    //        int wsize = index_max/200;
    //        chart.bottomProcessing.setDistance((index_max + chart.offset + wsize + 6)*chart.resolution, DistProcessing::DistanceSourceProcessing);
    //    }
}

void Epoch::doBottomTrackSideScan(DataChart &chart, bool is_update_dist) {
    //    int raw_size = _chartData.size() - 50;
    //    const int16_t* src = _chartData.data();

    //    int16_t* procData = NULL;

    //    if(raw_size > 0 && (!flags.processChartAvail || is_update_dist)) {
    //        m_processingDistData.resize(raw_size);
    //        m_processingDistData.fill(0);
    //        procData = m_processingDistData.data();

    //        int inc_offset = 0;
    //        int dec_offset = 0;
    //        int summ = 0;

    //        for(int i = dec_offset; i < 20; i ++) {
    //            summ += src[i];
    //            inc_offset++;
    //        }

    //        for(int i = raw_size - 20; i < raw_size; i ++) {
    //            procData[i] = 0;
    //        }

    //        for(dec_offset = 0; inc_offset < raw_size - 70; dec_offset ++) {
    //            summ -= src[dec_offset];
    //            summ += src[inc_offset];
    //            inc_offset++;
    ////            if(dec_offset % 1024 == 0) {
    ////                inc_offset++;
    ////                summ += src[inc_offset];
    ////            }
    //            procData[dec_offset] = summ/(inc_offset - dec_offset + 14);
    //        }

    //        for(dec_offset; dec_offset < raw_size - 8; dec_offset ++) {
    //            inc_offset--;
    //            summ -= src[dec_offset];
    //            procData[dec_offset] = summ/(raw_size - dec_offset);
    //        }

    //        for(dec_offset; dec_offset < raw_size + (( raw_size) >> 6)*2; dec_offset ++) {
    //            procData[dec_offset] = summ/(9);
    //        }

    //        for(int i = 0; i < 4; i ++) {
    //            procData[i] = -1000;
    //        }

    //        int att = procData[4];
    //        int wsize = 0;
    //        for(int i = 4; i < raw_size - 20 - wsize*6; i ++) {
    //            wsize = 10;

    //            procData[i] = ((-procData[i] - procData[i+wsize] - procData[i+wsize*2]*2 + procData[i+wsize*3]*2 + procData[i+wsize*4]*2 + procData[i+wsize*5]*2)) - att;
    //            att -= att/32;
    //        }

    //        flags.processChartAvail = true;
    //    }

    //    if(flags.processChartAvail && (!flags.processDistAvail || is_update_dist)) {
    //        procData = m_processingDistData.data();
    //        int index_max = 0;
    //        int16_t val_max = -32766;
    //        int max_index = _procMaxDist/_chartResol;
    //        int min_index = _procMinDist/_chartResol;

    //        if(max_index > raw_size) {
    //            max_index = raw_size;
    //        }

    //        if(min_index > raw_size) {
    //            min_index = raw_size;
    //        }

    //        if(min_index < 0) {
    //            min_index = 0;
    //        }

    //        for(int i = min_index; i < max_index; i ++) {
    //            if(procData[i] > val_max) {
    //                val_max = procData[i];
    //                index_max = i;
    //            }
    //        }

    //        int wsize = 10;
    //        m_processingDist = (index_max + _chartOffset + wsize*3 + 5)*_chartResol;
    //        flags.processDistAvail = true;
    //    }
}

Dataset::Dataset() {
    resetDataset();

    //    m_colorMap.resize(256);
    //    for(int i = 0; i < 256; i++) {
    //        m_colorMap[i] = QColor::fromRgb(0,0,0);
    //    }

    //    setThemeId(0);
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

    //    if(_bottomtrackType >= 0) {
    ////        doDistProcessing();
    //    }

    //    float full_range = _pool[pool_index].chartsFullRange();

    //    if(_autoRange != AutoRangeNone) {
    //        m_offset = offset;
    //        m_range = data.size()*resolution;
    //    }

    //    updateImage(true);
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
    //    updateImage(true);
}

void Dataset::addDopplerBeam(IDBinDVL::BeamSolution *beams, uint16_t cnt) {
    int pool_index = endIndex();

    makeNewEpoch();
    pool_index = endIndex();

    _pool[endIndex()].setDopplerBeam(beams, cnt);
    //    _pool[poolLastIndex()].setDist(beams[0+pool_index%cnt].distance*1000);
    //    updateImage(true);
}

void Dataset::addDVLSolution(IDBinDVL::DVLSolution dvlSolution) {
    int pool_index = endIndex();

    if(pool_index < 0 || (_pool[pool_index].isDopplerBeamAvail() == false)) {
        makeNewEpoch();
        pool_index = endIndex();
    }

    _pool[endIndex()].setDVLSolution(dvlSolution);
    //    _pool[poolLastIndex()].setDist(dvlSolution.distance.z*1000);
    //    updateImage(true);
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

    //   qInfo("Euler: yaw %f, pitch %f, roll %f", _lastYaw, _lastPitch, _lastRoll);
    //    updateImage();
}

void Dataset::addPosition(double lat, double lon, uint32_t unix_time, int32_t nanosec) {

    int pool_index = endIndex();
    if(pool_index < 0) {
        makeNewEpoch();
        pool_index = endIndex();
    }

    if(!_llaRef.isInit) {
        _llaRef.refLatRad = lat * M_DEG_TO_RAD;
        _llaRef.refLonRad= lon * M_DEG_TO_RAD;
        _llaRef.refLatSin = sin(_llaRef.refLatRad);
        _llaRef.refLatCos = cos(_llaRef.refLatRad);
        _llaRef.isInit = true;
    }

    _pool[pool_index].setPositionLLA(lat, lon, &_llaRef, unix_time, nanosec);
    _gnssTrackIndex.append(pool_index);
    _boatTrack.append(QVector3D(_pool[pool_index].relPosN(), _pool[pool_index].relPosE(), 0));
    //    _bottomTrack.append(QVector3D(_pool[pool_index].relPosN(), _pool[pool_index].relPosE(), 0));
    //    updateBottomTrack();
    //    if(_pool[pool_index].distProccesingAvail(0)) {
    //        updateBottomTrack();
    //    }
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

//void Dataset::setColorScheme(QVector<QColor> coloros, QVector<int> levels) {
//    if(coloros.length() != levels.length()) { return; }

//    int nbr_levels = coloros.length() - 1;
//    int i_level = 0;

//    for(int i = 0; i < nbr_levels; i++) {
//        while(levels[i + 1] >= i_level) {
//            float b_koef = (float)(i_level - levels[i]) / (float)(levels[i + 1] - levels[i]);
//            float a_koef = 1.0f - b_koef;

//            int red = qRound(coloros[i].red()*a_koef + coloros[i + 1].red()*b_koef);
//            int green = qRound(coloros[i].green()*a_koef + coloros[i + 1].green()*b_koef);
//            int blue = qRound(coloros[i].blue()*a_koef + coloros[i + 1].blue()*b_koef);
//            m_colorMap[i_level] = QColor::fromRgb(red, green, blue);
//            m_colorHashMap[i_level] = ((red / 8) << 10) | ((green / 8) << 5) | ((blue / 8));
//            i_level++;
//        }
//    }

//    m_colorDist = ((220 / 8) << 10) | ((50 / 8) << 5) | ((0 / 8));
//    updateImage();
//}

//void Dataset::setStartLevel(int level) {
//    m_startLevel = level;
//    updateImage();
//}

//void Dataset::setStopLevel(int level) {
//    m_stopLevel = level;
//    updateImage();
//}

//void Dataset::setTimelinePosition(double position) {
//    int m_lineVisibleCount = m_image.width();
//    if(m_lineVisibleCount > poolSize()) {
//        m_lineVisibleCount = poolSize();
//    }
//    m_offsetLine = (int)(position*(double)(poolSize() - m_lineVisibleCount/2));
//    updateImage(true);
//}

//void Dataset::scrollTimeline(int delta) {
//    int new_pos = m_offsetLine + delta/2;
//    if(new_pos < 0) {
//        new_pos = 0;
//    } else if(new_pos > poolSize() - m_image.width()) {
//        new_pos = poolSize() - m_image.width();
//    }

//    m_offsetLine = new_pos;
//    updateImage(true);
//}

//void Dataset::verZoom(int delta) {
//    if(delta == 0) return;

//    float zoom = delta < 0 ? -delta*0.001f : delta*0.001f;
//    int delta_range = ((int)((float)m_range*zoom)/1000)*1000;
//    int new_range = 0;

////    qInfo("delta %i", delta_range);

//    if(delta_range < 100) {
//        delta_range = 100;
//    } else if(delta_range > 10000) {
//        delta_range = 10000;
//    }

//    if(delta > 0) {
//        new_range = m_range + delta_range;
//    } else {
//        new_range = m_range - delta_range;
//    }

//    if(new_range < 1000) {
//        new_range = 1000;
//    } else if(new_range > 100000) {
//        new_range = 100000;
//    }

////    int new_offset = m_offset + (m_range - new_range)/2;
////    if(new_offset < 0) {
////        new_offset = 0;
////    } else if(new_offset > 100000) {
////        new_offset = 100000;
////    }
////    m_offset = new_offset;
//    m_range = new_range;
//    updateImage(true);
//}

//void Dataset::verScroll(int delta) {
//    int delta_offset = (float)m_range*(float)delta*0.0002;
//    if(delta_offset > 0 && delta_offset < 100) {
//        delta_offset = 100;
//    } else if(delta_offset < 0 && delta_offset > -100) {
//        delta_offset = -100;
//    }

//    int new_offset = m_offset + (delta_offset/100)*100;
//    if(new_offset < 0) {
//        new_offset = 0;
//    } else if(new_offset > 100000) {
//        new_offset = 100000;
//    }
//    m_offset = new_offset;

//    updateImage(true);
//}

//void Dataset::setMouseMode(int mode) {
//    _mouse_mode = mode;
//}

//void Dataset::setMouse(int x, int y) {
//    qInfo("LeftClick %i, %i, %i", x, y, _mouse_mode);
//    int waterfall_width = m_valueCash.size();
//    int height = m_prevValueCash.chartData.size();


//    if(x < -1) { x = -1; }
//    if(x >= waterfall_width) { x = waterfall_width - 1; }

//    if(y < 0) { y = 0; }
//    if(y >= height) { x = height - 1; }

//    if(x == -1) {
//        _mouse_x = -1;
//        return;
//    }

//    int x_start = 0, y_start = 0;
//    int x_length = 0;
//    float y_scale = 0.0f;
//    if(_mouse_x != -1) {
//        if(_mouse_x < x) {
//            x_length = x - _mouse_x;
//            x_start = _mouse_x;
//            y_start = _mouse_y;
//            y_scale = (float)(y - _mouse_y)/(float)x_length;
//        } else if(_mouse_x > x) {
//            x_length = _mouse_x - x;
//            x_start = x;
//            y_start = y;
//            y_scale = -(float)(y - _mouse_y)/(float)x_length;
//        } else {
//            x_length = 1;
//            x_start = x;
//            y_start = y;
//            y_scale = 0;
//        }
//    } else {
//        x_length = 1;
//        x_start = x;
//        y_start = y;
//        y_scale = 0;
//    }

//    _mouse_x = x;
//    _mouse_y = y;


//    if(_mouse_mode > 1) {
//        for(int x_ind = 0; x_ind < x_length; x_ind++) {
//            int val_col = (m_valueCashStart + x_start + x_ind);
//            if(val_col >= waterfall_width) {
//                val_col -= waterfall_width;
//            }

////            m_valueCash[val_col].processingDistData = 0;

//            int pool_index = m_valueCash[val_col].poolIndex;
//            if(pool_index > 0) {
//                int dist = ((float)y_start + (float)x_ind*y_scale)*(float)(m_range)/(float)height + m_offset;
//                if(_mouse_mode == 2) {
//                    _pool[pool_index].setMinDistProc(0, dist);
//                } else if(_mouse_mode == 3) {
//                    _pool[pool_index].setDistProcessing(0, dist);
//                } else if(_mouse_mode == 4) {
//                    _pool[pool_index].setMaxDistProc(0, dist);
//                }
//                m_valueCash[val_col].processingDistData = _pool[pool_index].distProccesing(0) - m_offset;
////                core.consoleInfo(QString("lat, lon: %1, %2").arg(_pool[pool_index].lat()).arg(_pool[pool_index].lon()));
//            }
//        }

//        updateBottomTrack(true);
//    }


//    updateImage(false);
//}

//void Dataset::setChartVis(bool visible) {
//    m_chartVis = visible;
//    resetValue();
//    updateImage(true);
//}

//void Dataset::setOscVis(bool visible) {
//    m_oscVis = visible;
//    resetValue();
//    updateImage(true);
//}

//void Dataset::setDistVis(bool visible) {
//    m_distSonarVis = visible;
//    resetValue();
//    updateImage(true);
//}

//void Dataset::setDistProcVis(bool visible) {
//    m_distProcessingVis = visible;
//    resetValue();
//    updateImage(true);
//}

//void Dataset::setBottomTrackTheme(int bottomTrackTheme) {
//    _bottomTrackTheme = bottomTrackTheme;
//    updateImage(true);
//}

//void Dataset::setEncoderVis(bool visible) {
//    _is_encoderVis = visible;
//    updateImage(true);
//}

//void Dataset::setVelocityVis(bool visible) {
//    _is_velocityVis = visible;
//    updateImage(true);
//}

//void Dataset::setVelocityRange(float range) {
//    _velocityRange = range*2;
//    updateImage(false);
//}

//void Dataset::setDopplerBeamVis(bool visible, int beamFilter, bool is_mode_visible, bool is_amp_visible) {
//    _isDopplerBeamVis = visible;
//    _dopplerBeamFilter = beamFilter;
//    _isDopplerBeamAmpitudeVisible = is_amp_visible;
//    _isDopplerBeamModeVisible = is_mode_visible;
//    updateImage(true);
//}

//void Dataset::setDopplerInstrumentVis(bool visible) {
//    _isDopplerInstrimentVis = visible;
//    updateImage(true);
//}

//void Dataset::setGridNumber(int number) {
//    m_verticalGridNum = number;
//    updateImage(true);
//}


//void Dataset::setImageType(int image_type) {
//    _imageType = image_type;
//    resetValue();
//    updateImage(true);
//}



//void Dataset::setAHRSVis(bool visible) {
//    _is_attitudeVis = visible;
//    updateImage(true);
//}

//void Dataset::updateImage(bool update_value) {
//    bool send_update = false;

//    if(update_value) {
//        renderValue();
//        send_update = true;
//    }

//    if(flags.renderImage == false) {
//        flags.renderImage = true;
//        send_update = true;
//    }

//    if(send_update) {
//        emit updatedImage();
//    }
//}

//void Dataset::renderValue() {
//    flags.renderValue = true;
//}

//void Dataset::resetValue() {
//    flags.resetValue = true;
//}

void Dataset::resetDataset() {
    _pool.clear();
    //    resetValue();
    //    m_valueCash.clear();
    _llaRef.isInit = false;
    _gnssTrackIndex.clear();
    _bottomTrack.clear();
    _boatTrack.clear();
    _channelsSetup.clear();
    resetDistProcessing();
}

void Dataset::doDistProcessing() {
    //    doDistProcessing(_bottomtrackType, _bottomTrackWindowSize, _bottomTrackVerticalGap, _bottomTrackMinRange, _bottomTrackMaxRange);
}

void Dataset::doDistProcessing(int source_type, int window_size, float vertical_gap, float range_min, float range_max) {
    //    int pool_size = poolSize();

    //    bool is_source_update = _bottomtrackType != source_type;
    //    bool is_param_update = is_source_update ||
    //            _bottomTrackWindowSize != window_size ||
    //            _bottomTrackVerticalGap != vertical_gap ||
    //            _bottomTrackMinRange != range_min ||
    //            _bottomTrackMaxRange != range_max;
    //    _bottomtrackType = source_type;
    //    _bottomTrackWindowSize = window_size;
    //    _bottomTrackVerticalGap = vertical_gap;
    //    _bottomTrackMinRange = range_min;
    //    _bottomTrackMaxRange = range_max;

    //    if(source_type < 0) {
    //        return;
    //    }

    //    const uint16_t avrg_size = window_size;
    //    uint16_t start_pos = _bottomTrackLastIndex;

    //    if(is_param_update) {
    //        start_pos = 0;
    //        _bottomTrackLastIndex = 0;
    //        _bottomTrackWindow.clear();
    //    }

    //    if(is_source_update) {
    //        _bottomTrackLastProcessing = 0;
    //    }

    //    uint16_t max_size = _bottomTrackWindow.size();
    //    for(int i = _bottomTrackLastProcessing+1; i < pool_size; i++) {
    //        PoolDataset* dataset = fromPool(i);
    //        dataset->doBottomTrack(source_type, is_source_update);
    //        uint16_t cur_size = dataset->m_processingDistData.size();
    //        if(max_size < cur_size) {  max_size = cur_size; }
    //        _bottomTrackLastProcessing = i;
    //    }

    //    if(_bottomTrackWindow.size() < max_size) {
    //        _bottomTrackWindow.resize(max_size);
    //    }

    //    if(pool_size > avrg_size && max_size > 0) {
    //        int32_t* avrg_data = _bottomTrackWindow.data();

    //        if(is_param_update || _bottomTrackLastIndex == 0) {
    //            for(int i = 0; i < avrg_size; i++) {
    //                PoolDataset* dataset = fromPool(i);

    //                uint16_t cur_size = dataset->m_processingDistData.size();
    //                const int16_t* data = dataset->m_processingDistData.constData();
    //                for(uint16_t k = 0; k < cur_size; k++) {
    //                    avrg_data[k] += data[k];
    //                }
    //            }
    //        }

    //        for(int i = start_pos; i < pool_size - avrg_size; i++) {
    //            PoolDataset* dataset = fromPool(i);

    //            uint16_t cur_size = dataset->m_processingDistData.size();
    //            const int16_t* data = dataset->m_processingDistData.data();

    //            for(uint16_t k = 0; k < cur_size; k++) {
    //                avrg_data[k] -= data[k];
    //            }

    //            PoolDataset* dataset2 = fromPool(i+avrg_size);
    //            uint16_t cur_size2 = dataset2->m_processingDistData.size();
    //            const int16_t* data2 = dataset2->m_processingDistData.data();

    //            for(uint16_t k = 0; k < cur_size2; k++) {
    //                avrg_data[k] += data2[k];
    //            }

    //            uint16_t start_pos = range_min*100;
    //            uint16_t stop_pos = range_max*100;

    //            if(start_pos < 0) {
    //                start_pos = 0;
    //            }

    //            if(stop_pos > max_size) {
    //                stop_pos = max_size;
    //            }

    //            int32_t max_val = -2000000000;
    //            uint16_t max_ind = start_pos;
    //            for(uint16_t k = start_pos; k < stop_pos; k++) {
    //                int val = avrg_data[k];
    //                if(max_val <= val) {
    //                    max_val = val;
    //                    max_ind = k;
    //                }
    //            }

    //            float dist = float(max_ind)*0.01f;
    //            float min_dist = dist*(1.0f - 0.5f*vertical_gap);
    //            float max_dist = dist*(1.0f + 0.5f*vertical_gap);

    //            if(vertical_gap == 0) {
    //                _pool[i+avrg_size/2].setDistProcessing(dist*1000);
    //            } else {
    //                _pool[i+avrg_size/2].setMinMaxDistProc(min_dist*1000, max_dist*1000, false);
    //            }
    //        }

    //        _bottomTrackLastIndex = pool_size - avrg_size;
    //    }

    //    updateImage(true);
    //    updateBottomTrack(true);
}

void Dataset::resetDistProcessing() {
    //    _bottomTrackWindow.clear();
    int pool_size = size();
    for(int i = 0; i < pool_size; i++) {
        Epoch* dataset = fromIndex(i);
        dataset->resetDistProccesing();
    }
    //    _bottomTrackLastIndex = 0;
    //    _bottomTrackLastProcessing = 0;
}

void Dataset::bottomTrackSidescan(int channel1, int channel2, BottomTrackParam param) {
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

    const int istart = 4;
    const int init_win = 6;
    const int scale_win = 20;

    const int16_t c1 = -6, c2 = 6, c3 = 4, c4 = 2, c5 = 0;
    const float s2 = 1.04f, s3 = 1.06f, s4 = 1.10f, s5 = 1.15f;
    const float t1 = 1.07;

    const int gain_slope_inv = 1000/(gain_slope);
    const int threshold_int = 100*gain_slope_inv*1000*threshold;

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
            cash_data[idata] = 100*data_acc / (avrg_range);
        }

        //            int max_val = threshold_int;
        //            int max_ind = -1;
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

            //                if(calc > max_val) {
            //                    max_val = calc;
            //                    max_ind = di3;
            //                }

            cash_data[idata] = calc;
        }

        //            if(max_ind > 0) {
        //                if(chart->bottomProcessing.source < Epoch::DistProcessing::DistanceSourceHand) {
        //                    chart->bottomProcessing.setDistance(max_ind*chart->resolution, Epoch::DistProcessing::DistanceSourceProcessing);
        //                }
        //            }



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
        if(epoch == NULL || !epoch->chartAvail(channel1)) { continue; }

        Epoch::DataChart* chart = epoch->chart(channel1);
        if(chart->bottomProcessing.source < Epoch::DistProcessing::DistanceSourceDirectHand) {
            chart->bottomProcessing.setDistance(bottom_track[iepoch - epoch_min_index], Epoch::DistProcessing::DistanceSourceProcessing);
        }
    }
}

//void Dataset::setThemeId(int theme_id) {
//    QVector<QColor> coloros;
//    QVector<int> levels;

//    if(theme_id == ClassicTheme) {
//        coloros = { QColor::fromRgb(0, 0, 0), QColor::fromRgb(20, 5, 80), QColor::fromRgb(50, 180, 230), QColor::fromRgb(220, 255, 255)};
//        levels = {0, 30, 130, 255};
//    } else if(theme_id == SepiaTheme) {
//        coloros = { QColor::fromRgb(0, 0, 0), QColor::fromRgb(50, 50, 10), QColor::fromRgb(230, 200, 100), QColor::fromRgb(255, 255, 220)};
//        levels = {0, 30, 130, 255};
//    }else if(theme_id == WRGBDTheme) {
//        coloros = {
//            QColor::fromRgb(0, 0, 0),
//            QColor::fromRgb(40, 0, 80),
//            QColor::fromRgb(0, 30, 150),
//            QColor::fromRgb(20, 230, 30),
//            QColor::fromRgb(255, 50, 20),
//            QColor::fromRgb(255, 255, 255),
//        };

//        levels = {0, 30, 80, 120, 150, 255};
//    } else if(theme_id == WBTheme) {
//        coloros = { QColor::fromRgb(0, 0, 0), QColor::fromRgb(190, 200, 200), QColor::fromRgb(230, 255, 255)};
//        levels = {0, 150, 255};
//    } else if(theme_id == BWTheme) {
//        coloros = {QColor::fromRgb(230, 255, 255), QColor::fromRgb(70, 70, 70), QColor::fromRgb(0, 0, 0)};
//        levels = {0, 150, 255};
//    }

//    setColorScheme(coloros, levels);
//    updateImage();
//}

void Dataset::updateBottomTrack(bool update_all) {
    const int to_size = _gnssTrackIndex.size();
    int from_index = _bottomTrack.size();

    if(update_all) { from_index = 0; }

    _bottomTrack.resize(to_size);

    for(int i = from_index; i < to_size; i+=1) {
        Epoch* dataset = fromIndex(_gnssTrackIndex[i]);
        _bottomTrack[i] = _boatTrack[i];
        _bottomTrack[i][2] = -dataset->relPosD();
    }

    if (mp3DSceneModel){
        mp3DSceneModel->setBottomTrack(_bottomTrack);
    }

    //if (update_all){

    //    updateRender3D();
    //}

}

QStringList Dataset::channelsNameList() {
    QStringList ch_names;
    QList<DatasetChannel> ch_list = channelsList();
    ch_names.append(QString("None"));
    for (const auto& channel : ch_list) {
        ch_names.append(QString("%1").arg(channel.channel));
    }
    return ch_names;

}

//void Dataset::updateValueMap(int width, int height) {
//    if(m_valueCash.size() != width) {
//        m_valueCash.resize(width);
//    }

//    m_prevValueCash.chartData.resize(height);
//    for(int column = 0; column < width; column++) {
//        m_valueCash[column].poolIndexUpdate = false;
//        if(m_valueCash[column].chartData.size() != height) {
//            m_valueCash[column].chartData.resize(height);
//            m_valueCash[column].poolIndexUpdate = true;
//        }
//    }

//    int pool_last_index = poolLastIndex();

//    int size_column = m_prevValueCash.chartData.size();
//    int16_t* data_column = m_prevValueCash.chartData.data();

//    int pool_index = poolIndex(pool_last_index);
//    if(pool_index >= 0) {
//        if(_pool[pool_index].chartAvail())  {
//            _pool[pool_last_index].chartTo(0, m_offset,m_offset + m_range, data_column, size_column, _imageType);
//        } else {
//            memset(data_column, 0, size_column*2);
//        }

//        if(_pool[pool_index].distAvail()) {
//            m_prevValueCash.distData = _pool[pool_index].distData() - m_offset;
//        }

////        if(_pool[pool_index].temperatureAvail()) {
////            m_prevValueCash.temperature = _pool[pool_index].temperature();
////        }
//    }

//    int pool_offset_index = pool_last_index - m_offsetLine;
//    m_valueCashStart = qAbs(pool_offset_index % width);

//    bool force_reset = flags.resetValue = true;
//    flags.resetValue = false;

//    for(int column = 0; column < width; column++) {
//        int val_col = (m_valueCashStart + column);
//        if(val_col >= width) {
//            val_col -= width;
//        }

//        int pool_ind = poolIndex(pool_offset_index + (column - width));
//        if(m_valueCash[val_col].poolIndex != pool_ind || force_reset) {
//            m_valueCash[val_col].poolIndex = pool_ind;
//            m_valueCash[val_col].poolIndexUpdate = true;
//        }
//    }

//    for(int column = 0; column < width; column++) {
//        if(!m_valueCash[column].poolIndexUpdate) {
//            continue;
//        }
//        m_valueCash[column].poolIndexUpdate = false;

//        const uint16_t datacashsize = m_valueCash[column].chartData.size();
//        int16_t* const datacash = m_valueCash[column].chartData.data();

//        int pool_index = m_valueCash[column].poolIndex;

//        if(pool_index >= 0) {
//            if(m_chartVis) {
//                QList<int16_t> charts_channels = _pool[pool_index].chartChannels();
////                memset(datacash, (int16_t(-65534) << 16) | int16_t(-65534), size_column*2);

//                for(uint16_t i = 0; i < datacashsize; i++) {
//                    datacash[i] = -32767;
//                }

//                for(uint16_t ch = 0; ch < charts_channels.size(); ch++) {
//                    int16_t channel = charts_channels[ch];

//                    if(channel == 0) {
//                        _pool[pool_index].chartTo(channel, m_offset, m_offset + m_range, datacash, datacashsize, _imageType);
//                    } else if(channel == 1) {
//                        _pool[pool_index].chartTo(channel, m_offset, m_offset + m_range, datacash, datacashsize/2, _imageType, true);
//                    } else if(channel == 2) {
//                        _pool[pool_index].chartTo(channel, m_offset, m_offset + m_range, &datacash[datacashsize/2], datacashsize/2, _imageType);
//                    } else if(channel == 3) {
//                        _pool[pool_index].chartTo(channel, m_offset, m_offset + m_range, &datacash[datacashsize/2], datacashsize/2, _imageType, true);
//                    } else if(channel == 4) {
//                        _pool[pool_index].chartTo(channel, m_offset, m_offset + m_range, &datacash[datacashsize/2], datacashsize/2, _imageType);
//                    }

//                }

//            }

//            if(m_distSonarVis) {
//                if(_pool[pool_index].distAvail()) {
//                    m_valueCash[column].distData = _pool[pool_index].distData() - m_offset;
//                } else {
//                    m_valueCash[column].distData = -1;
//                }
//            }

//            if(m_distProcessingVis) {
////                if(_pool[pool_index].distProccesingAvail(0)) {
////                    m_valueCash[column].processingDistData = _pool[pool_index].distProccesing(0) - m_offset;
////                } else {
////                    m_valueCash[column].processingDistData = INT_MIN;
////                }
//            }

//        } else {
//            memset(datacash, 0, datacashsize*2);
//            m_valueCash[column].distData = -1;
//            m_valueCash[column].processingDistData = INT_MIN;
//        }
//    }
//}

//void Dataset::updateImage(int width, int height) {
//    flags.renderImage = false;
//    int waterfall_width = width - m_prevLineWidth;

//    if(flags.renderValue) {
//        flags.renderValue = false;
//        updateValueMap(waterfall_width, height);
//    }

//    if(m_chartVis) {
//        memset(m_dataImage, 0, width*height*2);

//        int level_range = m_stopLevel - m_startLevel;
//        int index_offset = (int)((float)m_startLevel*2.5f);
//        float index_map_scale = 0;
//        if(level_range > 0) {
//            index_map_scale = (float)(m_colorMap.length() - 1)/((float)(m_stopLevel - m_startLevel)*2.5f);
//        } else {
//            index_map_scale = 10000;
//        }

//        int16_t* raw_col = m_prevValueCash.chartData.data();
//        for (int row = 0; row < height; row++) {
//            int32_t index_map = (float)(raw_col[row] - index_offset)*index_map_scale;
//            if(index_map < 0) { index_map = 0;
//            } else if(index_map > 255) { index_map = 255; }

//            for(int col = 1;  col < m_prevLineWidth; col++) {
//                m_dataImage[width - col + row*width] = m_colorHashMap[index_map];
//            }
//        }

//        for(int col = 0;  col < waterfall_width; col++) {
//            int val_col = (m_valueCashStart + col);
//            if(val_col >= waterfall_width) {
//                val_col -= waterfall_width;
//            }

//            int16_t* raw_col = m_valueCash[val_col].chartData.data();
//            int render_height = m_valueCash[val_col].chartData.size();
//            if(render_height > height) {
//                render_height = height;
//            }

//            for (int row = 0; row < render_height; row++) {
//                int32_t index_map = (float)(raw_col[row] - index_offset)*index_map_scale;
//                if(index_map < 0) { index_map = 0;
//                } else if(index_map > 255) { index_map = 255; }
//                m_dataImage[col + row*width] = m_colorHashMap[index_map];
//            }
//        }
//    } else {
//        memset(m_dataImage, 0, width*height*2);
//    }

////    if(true) {
////        QImage tmp_img = QImage((uint8_t*)m_dataImage, width, height, width*2, QImage::Format_RGB555);
////        QPainter p(&tmp_img);
////        QPen pen;
////        pen.setWidth(2);
////        pen.setColor(QColor::fromRgb(255, 150, 0));
////        p.setPen(pen);

////        float scale_y = (float)height*0.5f*0.5f;

////        float vx_prev = NAN;
////        for(int col = 1;  col < waterfall_width-1; col++) {
////            int val_col = (m_valueCashStart + col);
////            if(val_col >= waterfall_width) {
////                val_col -= waterfall_width;
////            }


////            if(isfinite(m_valueCash[val_col].dopplerX)) {
////                float vx = (float)height*0.5f - m_valueCash[val_col].dopplerX*scale_y;

////                if(isfinite(vx_prev)) {
////                    p.drawLine(col,vx_prev, col+1, vx);
////                }
////                vx_prev = vx;
////            } else {
////                vx_prev = NAN;
////            }
////        }
////    }

//    if(m_oscVis && _mouse_x >= 0 ) {
//        int val_col = (m_valueCashStart + _mouse_x);
//        if(val_col >= waterfall_width) {
//            val_col -= waterfall_width;
//        }

//        int pool_index = m_valueCash[val_col].poolIndex;
//        if(pool_index > 0 && _pool[pool_index].chartAvail()) {


//            int16_t* raw_col = _pool[pool_index].chartData().data();
//            int32_t data_size = _pool[pool_index].chartData().size();


//            float scale_scope_w = (float)height / (float)(waterfall_width - 100);
//            QImage tmp_img = QImage((uint8_t*)m_dataImage, width, height, width*2, QImage::Format_RGB555);
//            QPainter p(&tmp_img);
//            QPen pen;
//            pen.setWidth(2);
//            pen.setColor(QColor::fromRgb(50, 150, 0));

//            QPen pen2;
//            pen2.setWidth(2);
//            pen2.setColor(QColor::fromRgb(50, 255, 0));



//            float scale_y = (float)height*(1.0f/250.0f);
//            float scale_x = (float)(waterfall_width - 104)/(m_range/10);

//            int last_offset_y = (float)raw_col[0]*scale_y;
//            int last_col = 100;

////            for (int row = 1 + m_offset/10; row < (m_offset + m_range)/10; row++) {
////                int offset_y = height*0.5 - (float)(raw_col[row+1] - raw_col[row])*scale_y;
////                int offset_x = (float)(row-m_offset/10)*scale_x + 100;


////                if(last_offset_y != offset_y) {
////                    p.setPen(pen);
////                    p.drawLine(last_col,last_offset_y, offset_x, offset_y);
////                    p.setPen(pen2);
////                    p.drawRect(last_col-1, last_offset_y-1, 3, 3);

////                    last_col = offset_x;
////                }

////                last_offset_y = offset_y;
////            }

//            last_offset_y = (float)raw_col[0]*scale_y;
//            last_col = 100;

//            pen.setColor(QColor::fromRgb(255, 40, 0));
//            p.setPen(pen);

//            for (int row = 1 + m_offset/10; row < (m_offset + m_range)/10; row++) {
//                int offset_y = height - (float)(raw_col[row])*scale_y;
//                int offset_x = (float)(row-m_offset/10)*scale_x + 100;


//                if(last_offset_y != offset_y) {
//                    p.drawLine(last_col,last_offset_y, offset_x, offset_y);
//                    last_col = offset_x;
//                }

//                last_offset_y = offset_y;
//            }

//        }
//    }

//    if(_isDopplerInstrimentVis) {
//        QImage tmp_img = QImage((uint8_t*)m_dataImage, width, height, width*2, QImage::Format_RGB555);
//        QPainter p1(&tmp_img);

//        QColor vel_color[4] = {
//            QColor(255, 0, 0),
//            QColor(0, 255, 0),
//            QColor(0, 0, 255),
//            QColor(255, 255, 255)
//        };

//        QPen velo_pen;
//        velo_pen.setWidth(1);
//        velo_pen.setColor(QColor::fromRgb(100, 255, 0));

//        const float scaleY_vel = (float)height/_velocityRange;
//        float last_vel[4] = {};

//        int pool_index = -1;
//        for(int col = 1;  col < waterfall_width; col++) {
//            int val_col = (m_valueCashStart + col);
//            if(val_col >= waterfall_width) {
//                val_col -= waterfall_width;
//            }

//            pool_index = m_valueCash[val_col].poolIndex;
//            if(pool_index >= 0 && _pool[pool_index].isDVLSolutionAvail()) {

//                uint32_t hscale = 1;
//                IDBinDVL::DVLSolution dvl = _pool[pool_index].dvlSolution();
//                float vel_x = (float)height/2 - scaleY_vel*dvl.velocity.x;

//                if(isfinite(vel_x) && isfinite(last_vel[0])) {
//                    velo_pen.setColor(vel_color[0]);
//                    p1.setPen(velo_pen);
//                    p1.drawLine((col - 1)*hscale-waterfall_width*(hscale-1), last_vel[0], (col)*hscale-waterfall_width*(hscale-1), vel_x);
//                }
//                last_vel[0] = vel_x;

//                float vel_y = (float)height/2 - scaleY_vel*dvl.velocity.y;
//                if(isfinite(vel_y) && isfinite(last_vel[1])) {
//                    velo_pen.setColor(vel_color[1]);
//                    p1.setPen(velo_pen);
//                    p1.drawLine((col - 1)*hscale-waterfall_width*(hscale-1), last_vel[1], (col)*hscale-waterfall_width*(hscale-1), vel_y);
//                }
//                last_vel[1] = vel_y;

//                float vel_z = (float)height/2 - scaleY_vel*dvl.velocity.z;
//                if(isfinite(vel_z) && isfinite(last_vel[2])) {
//                    velo_pen.setColor(vel_color[2]);
//                    p1.setPen(velo_pen);
//                    p1.drawLine((col - 1)*hscale-waterfall_width*(hscale-1), last_vel[2], (col)*hscale-waterfall_width*(hscale-1), vel_z);
//                }
//                last_vel[2] = vel_z;

//            }
//        }

//    }

//    if(_isDopplerBeamVis) {
//        QImage tmp_img = QImage((uint8_t*)m_dataImage, width, height, width*2, QImage::Format_RGB555);
//        QPainter p1(&tmp_img);

//        QColor vel_color[4] = {
//            QColor(255, 0, 150),
//            QColor(0, 155, 255),
//            QColor(255, 175, 0),
//            QColor(75, 205, 55)
//        };

//        QColor amp_color[4] = {
//            QColor(255, 0, 150),
//            QColor(0, 155, 255),
//            QColor(255, 175, 0),
//            QColor(75, 205, 55)
//        };

//        QColor dist_color[4] = {
//            QColor(255, 0, 150, 200),
//            QColor(0, 155, 255, 200),
//            QColor(255, 175, 0, 200),
//            QColor(75, 205, 55, 200)
//        };

//        float last_vel[4] = {};
//        float last_amp[4] = {};

//        QPen velo_pen;
//        velo_pen.setWidth(2);
//        velo_pen.setColor(QColor::fromRgb(100, 255, 0));

//        QPen amp_pen;
//        amp_pen.setWidth(2);
//        amp_pen.setColor(QColor::fromRgb(255, 255, 255));

//        QPen mode_pen;
//        mode_pen.setWidth(2);
//        mode_pen.setColor(QColor::fromRgb(50, 255, 50));

//        QPen pen4;
//        pen4.setWidth(1);
//        pen4.setColor(QColor::fromRgb(0, 100, 255));

//        const float scaleY_amp = (float)height/(float)1000;
//        const float scaleY_vel = (float)height/_velocityRange;
//        const float scaleY_mode = (float)height/60;

//        int pool_index = -1;
//        float last_unc = 0, last_mode = 0;
//        for(int col = 1;  col < waterfall_width; col++) {
//            int val_col = (m_valueCashStart + col);
//            if(val_col >= waterfall_width) {
//                val_col -= waterfall_width;
//            }

//            pool_index = m_valueCash[val_col].poolIndex;
//            if(pool_index >= 0 && _pool[pool_index].isDopplerBeamAvail()) {
//                const uint16_t beam_cnt = _pool[pool_index].dopplerBeamCount();
////                const uint16_t beam_mask = (1 << 0) | (0 << 1);
//                const uint16_t beam_mask = _dopplerBeamFilter;
//                const uint16_t mode_mask = (1 << 0) | (1 << 1) | (1 << 2) | (1 << 3);
////                const uint16_t mode_mask = _dopplerBeamFilter<<1;

//                uint32_t hscale = 1;

//                for(uint16_t ibeam = 0; ibeam < beam_cnt; ibeam++) {
//                    if(((1 << ibeam) & beam_mask) == 0) { continue; }
//                    IDBinDVL::BeamSolution beam = _pool[pool_index].dopplerBeam(ibeam);
//                    if(((1 << beam.mode) & mode_mask) == 0) { continue; }

//                    float vel = (float)height/2 - scaleY_vel*beam.velocity;
//                    velo_pen.setColor(vel_color[ibeam]);
//                    p1.setPen(velo_pen);
//                    p1.drawLine((col - 1)*hscale-waterfall_width*(hscale-1), last_vel[ibeam], (col)*hscale-waterfall_width*(hscale-1), vel);
////                    p1.drawPoint((col)*hscale-waterfall_width*(hscale-1), vel);
//                    last_vel[ibeam] = vel;
//                }

//                if(_isDopplerBeamAmpitudeVisible) {
//                    for(uint16_t ibeam = 0; ibeam < beam_cnt; ibeam++) {
//                        if(((1 << ibeam) & beam_mask) == 0) { continue; }
//                        IDBinDVL::BeamSolution beam = _pool[pool_index].dopplerBeam(ibeam);
//                        if(((1 << beam.mode) & mode_mask) == 0) { continue; }

////                        _pool[pool_index].setDist(beam.distance*1000);

//                        float amp = scaleY_amp*(beam.amplitude);
//                        if(beam.mode > 0) {
//                            amp_pen.setColor(amp_color[ibeam]);
//                            p1.setPen(amp_pen);
//    //                        p1.drawPoint((col)*hscale-waterfall_width*(hscale-1), amp);
//                            p1.drawLine((col - 1)*hscale-waterfall_width*(hscale-1), last_amp[ibeam], (col)*hscale-waterfall_width*(hscale-1), amp);
//                        }
//                        last_amp[ibeam] = amp;
//                    }
//                }


//                if(_isDopplerBeamModeVisible) {
//                    for(uint16_t ibeam = 0; ibeam < beam_cnt; ibeam++) {
//                        if(((1 << ibeam) & beam_mask) == 0) { continue; }
//                        IDBinDVL::BeamSolution beam = _pool[pool_index].dopplerBeam(ibeam);
//                        if(((1 << beam.mode) & mode_mask) == 0) { continue; }

//                        float range_mode = (float)height - scaleY_mode*beam.mode;
//                        mode_pen.setColor(vel_color[ibeam]);
//                        p1.setPen(mode_pen);
//                        p1.drawPoint((col)*hscale-waterfall_width*(hscale-1), range_mode-3-ibeam*2);
//                    }
//                }

//                if(_isDopplerBeamDistVisible) {
//                    for(uint16_t ibeam = 0; ibeam < beam_cnt; ibeam++) {
//                        if(((1 << ibeam) & beam_mask) == 0) { continue; }
//                        IDBinDVL::BeamSolution beam = _pool[pool_index].dopplerBeam(ibeam);
//                        if(((1 << beam.mode) & mode_mask) == 0) { continue; }

//                        float range_mode = (float)height - scaleY_mode*beam.mode;

//                        int index_dist = (int)((float)beam.distance/(float)m_range*1000.0f*(float)height);
//                        if(index_dist < 0) {
//                            index_dist = 0;
//                        } else if(index_dist > height - 2) {
//                            index_dist = height - 2;
//                        }

//                        pen4.setColor(dist_color[ibeam]);
//                        p1.setPen(pen4);
//                        p1.drawPoint((col)*hscale-waterfall_width*(hscale-1), index_dist);
//                    }
//                }


////                IDBinDVL::BeamSolution beam1 = m_pool[pool_index].dopplerBeam(0);
////                float range1_mode = (float)height - scaleY_mode*beam1.mode;
////                p1.setPen(pen4);
////                p1.drawPoint(col, range1_mode-5);
////                last_mode = range1_mode;
////                float amp = (float)height - scaleY_amp*(float)(beam.amplitude - 70);

////                float unc = (float)height/2 - scaleY_vel*beam.uncertainty;

////                }
//            }
//        }
//    }


//    if( false ) {
//        int pool_index = -1;

//        if(_mouse_x >= 0) {
//            int val_col = (m_valueCashStart + _mouse_x);
//            if(val_col >= waterfall_width) {
//                val_col -= waterfall_width;
//            }
//            pool_index = m_valueCash[val_col].poolIndex;
//        } else {
//            pool_index = poolLastIndex();
//        }

//        if(pool_index >= 0 && _pool[pool_index].isIqAvail()) {
//            const int data_size = _pool[pool_index].iqData().size()/4;
////            const int data_size = 100;

//            float signalA[data_size+1000];
//            float signalPh[data_size+1000];

//            float comprA[data_size+1000];
//            float corrA[data_size+1000];
//            float corr2A[data_size+1000];
//            float corrI[data_size+1000];
//            float corrQ[data_size+1000];
//            float corr2I[data_size+1000];
//            float corr2Q[data_size+1000];
//            float speed[data_size+1000];
//            float speed2[data_size+1000];

//            const int16_t* iq_data = (const int16_t*)(_pool[pool_index].iqData().constData());

//            const int32_t dec_f = 2;
//            const int32_t w_size = 96/dec_f;
//            const int32_t w_size2 = 2000/dec_f;

////            int16_t data_ref[w_size*2];
////            for (int row = 0; row < w_size; row+=1) {
////                const uint32_t ref_ind = row*2;
////                const uint32_t row_ind = row*2 + 38*4;
//////                float coef = (0.355768f + 0.397396f*cosf(float(row - w_size/2)*(2.0f*3.14/float(w_size))) - 0.144232f*cosf(float(row - w_size/2)*(4.0f*3.14/float(w_size))) + 0.012604f*cosf(float(row - w_size/2)*(6.0f*3.14/float(w_size))))/float(w_size);
//////                float coef = (0.5f + 0.6f*cosf(float(row - w_size/2)*(2.0f*3.14/float(w_size))))/float(w_size);
////                float coef = (1.0f - abs(float(row - w_size/2)/float(w_size))*2.0f)/float(w_size);

////                data_ref[ref_ind] = iq_data[row_ind]*coef;
////                data_ref[ref_ind+1] = iq_data[row_ind+1]*coef;
////            }

//            for (int row = 0; row < data_size; row+=1) {
//                const uint32_t row_ind = row*2;
//                float i1 = iq_data[row_ind], q1 = iq_data[row_ind+1];

//                signalA[row] = (10*log10f(i1*i1 + q1*q1 + 0.1f) - 50.0f)*10.0f;
//                signalPh[row] = atan2f(q1, i1)/3.14f*500;
//            }

//            int chart_size = data_size;


////            uint32_t base = (928 + 48)/dec_f;
////            uint32_t offset = 0;

////            for(uint32_t i = 0; i < 5; i++) {
////                for (uint32_t row = offset; row < offset + base - 64; row+=1) {
////                    const uint32_t row_ind = row*2;
////                    float ci = 0, cq = 0;

////                    for(uint16_t i = 0; i < 48; i+=1) {
////                        const uint32_t r_ind = row_ind + i*2;
////                        int32_t r11i = iq_data[r_ind], r11q = iq_data[r_ind+1];
////                        int32_t r12i = iq_data[r_ind+base*2], r12q = iq_data[r_ind+1+base*2];
////                        ci += (int64_t)(r11i*r12i) + (int64_t)(r11q*r12q);
////                        cq += (int64_t)(r11q*r12i) - (int64_t)(r11i*r12q);
////                    }

////                    corrI[row] = ci;
////                    corrQ[row] = cq;

////                    corrA[row] = (5*log10f(float(corrI[row]*corrI[row] + corrQ[row]*corrQ[row]) + 0.1f) - 60.0f)*10.0f;
////                    speed[row] = atan2f(corrQ[row], corrI[row])*1500.0f/(4.0f*3.141592f*float(base*dec_f))*1000;
////                }
////                for (uint32_t row = offset + base - 64 ; row < offset + base; row+=1) {
////                    corrA[row] = (5*log10f(0.1f) - 73.0f)*10.0f;
////                }

////                offset += base;
////                base += 96/dec_f;

////            }
////            for (int row = 0; row < chart_size; row+=1) {
////                const uint32_t row_ind = row*2;
////                float ci = 0, cq = 0, c2i = 0, c2q = 0;

////                for(uint16_t i = 0; i < w_size*2; i+=1) {
////                    const uint32_t r_ind = row_ind + i*2;
////                    int32_t r11i = iq_data[r_ind], r11q = iq_data[r_ind+1];
////                    int32_t r12i = iq_data[r_ind+w_size*2], r12q = iq_data[r_ind+1+w_size*2];
////                    int32_t r21i = iq_data[r_ind+w_size2*2], r21q = iq_data[r_ind+1+w_size2*2];
////                    int32_t r22i = iq_data[r_ind+(w_size2+w_size)*2], r22q = iq_data[r_ind+1+(w_size2+w_size)*2];

////                    ci += (int64_t)(r11i*r12i) + (int64_t)(r11q*r12q);
////                    cq += (int64_t)(r11q*r12i) - (int64_t)(r11i*r12q);
//////                    ci += (int64_t)(r21i*r22i) + (int64_t)(r21q*r22q);
//////                    cq += (int64_t)(r21q*r22i) - (int64_t)(r21i*r22q);

////                    c2i += (int64_t)(r11i*r21i) + (int64_t)(r11q*r21q);
////                    c2q += (int64_t)(r11q*r21i) - (int64_t)(r11i*r21q);
//////                    c2i += (int64_t)(r12i*r22i) + (int64_t)(r12q*r22q);
//////                    c2q += (int64_t)(r12q*r22i) - (int64_t)(r12i*r22q);
////                }



////                corrI[row] = ci;
////                corrQ[row] = cq;
////                corr2I[row] = c2i;
////                corr2Q[row] = c2q;

////                speed[row] = atan2f(corrQ[row], corrI[row])*1500.0f/(4.0f*3.141592f*float(w_size*dec_f))*1000.0f;
////                corrA[row] = (5*log10f(float(corrI[row]*corrI[row] + corrQ[row]*corrQ[row]) + 0.1f) - 73.0f)*10.0f;

////                speed2[row] = atan2f(corr2Q[row], corr2I[row])*1500.0f/(4.0f*3.141592f*float(w_size2*dec_f))*1000.0f;
////                corr2A[row] = (5*log10f(float(corr2I[row]*corr2I[row] + corr2Q[row]*corr2Q[row]) + 0.1f) - 73.0f)*10.0f;
////            }

////            for (int row = 0; row < chart_size - 14*24; row+=1) {
////                corrA[row] = (5*log10f(float(corrI[row]*corrI[row] + corrQ[row]*corrQ[row]) + 0.1f) - 73.0f)*10.0f;
////                corr2A[row] = (5*log10f(float(corr2I[row]*corr2I[row] + corr2Q[row]*corr2Q[row] + corrI[row]*corrI[row] + corrQ[row]*corrQ[row]) + 0.1f) - 73.0f)*10.0f;
////                speed[row] = atan2f(corrQ[row], corrI[row])*1500.0f/(4.0f*3.141592f*float(w_size*dec_f));
////                speed2[row] = atan2f(corr2Q[row], corr2I[row])*1500.0f/(4.0f*3.141592f*float(w_size2*dec_f));

////                float speed_dif = speed[row] - speed2[row];
////                float resolver[] = {1.30208333f, -1.822916666, -0.5208333333, 0.78125, -1.30208333f, 1.822916666, 0.5208333333, -0.78125};
////                float corrector[] = {1.30208333f, 1.30208333f, 1.30208333f, 1.30208333f, -1.30208333f, -1.30208333f, -1.30208333f, -1.30208333f};

////                float min = fabs(speed_dif);
////                int32_t min_ind = -1;
////                for(uint32_t i = 0; i < sizeof(resolver)/4; i++) {
////                    float absdif = fabs(speed_dif - resolver[i]);
////                    if(min > absdif) {
////                        min = absdif;
////                        min_ind = i;
////                    }
////                }

////                if(min_ind >= 0) {
////                    speed2[row] += corrector[min_ind];
////                }

////                speed[row] = min;

////                speed[row] *= 200.0f;
////                speed2[row] *= 200.0f;

////            }

//            float scale_y = (float)height*(1.0f*(1.0f/1000.0f));
//            float scale_x = (float)(waterfall_width - 100)/(float)(chart_size);

//            float last_signal_amp_ofset = (float)signalA[0]*scale_y;
//            float last_offset_q = (float)signalPh[0]*scale_y;
//            float last_col = 100;

//            QImage tmp_img = QImage((uint8_t*)m_dataImage, width, height, width*2, QImage::Format_RGB555);
//            QPainter p(&tmp_img);
//            QPen pen_i;
//            pen_i.setWidth(2);
//            pen_i.setColor(QColor::fromRgb(70, 200, 0));

//            QPen pen_q;
//            pen_q.setWidth(2);
//            pen_q.setColor(QColor::fromRgb(50, 20, 250));

//            for (int row = 0; row < data_size; row++) {
//                float signal_amp_ofset =  height/2 - signalA[row]*scale_y;
//                float offset_q =  height/2 - signalPh[row]*scale_y;
//                float offset_x = (float)row*scale_x+100;

//                p.setPen(pen_i);
//                p.drawLine(last_col, last_signal_amp_ofset, offset_x, signal_amp_ofset);

////                if(signalA[row]>0) {
//                    p.setPen(pen_q);
//                    p.drawLine(last_col, last_offset_q, offset_x, offset_q);
////                }


//                last_col = offset_x;
//                last_signal_amp_ofset = signal_amp_ofset;
//                last_offset_q = offset_q;
//            }
//        }
//    }

////    if(m_oscVis) {


////        float scale_scope_w = (float)height / (float)(waterfall_width - 100);
////        QImage tmp_img = QImage((uint8_t*)m_dataImage, width, height, width*2, QImage::Format_RGB555);
////        QPainter p(&tmp_img);
////        QPen pen;
////        pen.setWidth(2);




////        int pool_index = poolLastIndex() - 1;

////        if(pool_index >= 0) {
////            float* raw_col = (float*)m_pool[pool_index].chartData().data();
////            int data_size = m_pool[pool_index].chartData().size()/2;

//////            QVector<int32_t> dataI, dataQ, dataPH;
//////            dataI.resize(data_size);
//////            dataQ.resize(data_size);
//////            dataPH.resize(data_size);
//////            const int mul_resol = 10;

//////            for (int row = 0; row < data_size; row++) {
//////                float a = float(row%4)*M_PI_2;
//////                float b = float((int32_t)raw_col[row]*mul_resol);
//////                dataI[row] = int(b*cos(a));
//////                dataQ[row] = int(b*sin(a));

//////                dataI[row] = (int32_t)raw_col[row];
//////            }

//////            int wsize = 32;
//////            int fil_num = 1;
//////            for(int fil_n = 0; fil_n < fil_num; fil_n++) {
//////                int summI = 0, summQ = 0;

//////                for (int row = 0; row < wsize; row++) {
//////                    summI +=  dataI[row];
//////                    summQ +=  dataQ[row];
//////                }

//////                for (int row = 0; row < data_size - wsize; row++) {
//////                    int summ = summI;
//////                    summI -= dataI[row];
//////                    summI += dataI[row + wsize];
//////                    dataI[row] = summ;

//////                    summ = summQ;
//////                    summQ -= dataQ[row];
//////                    summQ += dataQ[row + wsize];
//////                    dataQ[row] = summ;
//////                }
//////            }

//////            wsize = 8;
//////            for(int fil_n = 0; fil_n < fil_num; fil_n++) {
//////                int summI = 0, summQ = 0;

//////                for (int row = 0; row < wsize; row++) {
//////                    summI +=  dataI[row];
//////                    summQ +=  dataQ[row];
//////                }

//////                for (int row = 0; row < data_size - wsize; row++) {
//////                    int summ = summI;
//////                    summI -= dataI[row];
//////                    summI += dataI[row + wsize];
//////                    dataI[row] = summ;

//////                    summ = summQ;
//////                    summQ -= dataQ[row];
//////                    summQ += dataQ[row + wsize];
//////                    dataQ[row] = summ;
//////                }
//////            }



//////            for (int row = 0; row < data_size; row++) {
//////                float i_val =  (float)(dataI[row])*(1.0f/(float)(mul_resol*wsize*wsize));
//////                float q_val =  (float)(dataQ[row])*(1.0f/(float)(mul_resol*wsize*wsize));

//////                float sq_amp = i_val*i_val + q_val*q_val;

//////                dataI[row] = std::atan2(i_val, q_val)*(180.0f/M_PI*1000);
//////                dataQ[row] = std::sqrt(sq_amp);
//////            }


//////            for (int row = 0; row < data_size - 6000; row++) {
//////                dataPH[row] = dataI[row] - dataI[row+6000];
//////            }

////////            const int base_dif_a = 4;
////////            int valid_cnt = 0;
////////            for (int row = 0; row < data_size - base_dif_a; row++) {
////////                int abs_dif = std::abs(dataI[row] - dataI[row+base_dif_a]);

////////                if(abs_dif > 32000) {
////////                    dataI[row] = -200000;
////////                    valid_cnt = 0;
////////                }

////////                if(valid_cnt < 0) {
////////                    dataI[row] = -200000;
////////                }

////////                valid_cnt++;
////////            }

////////            valid_cnt = 200;
//////            int32_t ph_offset = 0;
//////            for (int row = 0; row < data_size; row++) {

//////                dataI[row] -= ph_offset;

//////                if(dataI[row] < -180000) {
//////                    dataI[row] += 360000;
//////                }

//////                if(dataI[row] > 180000) {
//////                    dataI[row] -= 360000;
//////                }


//////                if(dataPH[row] < -180000) {
//////                    dataPH[row] += 360000;
//////                }

//////                if(dataPH[row] > 180000) {
//////                    dataPH[row] -= 360000;
//////                }

//////                if(std::abs(dataQ[row]) < 100) {
//////                    dataPH[row] = -200000;
//////                    dataI[row] = -200000;
//////                }


//////            }

//////            for (int row = 0; row < data_size; row+=2) {
//////                 float pow = sqrtf(sqrtf((float)raw_col[row]*(float)raw_col[row] + (float)raw_col[row+1]*(float)raw_col[row+1]));
//////                 raw_col[row] = pow*(1.0f/10);
//////                 raw_col[row+1] = 0;
//////            }



////            float scale_y = (float)height*(1.0f*(1.0f/1.0f));
////            float scale_x = (float)(waterfall_width)/(float)(data_size);

////            float last_offset_y = (float)raw_col[0]*scale_y;
////            float last_col = 0;


////            pen.setColor(QColor::fromRgb(70, 200, 0));
////            p.setPen(pen);
////            for (int row = 0; row < data_size; row+=2) {
////                float offset_y =  height/2 - (float)raw_col[row]*scale_y;
////                float offset_x = (float)row*scale_x;

////                if(raw_col[row] != 0) {
////                    p.drawLine(last_col, last_offset_y, offset_x, offset_y);
////                }

////                last_col = offset_x;

////                last_offset_y = offset_y;
////            }

////            scale_y = (float)height*(1.0f*(1.0f/250));
////            last_offset_y =(float)raw_col[1]*scale_y;
////            last_col = 0;

////            pen.setColor(QColor::fromRgb(255, 50, 255));
////            p.setPen(pen);
////            for (int row = 1; row < data_size; row+=2) {
////                float offset_y =  height - (float)raw_col[row]*scale_y;
////                float offset_x = (float)(row-1)*scale_x;

////                if(raw_col[row] != 0) {
////                    p.drawLine(last_col, last_offset_y, offset_x, offset_y);
////                }

////                last_col = offset_x;

////                last_offset_y = offset_y;
////            }


//////            scale_y = (float)height*(1.0f/360000.0f);
//////            last_offset_y = (float)dataI[0]*scale_y;
//////            last_col = 0;

//////            pen.setColor(QColor::fromRgb(255, 50, 255));
//////            p.setPen(pen);
//////            for (int row = 1; row < data_size; row++) {
//////                float offset_y =  height*0.5f - (float)dataI[row]*scale_y;
//////                float offset_x = (float)row*scale_x;

//////                if(last_offset_y <= height && offset_y <= height) {
//////                    p.drawLine(last_col, last_offset_y, offset_x, offset_y);
//////                }

//////                last_col = offset_x;

//////                last_offset_y = offset_y;
//////            }

//////            last_offset_y = (float)dataPH[0]*scale_y;
//////            last_col = 0;

//////            pen.setColor(QColor::fromRgb(10, 50, 255));
//////            p.setPen(pen);
//////            for (int row = 1; row < data_size; row++) {
//////                float offset_y =  height*0.5f - (float)dataPH[row]*scale_y;
//////                float offset_x = (float)row*scale_x;

//////                if(last_offset_y <= height && offset_y <= height) {
//////                    p.drawLine(last_col, last_offset_y, offset_x, offset_y);
//////                }
//////                last_col = offset_x;

//////                last_offset_y = offset_y;
//////            }

////        }


////    }

//    if(m_distSonarVis) {
//        int dist = m_prevValueCash.distData;
//        if(dist >= 0) {
//            int index_dist = (int)((float)dist/(float)m_range*(float)height);
//            if(index_dist < 0) {
//                index_dist = 0;
//            } else if(index_dist > height - 2) {
//                index_dist = height - 2;
//            }

//            for(int col = 1;  col < m_prevLineWidth; col++) {
//                m_dataImage[width - col + index_dist*width] = m_colorDist;
//                m_dataImage[width - col + (index_dist + 1)*width] = m_colorDist;
//            }
//        }

//        for(int col = 1;  col < waterfall_width; col++) {
//            int val_col = (m_valueCashStart + col);
//            if(val_col >= waterfall_width) {
//                val_col -= waterfall_width;
//            }

//            int dist = m_valueCash[val_col].distData;
//            if(dist >= 0) {
//                int index_dist = (int)((float)dist/(float)m_range*(float)height);
//                if(index_dist < 0) {
//                    index_dist = 0;
//                } else if(index_dist > height - 2) {
//                    index_dist = height - 2;
//                }

//                m_dataImage[col - 1 + index_dist*width] = m_colorDist;
//                m_dataImage[col - 1 + (index_dist + 1)*width] = m_colorDist;
//                m_dataImage[col + index_dist*width] = m_colorDist;
//                m_dataImage[col + (index_dist + 1)*width] = m_colorDist;
//            }
//        }
//    }

//    if(m_distProcessingVis) {

//        QImage tmp_img = QImage((uint8_t*)m_dataImage, width, height, width*2, QImage::Format_RGB555);
//        QPainter p1(&tmp_img);

//        QPen pen1;
//        pen1.setWidth(1);
//        pen1.setColor(QColor::fromRgb(100, 255, 0));
//        p1.setPen(pen1);

//        QPen pen2;
//        pen2.setWidth(2);
//        pen2.setColor(QColor::fromRgb(100, 255, 0));

//        int last_dist = (int)((float)m_valueCash[m_valueCashStart].processingDistData/(float)m_range*(float)height);

//        for(int col = 1;  col < waterfall_width; col++) {
//            int val_col = (m_valueCashStart + col);
//            if(val_col >= waterfall_width) {
//                val_col -= waterfall_width;
//            }

//            int dist = m_valueCash[val_col].processingDistData;

//            if(dist != INT_MIN) {
//                int index_dist = (int)((float)dist/(float)m_range*(float)height);
//                if(index_dist < 1) {
//                    index_dist = 1;
//                } else if(index_dist > height - 1) {
//                    index_dist = height - 1;
//                }

////                p.drawLine(col - 1, last_dist, col, index_dist);
//                switch(_bottomTrackTheme) {
//                case 0:
//                    p1.setPen(pen1);
//                    p1.drawLine(col - 1, last_dist, col, index_dist);
//                    break;
//                case 1:
//                    p1.setPen(pen2);
//                    p1.drawLine(col - 1, last_dist, col, index_dist);
//                    break;
//                case 2:
//                    p1.setPen(pen1);
//                    p1.drawPoint(col, index_dist);
//                    break;
//                case 3:
//                    p1.setPen(pen2);
//                    p1.drawPoint(col, index_dist);
//                    break;
//                case 4:
//                    p1.setPen(pen1);
//                    p1.drawLine(col - 1, last_dist, col, index_dist);
//                    p1.setPen(pen2);
//                    p1.drawPoint(col, index_dist);
//                    break;
//                }

//                last_dist = index_dist;

////                m_dataImage[col - 1 + index_dist*width] = m_colorDistProc;
////                m_dataImage[col - 1 + (index_dist + 1)*width] = m_colorDistProc;
////                m_dataImage[col + index_dist*width] = m_colorDistProc;
////                m_dataImage[col + (index_dist + 1)*width] = m_colorDistProc;
//            }
//        }
//    }

//    m_image = QImage((uint8_t*)m_dataImage, width, height, width*2, QImage::Format_RGB555);
////    QTransform tr;
////    tr.rotate(90);
////    m_image = m_image.transformed(tr, Qt::FastTransformation);
//}

//QImage Dataset::getImage(QSize size) {
//    if(m_image.size() != size) { renderValue();  }

//    flags.renderImage |= flags.renderValue;
//    if(flags.renderImage) {
//        int full_range = m_range*2;
//        int full_offset = -m_range + m_offset;

//        updateImage(size.width(), size.height());

//        QPainter p(&m_image);

//        p.setPen(QColor::fromRgb(200, 200, 200));
//        p.setFont(QFont("Asap", 14, QFont::Normal));

//        int nbr_hor_div = m_verticalGridNum;
//        for (int i = 1; i < nbr_hor_div; i++) {
//            int offset_y = m_image.height()*i/nbr_hor_div;
//            p.setPen(QColor::fromRgb(100, 100, 100));
//            p.drawLine(0, offset_y, m_image.width(), offset_y);

//            p.setPen(QColor::fromRgb(200, 200, 200));

//            float range_text = (float)(full_range*i/nbr_hor_div + full_offset)*m_legendMultiply;
//            p.drawText(m_image.width() - m_prevLineWidth - 70, offset_y - 10, QString::number((double)range_text) + QStringLiteral(" m"));
//            if(_is_velocityVis) {
//                float velo_text = (float)(_velocityRange*(float(nbr_hor_div)/2.0f - (float)i)/(float)nbr_hor_div);
//                p.drawText(m_image.width() - m_prevLineWidth - 170, offset_y - 10, QString::number((double)velo_text) + QStringLiteral(" m/s"));
//            }
//        }

//        p.drawLine(m_image.width() - m_prevLineWidth, 0, m_image.width() - m_prevLineWidth, m_image.height());
//        p.setPen(QColor::fromRgb(250, 250, 250));
//        p.setFont(QFont("Asap", 26, QFont::Normal));
//        QString range_text = QString::number((double)((full_range + full_offset)*m_legendMultiply)) + QStringLiteral(" m");
//        p.drawText(m_image.width() - m_prevLineWidth - 30 - range_text.count()*15, m_image.height() - 10, range_text);

////        QPainterPath path;
////        path.moveTo(0, 0);
////        path.cubicTo(80, 0, 50, 50, 500, 500);
////        path.quadTo(350, 150, 500, 500);
////        p.drawPoint(350, 150);
////        p.drawPath(path);

//        if(m_distSonarVis) {
//            p.setPen(QColor::fromRgb(250, 70, 0));
//            int disp_dist = m_prevValueCash.distData;
//            if(disp_dist >= 0) {
//                QString rangefinder_text = QString::number((double)(m_prevValueCash.distData)*0.001) + QStringLiteral(" m");
//                p.drawText(m_image.width() - m_prevLineWidth - 30 - rangefinder_text.count()*15, m_image.height() - 60, rangefinder_text);
//            }
//        }

////        if(m_TemperatureVis) {
////            p.setPen(QColor::fromRgb(250, 70, 0));
//////            qInfo("Temperature %f", m_prevValueCash.temperature);
////            QString rangefinder_text = QString::number((lastTemperature)) + QStringLiteral(" C");
////            p.drawText(m_image.width() - m_prevLineWidth - 400, m_image.height() - 60, rangefinder_text);
////        }

//        if(_is_attitudeVis) {
//            p.setFont(QFont("Asap", 16, QFont::Normal));
//            p.setPen(QColor::fromRgb(250, 200, 50));
//            p.drawText(m_image.width() - m_prevLineWidth - 250, m_image.height() - 70, QString("Yaw: %1").arg(_lastYaw));
//            p.drawText(m_image.width() - m_prevLineWidth - 250, m_image.height() - 45, QString("Pitch: %1").arg(_lastPitch));
//            p.drawText(m_image.width() - m_prevLineWidth - 250, m_image.height() - 20, QString("Roll: %1").arg(_lastRoll));
//        }

//        if(_is_encoderVis) {
//            p.setFont(QFont("Asap", 16, QFont::Normal));
//            p.setPen(QColor::fromRgb(250, 200, 50));
//            p.drawText(m_image.width() - m_prevLineWidth - 250, m_image.height() - 100, QString("Encoder: %1").arg(_lastEncoder*0.000001f));
//        }
//    }

//    return m_image;
//}

