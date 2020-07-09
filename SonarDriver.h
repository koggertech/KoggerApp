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

    typedef enum {
        DatasetOff = 0,
        DatasetCh1 = 1,
        DatasetCh2 = 2,
        DatasetRequest = 255
    } DatasetChannel;

    int distMax();
    void setDistMax(int dist);

    int distDeadZone();
    void setDistDeadZone(int dead_zone);

    int chartSamples();
    void setChartSamples(int samples);

    int chartResolution();
    void setChartResolution(int resol);

    int chartOffset();
    void setChartOffset(int offset);

    int datasetDist();
    void setDatasetDist(int ch_param);
    int datasetChart();
    void setDatasetChart(int ch_param);
    int datasetTemp();
    void setDatasetTemp(int ch_param);
    int datasetSDDBT();
    void setDatasetSDDBT(int ch_param);
    int datasetSDDBT_P2();
    void setDatasetSDDBT_P2(int ch_param);

    int ch1Period();
    void setCh1Period(int period);

    int ch2Period();
    void setCh2Period(int period);

    void sendUpdateFW(QByteArray update_data);
    bool isUpdatingFw() { return m_bootloader; }
    int upgradeFWStatus() {return m_upgrade_status; }


    int transFreq();
    void setTransFreq(int freq);

    int transPulse();
    void setTransPulse(int pulse);

    int transBoost();
    void setTransBoost(int boost);

    int soundSpeed();
    void setSoundSpeed(int speed);

signals:
    void dataSend(QByteArray data);
    void chartComplete(QVector<int16_t> data, int resolution, int offset);
    void chartSetupChanged();
    void distSetupChanged();
    void datasetChanged();
    void transChanged();
    void soundChanged();
    void upgradeProgressChanged();

public slots:
    void putData(const QByteArray &data);
    void protoComplete(ProtIn &proto);
    void startConnection();

    void requestDist();
    void requestChart();

    void flashSettings();
    void resetSettings();
    void reboot();

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

    bool m_inited = false;
    bool m_bootloader = false;
    int m_upgrade_status = 0;

    void regID(IDBin* id_bin, void (SonarDriver::* method)(Type type, Version ver, Resp resp), bool is_setup = false);

    void requestSetup();

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
