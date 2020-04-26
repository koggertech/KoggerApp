#ifndef SONARDRIVERINTERFACE_H
#define SONARDRIVERINTERFACE_H

#include "SonarDriver.h"

class SonarDriverInterface : public SonarDriver
{
    Q_OBJECT
public:
    explicit SonarDriverInterface(QObject *parent = nullptr) :
        SonarDriver(parent){
    }

    Q_PROPERTY(int chartSamples READ chartSamples WRITE setChartSamples NOTIFY chartSetupChanged)
    Q_PROPERTY(int chartSamplSlider READ chartSamplesSlider WRITE setChartSamplesSlider NOTIFY chartSetupChanged)
    Q_PROPERTY(int chartSamplSliderCount READ chartSamplSliderCount NOTIFY sliderChanged)
    void setChartSamplesSlider(int pos);
    int chartSamplesSlider();
    int chartSamplSliderCount();

    Q_PROPERTY(int chartResolution READ chartResolution WRITE setChartResolution NOTIFY chartSetupChanged)
    Q_PROPERTY(int chartResolutionSlider READ chartResolutionSlider WRITE setChartResolutionSlider NOTIFY chartSetupChanged)
    Q_PROPERTY(int chartResolutionSliderCount READ chartResolutionSliderCount NOTIFY sliderChanged)
    void setChartResolutionSlider(int pos);
    int chartResolutionSlider();
    int chartResolutionSliderCount();

    Q_PROPERTY(int datasetDist READ datasetDist WRITE setDatasetDist NOTIFY datasetChanged)
    Q_PROPERTY(int datasetChart READ datasetChart WRITE setDatasetChart NOTIFY datasetChanged)
    Q_PROPERTY(int datasetTemp READ datasetTemp WRITE setDatasetTemp NOTIFY datasetChanged)
    Q_PROPERTY(int datasetSDDBT READ datasetSDDBT WRITE setDatasetSDDBT NOTIFY datasetChanged)

    Q_PROPERTY(int ch1Period READ ch1Period WRITE setCh1Period NOTIFY datasetChanged)
    Q_PROPERTY(int ch1PeriodSlider READ ch1PeriodSlider WRITE setCh1PeriodSlider NOTIFY datasetChanged)
    Q_PROPERTY(int ch1PeriodSliderCount READ ch1PeriodSliderCount NOTIFY sliderChanged)
    void setCh1PeriodSlider(int pos);
    int ch1PeriodSlider();
    int ch1PeriodSliderCount();

    Q_PROPERTY(int ch2Period READ ch2Period WRITE setCh2Period NOTIFY datasetChanged)
    Q_PROPERTY(int ch2PeriodSlider READ ch2PeriodSlider WRITE setCh2PeriodSlider NOTIFY datasetChanged)
    Q_PROPERTY(int ch2PeriodSliderCount READ ch2PeriodSliderCount NOTIFY sliderChanged)
    void setCh2PeriodSlider(int pos);
    int ch2PeriodSlider();
    int ch2PeriodSliderCount();


    Q_PROPERTY(int transFreq READ transFreq WRITE setTransFreq NOTIFY transChanged)
    Q_PROPERTY(int transFreqSlider READ transFreqSlider WRITE setTransFreqSlider NOTIFY transChanged)
    Q_PROPERTY(int transFreqSliderCount READ transFreqSliderCount NOTIFY sliderChanged)
    void setTransFreqSlider(int pos);
    int transFreqSlider();
    int transFreqSliderCount();


    Q_PROPERTY(int transPulse READ transPulse WRITE setTransPulse NOTIFY transChanged)
    Q_PROPERTY(int transPulseSlider READ transPulseSlider WRITE setTransPulseSlider NOTIFY transChanged)
    Q_PROPERTY(int transPulseSliderCount READ transPulseSliderCount NOTIFY sliderChanged)
    void setTransPulseSlider(int pos);
    int transPulseSlider();
    int transPulseSliderCount();

    Q_PROPERTY(int transBoost READ transBoost WRITE setTransBoost NOTIFY transChanged)
    Q_PROPERTY(int transBoostSlider READ transBoostSlider WRITE setTransBoostSlider NOTIFY transChanged)
    Q_PROPERTY(int transBoostSliderCount READ transBoostSliderCount NOTIFY sliderChanged)
    void setTransBoostSlider(int pos);
    int transBoostSlider();
    int transBoostSliderCount();


    Q_PROPERTY(int upgradeFWStatus READ upgradeFWStatus NOTIFY upgradeProgressChanged)

signals:
    void sliderChanged();

public slots:

private:
    QVector<int> chartSampleArray = QVector<int>({100, 200, 300, 500, 700, 1000, 1500});
    QVector<int> chartResolutionArray = QVector<int>({10, 20, 30, 40, 50, 60, 70, 80, 90, 100});
    QVector<int> chPeriodArray = QVector<int>({0, 10, 20, 50, 100, 200, 500, 1000, 2000});
    QVector<int> freqArray = QVector<int>({200, 300, 400, 500, 600, 700, 800});
    QVector<int> pulseArray = QVector<int>({0, 5, 10, 15, 20, 30, 40, 50});
    QVector<int> boostArray = QVector<int>({0, 1});

    int sliderPos(QVector<int> data, int value);
    int valuePos(QVector<int> data, int pos);
};



#endif // SONARDRIVERINTERFACE_H
