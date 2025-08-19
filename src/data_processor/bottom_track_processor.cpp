#include "bottom_track_processor.h"

#include <QDebug>
#include "data_processor.h"
#include "dataset.h"


BottomTrackProcessor::BottomTrackProcessor(DataProcessor* parent) :
    dataProcessor_(parent),
    datasetPtr_(nullptr)
{
}

BottomTrackProcessor::~BottomTrackProcessor()
{
}

void BottomTrackProcessor::clear()
{

}

void BottomTrackProcessor::setDatasetPtr(Dataset *datasetPtr)
{
    datasetPtr_ = datasetPtr;
}

void BottomTrackProcessor::bottomTrackProcessing(const DatasetChannel &channel1, const DatasetChannel &channel2, const BottomTrackParam& btP, bool manual)
{
    auto size = btP.indexTo + btP.windowSize / 2;

    if(btP.indexFrom < 0 || btP.indexTo < 0) { return; }

    dataProcessor_->changeState(DataProcessorType::kBottomTrack);

    int epoch_min_index = btP.indexFrom - btP.windowSize/2;

    if(epoch_min_index < 0) {
        epoch_min_index = 0;
    }

    int epoch_max_index = btP.indexTo + btP.windowSize/2;

    if(epoch_max_index >= size) {
        epoch_max_index = size;
    }

    QVector<int32_t> summ;

    float gain_slope = btP.gainSlope;
    float threshold = btP.threshold;

    int istart = 4;
    int init_win = 6;
    int scale_win = 20;

    int16_t c1 = -6, c2 = 6, c3 = 4, c4 = 2, c5 = 0;
    float s2 = 1.04f, s3 = 1.06f, s4 = 1.10f, s5 = 1.15f;
    float t1 = 1.07;

    if(btP.preset == BottomTrackPreset::BottomTrackOneBeamNarrow) {
        istart = 4;
        init_win = 6;
        scale_win = 35;

        c1 = -3, c2 = 8, c3 = 5, c4 = -1, c5 = -1;
        s2 = 1.015f, s3 = 1.035f, s4 = 1.08f, s5 = 1.12f;
        t1 = 1.04;
    }

    if(btP.preset == BottomTrackPreset::BottomTrackSideScan) {
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

    QVector<QVector<int32_t>> cash(btP.windowSize);
    QVector<EpochConstrants> constr(btP.windowSize);

    QVector<float> bottom_track(epoch_max_index - epoch_min_index);
    bottom_track.fill(NAN);

    int epoch_counter = 0;

    for(int iepoch = epoch_min_index; iepoch < epoch_max_index; iepoch++) {

        Epoch epoch = datasetPtr_->fromIndexCopy(iepoch);

        Epoch::Echogram* chart = NULL;
        uint8_t* data = NULL;
        int data_size = 0;

        bool ch1_avail = epoch.chartAvail(channel1.channelId_, channel1.subChannelId_);
        bool ch2_avail = epoch.chartAvail(channel2.channelId_, channel2.subChannelId_);

        if(ch1_avail && ch2_avail) {
            Epoch::Echogram* chart1 = epoch.chart(channel1.channelId_, channel1.subChannelId_);
            uint8_t* data1 = (uint8_t*)chart1->amplitude.constData();
            const int data_size1 = chart1->amplitude.size();

            Epoch::Echogram* chart2 = epoch.chart(channel2.channelId_, channel2.subChannelId_);
            uint8_t* data2 = (uint8_t*)chart2->amplitude.constData();
            const int data_size2 = chart2->amplitude.size();

            if(chart1->resolution == chart2->resolution && data_size1 == data_size2) {
                QVector<uint8_t> data12(data_size1);
                uint8_t* data12_data = (uint8_t*)data12.constData();

                for(int idata = 0;idata < data_size1; idata++) {
                    data12_data[idata] = ((uint16_t)data1[idata] + (uint16_t)data2[idata])>>2; //+ (uint16_t)data2[idata]
                }

                chart = chart1;
                data = data12_data;
                data_size = data_size1;
            } else {
                chart = chart1;
                data = data1;
                data_size = data_size1;
            }
        } else if(ch1_avail && !ch2_avail) {
            chart = epoch.chart(channel1.channelId_, channel1.subChannelId_);
            data = (uint8_t*)chart->amplitude.constData();
            data_size = chart->amplitude.size();
        } else if(!ch1_avail && ch2_avail) {
            chart = epoch.chart(channel2.channelId_, channel2.subChannelId_);
            data = (uint8_t*)chart->amplitude.constData();
            data_size = chart->amplitude.size();
        }

        if(data == NULL) {
            continue;
        }

        epoch_counter++;


        int cash_ind = (epoch_counter-1)%btP.windowSize;

        int back_cash_ind = ((epoch_counter)%btP.windowSize);
        int32_t* back_cash_data = (int32_t*)cash[back_cash_ind].constData();
        const int back_cash_size = cash[back_cash_ind].size();

        int32_t* summ_data = (int32_t*)summ.constData();

        if(epoch_counter >= btP.windowSize) {
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

        const int win_center_index = (epoch_counter - 1 + btP.windowSize/2)%btP.windowSize;

        float search_from_distance = btP.minDistance;
        float search_to_distance = btP.maxDistance;

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


        int max_val = threshold_int*btP.windowSize;
        int max_ind = -1;
        for(int i = start_search_index; i < end_search_index ; i++) {
            if(max_val < summ_data[i]) {
                max_val = summ_data[i];
                max_ind = i;
            }
        }

        if(max_ind > 0) {
            float distance = ((max_ind+init_win+1)*t1)*chart->resolution;

            if(epoch_counter >= btP.windowSize) {
                if(btP.verticalGap > 0) {
                    int32_t* center_cash_data = (int32_t*)cash[win_center_index].constData();
                    const int center_cash_size = cash[win_center_index].size();

                    int start_gap_index = max_ind*(1.0f-btP.verticalGap);
                    int end_gap_index = max_ind*(1.0f+btP.verticalGap);

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

                bottom_track[iepoch - epoch_min_index - btP.windowSize/2] = distance;
            } else {
                bottom_track[iepoch - epoch_min_index - epoch_counter/2] = distance;
            }
        }
    }


    int epoch_start_index = btP.indexFrom;

    if(epoch_start_index < 0) {
        epoch_start_index = 0;
    }

    int epoch_stop_index = btP.indexTo;

    if(epoch_stop_index >= size) {
        epoch_stop_index = size;
    }

    for(int iepoch = epoch_start_index; iepoch < epoch_stop_index; iepoch++) {
        Epoch epPtr = datasetPtr_->fromIndexCopy(iepoch);

        if(epPtr.chartAvail(channel1.channelId_, channel1.subChannelId_)) {
            Epoch::Echogram* chart = epPtr.chart(channel1.channelId_, channel1.subChannelId_);
            if(chart->bottomProcessing.source < Epoch::DistProcessing::DistanceSourceDirectHand) {
                float dist = bottom_track[iepoch - epoch_min_index];
                emit dataProcessor_->distCompletedByProcessing(iepoch, channel1.channelId_, dist); //запись в датасет
            }
        }

        if(epPtr.chartAvail(channel2.channelId_, channel2.subChannelId_)) {
            Epoch::Echogram* chart = epPtr.chart(channel2.channelId_, channel2.subChannelId_);
            if(chart->bottomProcessing.source < Epoch::DistProcessing::DistanceSourceDirectHand) {
                float dist = bottom_track[iepoch - epoch_min_index];
                emit dataProcessor_->distCompletedByProcessing(iepoch, channel2.channelId_, dist);
            }
        }
    }

    dataProcessor_->changeState(DataProcessorType::kUndefined);

    emit dataProcessor_->lastBottomTrackEpochChanged(channel1.channelId_, size, btP, manual);
}
