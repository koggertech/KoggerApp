#include "SonarDriverInterface.h"


int SonarDriverInterface::chartSamplesSlider() {
    return sliderPos(chartSampleArray, chartSamples());
}
void SonarDriverInterface::setChartSamplesSlider(int pos) {
    setChartSamples(valuePos(chartSampleArray, pos));
}
int SonarDriverInterface::chartSamplSliderCount() {
    return chartSampleArray.size() - 1;
}


int SonarDriverInterface::chartResolutionSlider() {
    return sliderPos(chartResolutionArray, chartResolution());
}
void SonarDriverInterface::setChartResolutionSlider(int pos) {
    setChartResolution(valuePos(chartResolutionArray, pos));
}
int SonarDriverInterface::chartResolutionSliderCount() {
    return chartResolutionArray.size() - 1;
}


void SonarDriverInterface::setCh1PeriodSlider(int pos) {
    setCh1Period(valuePos(chPeriodArray, pos));
}
int SonarDriverInterface::ch1PeriodSlider() {
    return sliderPos(chPeriodArray, ch1Period());
}
int SonarDriverInterface::ch1PeriodSliderCount() {
    return chPeriodArray.size() - 1;
}


void SonarDriverInterface::setCh2PeriodSlider(int pos) {
    setCh2Period(valuePos(chPeriodArray, pos));
}
int SonarDriverInterface::ch2PeriodSlider() {
    return sliderPos(chPeriodArray, ch2Period());
}
int SonarDriverInterface::ch2PeriodSliderCount() {
    return chPeriodArray.size() - 1;
}


void SonarDriverInterface::setTransFreqSlider(int pos) {
    setTransFreq(valuePos(freqArray, pos));
}
int SonarDriverInterface::transFreqSlider() {
    return sliderPos(freqArray, transFreq());
}
int SonarDriverInterface::transFreqSliderCount() {
    return freqArray.size() - 1;
}


void SonarDriverInterface::setTransPulseSlider(int pos) {
    setTransPulse(valuePos(pulseArray, pos));
}
int SonarDriverInterface::transPulseSlider() {
    return sliderPos(pulseArray, transPulse());
}
int SonarDriverInterface::transPulseSliderCount() {
    return pulseArray.size() - 1;
}


void SonarDriverInterface::setTransBoostSlider(int pos) {
    setTransBoost(valuePos(boostArray, pos));
}
int SonarDriverInterface::transBoostSlider() {
    return sliderPos(boostArray, transBoost());
}
int SonarDriverInterface::transBoostSliderCount() {
    return boostArray.size() - 1;
}

int SonarDriverInterface::sliderPos(QVector<int> data, int value) {
    for(int i = 0; i < data.size(); i++) {
        if(data[i] >= value) {
            return i;
        }
    }
    return data.back();
}

int SonarDriverInterface::valuePos(QVector<int> data, int pos) {
    int arr_pos = qMin(pos, data.size() - 1);
    return data[arr_pos];
}
