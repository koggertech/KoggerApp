#ifndef SONARDRIVER_H
#define SONARDRIVER_H

#include <QObject>
#include <ProtoBinnary.h>
#include <IDBinnary.h>
#include <QHash>
#include <QVector>

using namespace KoggerBinnaryProtocol;

class SonarDriver : public QObject
{
    Q_OBJECT
public:
    explicit SonarDriver(QObject *parent = nullptr);

    Q_PROPERTY(int chartSamples READ chartSamples WRITE setChartSamples NOTIFY chartSamplesChanged)
    Q_PROPERTY(double chartSamplSlider READ chartSamplesSlider WRITE setChartSamplesSlider NOTIFY chartSamplesSliderChanged)
    Q_PROPERTY(double chartSamplSliderStep READ chartSamplSliderStep NOTIFY sliderChanged)
    int chartSamples();
    void setChartSamples(int samples);
    void setChartSamplesSlider(double pos);
    double chartSamplesSlider();
    double chartSamplSliderStep();

    void sendUpdateFW(QByteArray update_data);

signals:
    void dataSend(QByteArray data);

    void chartComplete(QVector<uint8_t> data, int resolution, int offset);
    void chartSamplesChanged(int);
    void chartSamplesSliderChanged(double pos);
    void sliderChanged();

public slots:
    void putData(const QByteArray &data);
    void protoComplete(ProtIn &proto);

private:
    ProtIn* m_proto;

    IDBinTimestamp* idTimestamp;
    IDBinDist* idDist;
    IDBinChart* idChart;
    IDBinAttitude* idAtt;
    IDBinTemp* idTemp;

    IDBinDataset* idDataset;
    IDBinDistSetup* idDistSetup;
    IDBinChartSetup* idChartSetup;
    IDBinTransc* idTransc;
    IDBinSoundSpeed* idSoundSpeed;
    IDBinUART* idUART;

    IDBinMark* idMark;
    IDBinFlash* idFlash;
    IDBinBoot* idBoot;
    IDBinUpdate* idUpdate;

    IDBinNav* idNav;

    QHash<ID, IDBin*> hashIDParsing;
    QHash<ID, IDBin*> hashIDSetup;

    QByteArray updateData;

    void regID(IDBin* id_bin, void (SonarDriver::* method)(Type type, Version ver, Resp resp), bool is_setup = false);

    void requestSetup();

    int m_chartSamplesCnt = 100;

protected slots:
    void receivedTimestamp(Type type, Version ver, Resp resp);
    void receivedDist(Type type, Version ver, Resp resp);
    void receivedChart(Type type, Version ver, Resp resp);
    void receivedAtt(Type type, Version ver, Resp resp);
    void receivedTemp(Type type, Version ver, Resp resp);

    void receivedDataset(Type type, Version ver, Resp resp);
    void receivedDistSetup(Type type, Version ver, Resp resp);
    void receivedChartSetup(Type type, Version ver, Resp resp);
    void receivedTransc(Type type, Version ver, Resp resp);
    void receivedSoundSpeed(Type type, Version ver, Resp resp);
    void receivedUART(Type type, Version ver, Resp resp);

    void receivedMark(Type type, Version ver, Resp resp);
    void receivedFlash(Type type, Version ver, Resp resp);
    void receivedBoot(Type type, Version ver, Resp resp);
    void receivedUpdate(Type type, Version ver, Resp resp);

    void receivedNav(Type type, Version ver, Resp resp);
};

#endif // SONARDRIVER_H
