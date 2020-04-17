#include "SonarDriver.h"


int SonarDriver::chartSamples() {
    return m_chartSamplesCnt;
}

void SonarDriver::setChartSamples(int) {
}


double SonarDriver::chartSamplesSlider() {
    return 80;
}

void SonarDriver::setChartSamplesSlider(double pos) {
    m_chartSamplesCnt = (int)(pos*10);
    emit chartSamplesChanged(m_chartSamplesCnt);
//    emit chartSamplesSliderChanged(pos);
}

double SonarDriver::chartSamplSliderStep() {
    return 100.0/20;
}
