#ifndef SONARDRIVERINTERFACE_H
#define SONARDRIVERINTERFACE_H

#include "dev_driver.h"

class DevQProperty : public DevDriver
{
    Q_OBJECT
public:
    explicit DevQProperty(QObject *parent = nullptr) :
        DevDriver(parent){
    }

#ifndef SEPARATE_READING
    Q_PROPERTY(int distMax READ distMax WRITE setDistMax NOTIFY distSetupChanged)
    Q_PROPERTY(int distDeadZone READ distDeadZone WRITE setDistDeadZone NOTIFY distSetupChanged)
    Q_PROPERTY(int distConfidence READ distConfidence WRITE setConfidence NOTIFY distSetupChanged)
    Q_PROPERTY(bool distSetupState READ getDistSetupState NOTIFY distSetupChanged)

    Q_PROPERTY(int chartSamples READ chartSamples WRITE setChartSamples NOTIFY chartSetupChanged)
    Q_PROPERTY(int chartResolution READ chartResolution WRITE setChartResolution NOTIFY chartSetupChanged)
    Q_PROPERTY(int chartOffset READ chartOffset WRITE setChartOffset NOTIFY chartSetupChanged)
    Q_PROPERTY(bool chartSetupState READ getChartSetupState NOTIFY chartSetupChanged)

    Q_PROPERTY(int datasetTimestamp READ datasetTimestamp WRITE setDatasetTimestamp NOTIFY datasetChanged)
    Q_PROPERTY(int datasetDist READ datasetDist WRITE setDatasetDist NOTIFY datasetChanged)
    Q_PROPERTY(int datasetChart READ datasetChart WRITE setDatasetChart NOTIFY datasetChanged)
    Q_PROPERTY(int datasetTemp READ datasetTemp WRITE setDatasetTemp NOTIFY datasetChanged)
    Q_PROPERTY(int datasetEuler READ datasetEuler WRITE setDatasetEuler NOTIFY datasetChanged)
    Q_PROPERTY(int datasetSDDBT READ datasetSDDBT WRITE setDatasetSDDBT NOTIFY datasetChanged)
    Q_PROPERTY(int datasetSDDBT_P2 READ datasetSDDBT_P2 WRITE setDatasetSDDBT_P2 NOTIFY datasetChanged)
    Q_PROPERTY(int ch1Period READ ch1Period WRITE setCh1Period NOTIFY datasetChanged)
    Q_PROPERTY(int ch2Period READ ch2Period WRITE setCh2Period NOTIFY datasetChanged)
    Q_PROPERTY(bool datasetState READ getDatasetState NOTIFY datasetChanged)

    Q_PROPERTY(int transFreq READ transFreq WRITE setTransFreq NOTIFY transChanged)
    Q_PROPERTY(int transPulse READ transPulse WRITE setTransPulse NOTIFY transChanged)
    Q_PROPERTY(int transBoost READ transBoost WRITE setTransBoost NOTIFY transChanged)
    Q_PROPERTY(bool transcState READ getTranscState NOTIFY transChanged)

    Q_PROPERTY(int dspHorSmooth READ dspSmoothFactor WRITE setDspSmoothFactor NOTIFY dspSetupChanged)
    Q_PROPERTY(bool dspState READ getDspSetupState NOTIFY dspSetupChanged)

    Q_PROPERTY(int soundSpeed READ soundSpeed WRITE setSoundSpeed NOTIFY soundChanged)
    Q_PROPERTY(bool soundState READ getSoundSpeedState NOTIFY soundChanged)

    Q_PROPERTY(int busAddress READ getBusAddress WRITE setBusAddress NOTIFY UARTChanged)
    Q_PROPERTY(int baudrate READ getBaudrate WRITE setBaudrate NOTIFY UARTChanged)
    Q_PROPERTY(int devAddress READ getDevAddress WRITE setDevAddress NOTIFY UARTChanged)
    Q_PROPERTY(int devDefAddress READ getDevDefAddress WRITE setDevDefAddress NOTIFY UARTChanged)
    Q_PROPERTY(bool uartState READ getUartState NOTIFY UARTChanged)

    Q_PROPERTY(int upgradeFWStatus READ upgradeFWStatus NOTIFY upgradeChanged)

    Q_PROPERTY(QString devName READ devName NOTIFY deviceVersionChanged)
    Q_PROPERTY(int devType READ devType NOTIFY deviceVersionChanged)
    Q_PROPERTY(int devSN READ devSerialNumber NOTIFY deviceVersionChanged)
    Q_PROPERTY(QString fwVersion READ fwVersion NOTIFY deviceVersionChanged)

    Q_PROPERTY(bool isSonar READ isSonar NOTIFY deviceVersionChanged)
    Q_PROPERTY(bool isRecorder READ isRecorder NOTIFY deviceVersionChanged)
    Q_PROPERTY(bool isDoppler READ isDoppler NOTIFY deviceVersionChanged)
    Q_PROPERTY(bool isUSBLBeacon READ isUSBLBeacon NOTIFY deviceVersionChanged)
    Q_PROPERTY(bool isUSBL READ isUSBL NOTIFY deviceVersionChanged)
    Q_PROPERTY(bool isChartSupport READ isChartSupport NOTIFY deviceVersionChanged)
    Q_PROPERTY(bool isDistSupport READ isDistSupport NOTIFY deviceVersionChanged)
    Q_PROPERTY(bool isDSPSupport READ isDSPSupport NOTIFY deviceVersionChanged)
    Q_PROPERTY(bool isTransducerSupport READ isTransducerSupport NOTIFY deviceVersionChanged)
    Q_PROPERTY(bool isDatasetSupport READ isDatasetSupport NOTIFY deviceVersionChanged)
    Q_PROPERTY(bool isSoundSpeedSupport READ isSoundSpeedSupport NOTIFY deviceVersionChanged)
    Q_PROPERTY(bool isAddressSupport READ isAddressSupport NOTIFY deviceVersionChanged)
    Q_PROPERTY(bool isUpgradeSupport READ isUpgradeSupport NOTIFY deviceVersionChanged)
    Q_PROPERTY(QString modemLastPayload READ modemLastPayload NOTIFY modemSolutionChanged)
    Q_PROPERTY(bool recorderStatusAvailable READ recorderStatusAvailable NOTIFY recorderStatusChanged)
    Q_PROPERTY(int recorderDeviceCondition READ recorderDeviceCondition NOTIFY recorderStatusChanged)
    Q_PROPERTY(int recorderMode READ recorderMode NOTIFY recorderStatusChanged)
    Q_PROPERTY(int recorderState READ recorderState NOTIFY recorderStatusChanged)
    Q_PROPERTY(uint recorderStatusFlags READ recorderStatusFlags NOTIFY recorderStatusChanged)
    Q_PROPERTY(uint recorderWarningFlags READ recorderWarningFlags NOTIFY recorderStatusChanged)
    Q_PROPERTY(uint recorderDegradedFlags READ recorderDegradedFlags NOTIFY recorderStatusChanged)
    Q_PROPERTY(uint recorderCriticalFlags READ recorderCriticalFlags NOTIFY recorderStatusChanged)
    Q_PROPERTY(int recorderUptimeSeconds READ recorderUptimeSeconds NOTIFY recorderStatusChanged)
    Q_PROPERTY(int recorderCurrentLogId READ recorderCurrentLogId NOTIFY recorderStatusChanged)
    Q_PROPERTY(uint recorderRecordedSizeBytes READ recorderRecordedSizeBytes NOTIFY recorderStatusChanged)
    Q_PROPERTY(qulonglong recorderFreeSpaceBytes READ recorderFreeSpaceBytes NOTIFY recorderStatusChanged)
    Q_PROPERTY(int recorderRecordingDurationSeconds READ recorderRecordingDurationSeconds NOTIFY recorderStatusChanged)
    Q_PROPERTY(int recorderSecondsSinceLastWrite READ recorderSecondsSinceLastWrite NOTIFY recorderStatusChanged)
    Q_PROPERTY(QString recorderConditionText READ recorderConditionText NOTIFY recorderStatusChanged)
    Q_PROPERTY(QString recorderStateText READ recorderStateText NOTIFY recorderStatusChanged)
    Q_PROPERTY(QString recorderReasonText READ recorderReasonText NOTIFY recorderStatusChanged)
    Q_PROPERTY(QString recorderFreshnessText READ recorderFreshnessText NOTIFY recorderStatusChanged)
    Q_PROPERTY(QString recorderProgressText READ recorderProgressText NOTIFY recorderStatusChanged)
#endif

};



#endif // SONARDRIVERINTERFACE_H
