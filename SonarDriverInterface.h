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

    Q_PROPERTY(int distMax READ distMax WRITE setDistMax NOTIFY distSetupChanged)
    Q_PROPERTY(int distMaxSlider READ distMaxSlider WRITE setdistMaxSlider NOTIFY distSetupChanged)
    Q_PROPERTY(int distMaxSliderCount READ distMaxSliderCount NOTIFY sliderChanged)
    void setdistMaxSlider(int pos);
    int distMaxSlider();
    int distMaxSliderCount();

    Q_PROPERTY(int distDeadZone READ distDeadZone WRITE setDistDeadZone NOTIFY distSetupChanged)
    Q_PROPERTY(int distDeadZoneSlider READ distDeadZoneSlider WRITE setdistDeadZoneSlider NOTIFY distSetupChanged)
    Q_PROPERTY(int distDeadZoneSliderCount READ distDeadZoneSliderCount NOTIFY sliderChanged)
    void setdistDeadZoneSlider(int pos);
    int distDeadZoneSlider();
    int distDeadZoneSliderCount();

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

    Q_PROPERTY(int chartOffset READ chartOffset WRITE setChartOffset NOTIFY chartSetupChanged)
    Q_PROPERTY(int chartOffsetSlider READ chartOffsetSlider WRITE setChartOffsetSlider NOTIFY chartSetupChanged)
    Q_PROPERTY(int chartOffsetSliderCount READ chartOffsetSliderCount NOTIFY sliderChanged)
    void setChartOffsetSlider(int pos);
    int chartOffsetSlider();
    int chartOffsetSliderCount();



    Q_PROPERTY(int datasetDist READ datasetDist WRITE setDatasetDist NOTIFY datasetChanged)
    Q_PROPERTY(int datasetChart READ datasetChart WRITE setDatasetChart NOTIFY datasetChanged)
    Q_PROPERTY(int datasetTemp READ datasetTemp WRITE setDatasetTemp NOTIFY datasetChanged)
    Q_PROPERTY(int datasetSDDBT READ datasetSDDBT WRITE setDatasetSDDBT NOTIFY datasetChanged)
    Q_PROPERTY(int datasetSDDBT_P2 READ datasetSDDBT_P2 WRITE setDatasetSDDBT_P2 NOTIFY datasetChanged)

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

    Q_PROPERTY(int soundSpeed READ soundSpeed WRITE setSoundSpeed NOTIFY soundChanged)


    Q_PROPERTY(int upgradeFWStatus READ upgradeFWStatus NOTIFY upgradeProgressChanged)

signals:
    void sliderChanged();

public slots:

private:
    QVector<int> chartSampleArray = QVector<int>({100, 200, 300, 500, 700, 1000, 1500});
    QVector<int> chartResolutionArray = QVector<int>({10, 20, 30, 40, 50, 60, 70, 80, 90, 100});
    QVector<int> chartOffsetArray = QVector<int>({0, 100, 200, 300, 500, 800, 1000});
    QVector<int> distMaxArray = QVector<int>({2000, 5000, 10000, 15000, 20000, 30000, 40000, 50000});
    QVector<int> distDeadZoneArray = QVector<int>({100, 150, 200, 250, 300, 350, 400, 450, 500});
    QVector<int> chPeriodArray = QVector<int>({0, 10, 20, 50, 100, 200, 500, 1000, 2000});
    QVector<int> freqArray = QVector<int>({90, 100, 110, 120, 130, 140, 150, 160, 170, 180, 190, 200, 210, 220, 230, 240, 250, 260, 270, 280, 290, 300, 310, 320, 330, 340, 350, 360, 370, 380, 390, 400, 410, 420, 430, 440, 450, 460, 470, 480, 490, 500, 510, 520, 530, 540, 550, 560, 570, 580, 590, 600, 610, 620, 630, 640, 650, 660, 670, 680, 690, 700, 710, 720, 730, 740, 750, 760, 770, 780, 790, 800});
    QVector<int> pulseArray = QVector<int>({0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30});
    QVector<int> boostArray = QVector<int>({0, 1});

    int sliderPos(QVector<int> data, int value);
    int valuePos(QVector<int> data, int pos, int value);
};



#endif // SONARDRIVERINTERFACE_H
