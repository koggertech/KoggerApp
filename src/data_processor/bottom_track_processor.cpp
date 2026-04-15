#include "bottom_track_processor.h"

#include <algorithm>
#include <cmath>
#include <QDebug>
#include "data_processor.h"
#include "dataset.h"


BottomTrackProcessor::BottomTrackProcessor(DataProcessor* parent) :
    dataProcessor_(parent),
    datasetPtr_(nullptr)
{
    qRegisterMetaType<ChannelId>("ChannelId");
    qRegisterMetaType<BottomTrackParam>("BottomTrackParam");
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

void BottomTrackProcessor::bottomTrackProcessing(const DatasetChannel &channel1, const DatasetChannel &channel2, const BottomTrackParam& btP, bool manual, bool redrawAll)
{
    if (btP.windowSize <= 0) {
        return;
    }

    if (!datasetPtr_) {
        qWarning() << "[BT] dataset is null";
        return;
    }

    const int datasetSize = datasetPtr_->sizeThreadSafe();
    if (datasetSize <= 0) {
        return;
    }

    if (btP.indexFrom < 0 || btP.indexTo < 0) {
        return;
    }

    int epoch_start_index = std::clamp(btP.indexFrom, 0, datasetSize);
    int epoch_stop_index  = std::clamp(btP.indexTo,   0, datasetSize);
    if (epoch_start_index >= epoch_stop_index) {
        return;
    }

    int epoch_min_index = std::max(0, epoch_start_index - btP.windowSize/2);
    int epoch_max_index = std::min(datasetSize, epoch_stop_index + btP.windowSize/2);

    if (epoch_max_index <= epoch_min_index) {
        return;
    }

    QMetaObject::invokeMethod(dataProcessor_, "postState", Qt::QueuedConnection, Q_ARG(DataProcessorType, DataProcessorType::kBottomTrack));

    QVector<int32_t> summ;

    float gain_slope = btP.gainSlope;
    float threshold = btP.threshold;

    int istart_ref = 4;
    int init_win_ref = 6;
    int scale_win_ref = 20;

    int16_t c1 = -6, c2 = 6, c3 = 4, c4 = 2, c5 = 0;
    float s2 = 1.04f, s3 = 1.06f, s4 = 1.10f, s5 = 1.15f;
    float t1 = 1.07f;

    if(btP.preset == BottomTrackPreset::BottomTrackOneBeamNarrow) {
        istart_ref = 4;
        init_win_ref = 6;
        scale_win_ref = 35;

        c1 = -3, c2 = 8, c3 = 5, c4 = -1, c5 = -1;
        s2 = 1.015f, s3 = 1.035f, s4 = 1.08f, s5 = 1.12f;
        t1 = 1.04f;
    }

    if(btP.preset == BottomTrackPreset::BottomTrackSideScan) {
        istart_ref = 4;
        init_win_ref = 6;
        scale_win_ref = 12;

        c1 = -3, c2 = -3, c3 = 4, c4 = 2, c5 = 1;
        s2 = 1.1f, s3 = 1.2f, s4 = 1.3f, s5 = 1.4f;
        t1 = 1.2f;
    }

    const int gain_slope_inv = 1000/(gain_slope);
    const int threshold_int = 10*gain_slope_inv*1000*threshold;
    constexpr float kBottomTrackRefResolution = 0.01f; // 10 mm

    int istart = istart_ref;
    int init_win = init_win_ref;
    float init_win_scaled = static_cast<float>(init_win_ref);
    float index_gain_scale = 1.0f;
    int index_gain_scale_q12 = 1 << 12;
    int init_scale_term_q12 = 0;
    int scale_win_q12 = 0;
    bool coeffs_initialized = false;
    float active_resolution = NAN;

    struct EpochConstrants
    {
        float min = NAN;
        float max = NAN;
    };

    QVector<QVector<int32_t>> cash(btP.windowSize);
    QVector<EpochConstrants> constr(btP.windowSize);

    QVector<float> bottom_track(epoch_max_index - epoch_min_index);
    bottom_track.fill(NAN);

    int epoch_counter = 0;

    for(int iepoch = epoch_min_index; iepoch < epoch_max_index; iepoch++) {
        if (canceled()) {
            QMetaObject::invokeMethod(dataProcessor_, "postState", Qt::QueuedConnection, Q_ARG(DataProcessorType, DataProcessorType::kUndefined));
            return;
        }

        Epoch epoch = datasetPtr_->fromIndexBottomTrackCopy(iepoch);
        if (!epoch.isValid()) {
            continue;
        }

        Epoch::Echogram* chart = nullptr;
        uint8_t* data = nullptr;
        int data_size = 0;
        QVector<uint8_t> mergedData;

        bool ch1_avail = epoch.chartAvail(channel1.channelId_, channel1.subChannelId_);
        bool ch2_avail = epoch.chartAvail(channel2.channelId_, channel2.subChannelId_);

        if(ch1_avail && ch2_avail) {
            Epoch::Echogram* chart1 = epoch.chart(channel1.channelId_, channel1.subChannelId_);
            Epoch::Echogram* chart2 = epoch.chart(channel2.channelId_, channel2.subChannelId_);
            if (!chart1 || !chart2) {
                continue;
            }
            uint8_t* data1 = (uint8_t*)chart1->amplitude.constData();
            const int data_size1 = chart1->amplitude.size();

            uint8_t* data2 = (uint8_t*)chart2->amplitude.constData();
            const int data_size2 = chart2->amplitude.size();

            if(chart1->resolution == chart2->resolution && data_size1 == data_size2) {
                mergedData.resize(data_size1);
                uint8_t* mergedDataPtr = mergedData.data();

                for(int idata = 0;idata < data_size1; idata++) {
                    mergedDataPtr[idata] = ((uint16_t)data1[idata] + (uint16_t)data2[idata])>>2; //+ (uint16_t)data2[idata]
                }

                chart = chart1;
                data = mergedDataPtr;
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

        if(data == nullptr || chart == nullptr) {
            continue;
        }

        const float resolution = chart->resolution;
        if (!(resolution > 0.0f)) {
            continue;
        }

        const bool resolution_changed = !coeffs_initialized || std::fabs(resolution - active_resolution) > 1e-6f;
        if (resolution_changed) {
            const float samples_scale = kBottomTrackRefResolution / resolution;

            istart = std::max(0, static_cast<int>(std::lround(static_cast<float>(istart_ref) * samples_scale)));
            init_win_scaled = static_cast<float>(init_win_ref) * samples_scale;
            init_win = std::max(1, static_cast<int>(std::lround(init_win_scaled)));
            index_gain_scale = resolution / kBottomTrackRefResolution;
            index_gain_scale_q12 = std::max(1, static_cast<int>(std::lround(index_gain_scale * static_cast<float>(1 << 12))));
            scale_win_q12 = scale_win_ref << 12;
            init_scale_term_q12 = std::max(1, static_cast<int>(std::lround(init_win_scaled * static_cast<float>(scale_win_q12))));

            if (coeffs_initialized) {
                summ.clear();
                for (auto& col : cash) {
                    col.clear();
                }
                for (auto& c : constr) {
                    c.min = NAN;
                    c.max = NAN;
                }
                epoch_counter = 0;
            }

            active_resolution = resolution;
            coeffs_initialized = true;
        }

        if (data_size <= (istart + init_win)) {
            continue;
        }

        epoch_counter++;


        int cash_ind = (epoch_counter-1)%btP.windowSize;

        int back_cash_ind = ((epoch_counter)%btP.windowSize);
        const int32_t* back_cash_data = cash[back_cash_ind].constData();
        const int back_cash_size = cash[back_cash_ind].size();

        int32_t* summ_data = summ.data();

        if(epoch_counter >= btP.windowSize) {
            for(int i = istart; i < back_cash_size; i++) { summ_data[i] -= back_cash_data[i]; }
        }

        if(cash[cash_ind].size() != data_size) {
            cash[cash_ind].resize(data_size);
        }

        int32_t* cash_data = cash[cash_ind].data();


        uint8_t* data_from = data + istart;
        uint8_t* data_to = data + (istart+init_win);
        uint8_t* data_end = data + data_size;

        int data_acc = 0;
        for(int idata = istart; idata < (istart+init_win); idata++) {
            data_acc += data[idata];
        }

        int avrg_range = init_win;
        for(int idata = istart; (idata + avrg_range) < data_size; idata++) {
            if (data_from >= data_end || data_to >= data_end) {
                break;
            }
            data_acc -= *data_from; data_from++;
            data_acc += *data_to; data_to++;
            while((data_to < data_end) &&
                  (((idata << 12) + init_scale_term_q12) >= ((avrg_range = data_to - data_from) * scale_win_q12))) {
                data_acc += *data_to; data_to++;
            }
            cash_data[idata] = 10*data_acc / (avrg_range);
        }

        const int data_conv_size = data_size - avrg_range;
        for(int idata = istart; ; idata++) {
            const float fidata = idata + init_win_scaled + 1.0f;
            int di1 =  idata;
            int di2 =  fidata*s2;
            int di3 =  fidata*s3;
            int di4 =  fidata*s4;
            int di5 =  fidata*s5;

            if(di5 >= data_conv_size) { break; }

            int calc = (c1*cash_data[di1] + c2*cash_data[di2] + c3*cash_data[di3] + c4*cash_data[di4] + c5*cash_data[di5]);
            const int idata_ref = std::max(0, (idata * index_gain_scale_q12 + (1 << 11)) >> 12);
            calc = calc*gain_slope_inv + calc*idata_ref;

            cash_data[idata] = calc;
        }

        const int col_size = cash[cash_ind].size();
        if(summ.size() < col_size) { summ.resize(col_size); }
        summ_data = summ.data();
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
            float distance = ((max_ind + init_win_scaled + 1.0f) * t1) * chart->resolution;

            if(epoch_counter >= btP.windowSize) {
                if(btP.verticalGap > 0) {
                    const int32_t* center_cash_data = cash[win_center_index].constData();
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
                        distance = ((max_gap_ind + init_win_scaled + 1.0f) * t1) * chart->resolution;
                    }
                }
            }

            int btIndex = (epoch_counter >= btP.windowSize)
                    ? (iepoch - epoch_min_index - btP.windowSize/2)
                    : (iepoch - epoch_min_index - epoch_counter/2);
            if (btIndex >= 0 && btIndex < bottom_track.size()) {
                bottom_track[btIndex] = distance;
            }
        }
    }

    bool flushEachEpoch = datasetPtr_->getState() == Dataset::DatasetState::kConnection;
#ifdef SEPARATE_READING
    flushEachEpoch = true;
#endif
    const int batchLimit = flushEachEpoch ? 0 : 512;

    QVector<BottomTrackUpdate> batch;
    batch.reserve(batchLimit > 0 ? batchLimit : 8);
    auto flushBatch = [&]() {
        if (!dataProcessor_ || batch.isEmpty()) {
            return;
        }
        QMetaObject::invokeMethod(dataProcessor_, "postDistCompletedByProcessingBatch", Qt::QueuedConnection,
                                  Q_ARG(QVector<BottomTrackUpdate>, batch));
        batch.clear();
    };

    for(int iepoch = epoch_start_index; iepoch < epoch_stop_index; iepoch++) {
        if (canceled()) {
            QMetaObject::invokeMethod(dataProcessor_, "postState", Qt::QueuedConnection, Q_ARG(DataProcessorType, DataProcessorType::kUndefined));
            return;
        }

        const int btIndex = iepoch - epoch_min_index;
        if (btIndex < 0 || btIndex >= bottom_track.size()) {
            continue;
        }

        Epoch epPtr = datasetPtr_->fromIndexBottomTrackCopy(iepoch);

        if(epPtr.chartAvail(channel1.channelId_, channel1.subChannelId_)) {
            Epoch::Echogram* chart = epPtr.chart(channel1.channelId_, channel1.subChannelId_);
            if(chart && chart->bottomProcessing.source < Epoch::DistProcessing::DistanceSource::DistanceSourceDirectHand) {
                float dist = bottom_track[btIndex];
                batch.push_back(BottomTrackUpdate{iepoch, channel1.channelId_, dist});
            }
        }

        if(epPtr.chartAvail(channel2.channelId_, channel2.subChannelId_)) {
            Epoch::Echogram* chart = epPtr.chart(channel2.channelId_, channel2.subChannelId_);
            if(chart && chart->bottomProcessing.source < Epoch::DistProcessing::DistanceSource::DistanceSourceDirectHand) {
                float dist = bottom_track[btIndex];
                batch.push_back(BottomTrackUpdate{iepoch, channel2.channelId_, dist});
            }
        }

        if (flushEachEpoch) {
            flushBatch();
            continue;
        }

        if (batchLimit > 0 && batch.size() >= batchLimit) {
            flushBatch();
        }
    }

    flushBatch();

    QMetaObject::invokeMethod(dataProcessor_, "postState", Qt::QueuedConnection, Q_ARG(DataProcessorType, DataProcessorType::kUndefined));
    QMetaObject::invokeMethod(dataProcessor_, "postLastBottomTrackEpochChanged", Qt::QueuedConnection,
                              Q_ARG(ChannelId, channel1.channelId_),
                              Q_ARG(int, epoch_stop_index),
                              Q_ARG(BottomTrackParam, btP),
                              Q_ARG(bool, manual),
                              Q_ARG(bool, redrawAll));
}

bool BottomTrackProcessor::canceled() const noexcept
{
    return dataProcessor_ && dataProcessor_->isCancelRequested();
}
