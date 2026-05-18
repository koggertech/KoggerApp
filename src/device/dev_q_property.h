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

    Q_PROPERTY(bool isServoSupport READ getServoControlState NOTIFY servoControlChanged)
    Q_PROPERTY(bool servoEnabled READ servoEnabled WRITE setServoEnabled NOTIFY servoControlChanged)
    Q_PROPERTY(bool servoReverse READ servoReverse WRITE setServoReverse NOTIFY servoControlChanged)
    Q_PROPERTY(int servoPwmMinUs READ servoPwmMinUs WRITE setServoPwmMinUs NOTIFY servoControlChanged)
    Q_PROPERTY(int servoPwmMaxUs READ servoPwmMaxUs WRITE setServoPwmMaxUs NOTIFY servoControlChanged)
    Q_PROPERTY(double servoAngleRangeDeg READ servoAngleRangeDeg WRITE setServoAngleRangeDeg NOTIFY servoControlChanged)
    Q_PROPERTY(double servoStepDeg READ servoStepDeg WRITE setServoStepDeg NOTIFY servoControlChanged)
    Q_PROPERTY(double servoRangeDeg READ servoRangeDeg WRITE setServoRangeDeg NOTIFY servoControlChanged)
    Q_PROPERTY(double servoCenterDeg READ servoCenterDeg WRITE setServoCenterDeg NOTIFY servoControlChanged)
    Q_PROPERTY(double servoCurrentAngleDeg READ servoCurrentAngleDeg NOTIFY servoCurrentAngleChanged)

    Q_PROPERTY(int pwmRouteOut1 READ pwmRouteOut1 WRITE setPwmRouteOut1 NOTIFY pwmRouteChanged)
    Q_PROPERTY(int pwmRouteOut2 READ pwmRouteOut2 WRITE setPwmRouteOut2 NOTIFY pwmRouteChanged)
    Q_PROPERTY(int pwmRouteOut3 READ pwmRouteOut3 WRITE setPwmRouteOut3 NOTIFY pwmRouteChanged)
#endif

    // ----- servo getters/setters (inline pass-through к IDBin*) -----
    bool servoEnabled() const { return idServoControl ? idServoControl->enabled() : false; }
    void setServoEnabled(bool v) { if (idServoControl) idServoControl->setEnabled(v); }

    bool servoReverse() const { return idServoControl ? idServoControl->reverse() : false; }
    void setServoReverse(bool v) { if (idServoControl) idServoControl->setReverse(v); }

    int servoPwmMinUs() const { return idServoControl ? idServoControl->pwmMinUs() : 0; }
    void setServoPwmMinUs(int v) { if (idServoControl) idServoControl->setPwmMinUs(static_cast<U2>(v)); }

    int servoPwmMaxUs() const { return idServoControl ? idServoControl->pwmMaxUs() : 0; }
    void setServoPwmMaxUs(int v) { if (idServoControl) idServoControl->setPwmMaxUs(static_cast<U2>(v)); }

    double servoAngleRangeDeg() const {
        return idServoControl ? double(idServoControl->angleRangeDeg()) / IDBinServoControl::AngleScale : 0.0;
    }
    void setServoAngleRangeDeg(double deg) {
        if (idServoControl) idServoControl->setAngleRangeDeg(degToS2(deg));
    }

    double servoStepDeg() const {
        return idServoControl ? double(idServoControl->stepDeg()) / IDBinServoControl::AngleScale : 0.0;
    }
    void setServoStepDeg(double deg) {
        if (idServoControl) idServoControl->setStepDeg(degToS2(deg));
    }

    double servoRangeDeg() const {
        return idServoControl ? double(idServoControl->rangeDeg()) / IDBinServoControl::AngleScale : 0.0;
    }
    void setServoRangeDeg(double deg) {
        if (idServoControl) idServoControl->setRangeDeg(degToS2(deg));
    }

    double servoCenterDeg() const {
        return idServoControl ? double(idServoControl->centerDeg()) / IDBinServoControl::AngleScale : 0.0;
    }
    void setServoCenterDeg(double deg) {
        if (idServoControl) idServoControl->setCenterDeg(degToS2(deg));
    }

    double servoCurrentAngleDeg() const { return idAtt ? idAtt->roll() : 0.0; }

    int pwmRouteOut1() const { return idPwmRoute ? idPwmRoute->target(0) : 0; }
    void setPwmRouteOut1(int v) { if (idPwmRoute) idPwmRoute->setTarget(0, static_cast<U1>(v)); }
    int pwmRouteOut2() const { return idPwmRoute ? idPwmRoute->target(1) : 0; }
    void setPwmRouteOut2(int v) { if (idPwmRoute) idPwmRoute->setTarget(1, static_cast<U1>(v)); }
    int pwmRouteOut3() const { return idPwmRoute ? idPwmRoute->target(2) : 0; }
    void setPwmRouteOut3(int v) { if (idPwmRoute) idPwmRoute->setTarget(2, static_cast<U1>(v)); }

private:
    static S2 degToS2(double deg) {
        double scaled = deg * IDBinServoControl::AngleScale;
        if (scaled >  32767.0) scaled =  32767.0;
        if (scaled < -32768.0) scaled = -32768.0;
        return static_cast<S2>(scaled);
    }
};



#endif // SONARDRIVERINTERFACE_H
