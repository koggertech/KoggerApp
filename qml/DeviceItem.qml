import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import QtQuick.Dialogs


ColumnLayout {
    id: columnItem
    spacing: 0
    Layout.margins: 0
    property var dev: null

    ParamGroup {
        groupName: qsTr("Echogram")

        ParamSetup {
            paramName: qsTr("Resolution, mm")

            SpinBoxCustom {
                from: 10
                to: 100
                stepSize: 10
                value: 0
                devValue: dev !== null ? dev.chartResolution : 0
                isValid: dev !== null ? dev.chartSetupState : false

                onValueChanged: {
                    if (!isDriverChanged) {
                        dev.chartResolution = value
                    }
                    isDriverChanged = false
                }
            }
        }

        ParamSetup {
            paramName: qsTr("Number of Samples")

            SpinBoxCustom {
                from: 100
                to: 15000
                stepSize: 100
                value: 0
                devValue: dev !== null ? dev.chartSamples : 0
                isValid: dev !== null ? dev.chartSetupState : false

                onValueChanged: {
                    if (!isDriverChanged) {
                        dev.chartSamples = value
                    }
                    isDriverChanged = false
                }
            }
        }

        ParamSetup {
            paramName: qsTr("Offset of Samples")

            SpinBoxCustom {
                from: 0
                to: 10000
                stepSize: 100
                value:0
                devValue: dev !== null ? dev.chartOffset : 0
                isValid: dev !== null ? dev.chartSetupState : false

                onValueChanged: {
                    if (!isDriverChanged) {
                        dev.chartOffset = value
                    }
                    isDriverChanged = false
                }
            }
        }
    }

    ParamGroup {
        groupName: qsTr("Rangefinder")

        ParamSetup {
            paramName: qsTr("Max distance, mm")

            SpinBoxCustom {
                from: 0;
                to: 50000;
                stepSize: 1000
                value: 0
                devValue: dev !== null ? dev.distMax : 0
                isValid: dev !== null ? dev.distSetupState : false

                onValueChanged: {
                    if (!isDriverChanged) {
                        dev.distMax = value
                    }
                    isDriverChanged = false
                }
            }
        }

        ParamSetup {
            paramName: qsTr("Dead zone, mm")

            SpinBoxCustom {
                from: 0
                to: 50000
                stepSize: 100
                value: 0
                devValue: dev !== null ? dev.distDeadZone : 0
                isValid: dev !== null ? dev.distSetupState : false

                onValueChanged: {
                    if (!isDriverChanged) {
                        dev.distDeadZone = value
                    }
                    isDriverChanged = false
                }
            }
        }

        ParamSetup {
            paramName: qsTr("Confidence threshold, %")

            SpinBoxCustom {
                from: 0
                to: 100
                stepSize: 1
                value: 0
                devValue: dev !== null ? dev.distConfidence : 0
                isValid: dev !== null ? dev.distSetupState : false

                onValueChanged: {
                    if (!isDriverChanged) {
                        dev.distConfidence = value
                    }
                    isDriverChanged = false
                }
            }
        }
    }

    ParamGroup {
        groupName: qsTr("Transducer")

        ParamSetup {
            paramName: qsTr("Pulse count")

            SpinBoxCustom {
                from: 0
                to: 5000
                stepSize: 1
                value: 0
                devValue: dev !== null ? dev.transPulse : 0
                isValid: dev !== null ? dev.transcState : false

                onValueChanged: {
                    if (!isDriverChanged) {
                        dev.transPulse = value
                    }
                    isDriverChanged = false
                }
            }
        }

        ParamSetup {
            paramName: qsTr("Frequency, kHz")

            SpinBoxCustom {
                from: 40
                to: 6000
                stepSize: 5
                value: 0
                devValue: dev !== null ? dev.transFreq : 0
                isValid: dev !== null ? dev.transcState : false

                onValueChanged: {
                    if (!isDriverChanged) {
                        dev.transFreq = value
                    }
                    isDriverChanged = false
                }
            }
        }

        ParamSetup {
            paramName: qsTr("Booster")

            SpinBoxCustom {
                id: spinBoxBooster

                from: 0
                to: 1
                stepSize: 1
                value: 0
                devValue: dev !== null ? dev.transBoost : 0
                isValid: dev !== null ? dev.transcState : false

                onValueChanged: {
                    if (!isDriverChanged) {
                        dev.transBoost = value
                    }
                    isDriverChanged = false
                }

                property var items: [qsTr("Off"), qsTr("On")]
                property string regExpPattern: "(" + items.join("|") + ")"

                validator: RegularExpressionValidator {
                    regularExpression: new RegExp(spinBoxBooster ? spinBoxBooster.regExpPattern : "(Off|On)", "i")
                }

                textFromValue: function(value) {
                    return items[value];
                }

                valueFromText: function(text) {
                    for (var i = 0; i < items.length; ++i) {
                        if (items[i].toLowerCase().indexOf(text.toLowerCase()) === 0) {
                            return i
                        }
                    }
                    return spinBoxBooster.value
                }
            }
        }
    }

    ParamGroup {
        groupName: qsTr("DSP")

        ParamSetup {
            paramName: qsTr("Horizontal smoothing factor")

            SpinBoxCustom {
                from: 0
                to: 4
                stepSize: 1
                value: 0
                devValue: dev !== null ? dev.dspHorSmooth : 0
                isValid: dev !== null ? dev.dspState : false

                onValueChanged: {
                    if (!isDriverChanged) {
                        dev.dspHorSmooth = value
                    }
                    isDriverChanged = false
                }
            }
        }

        ParamSetup {
            paramName: qsTr("Speed of Sound, m/s")

            SpinBoxCustom {
                from: 300
                to: 6000
                stepSize: 5
                value: 0
                devValue: dev !== null ? dev.soundSpeed / 1000 : 0
                isValid: dev !== null ? dev.soundState : false

                onValueChanged: {
                    if (!isDriverChanged) {
                        dev.soundSpeed = value * 1000
                    }
                    isDriverChanged = false
                }
            }
        }
    }

    ParamGroup {
        groupName: qsTr("Dataset")

        ParamSetup {
            paramName: qsTr("Period, ms")

            SpinBoxCustom {
                from: 0
                to: 2000
                stepSize: 50
                value: 0
                devValue: dev !== null ? dev.ch1Period : 0
                isValid: dev !== null ? dev.datasetState : false

                onValueChanged: {
                    if (!isDriverChanged) {
                        dev.ch1Period = value
                    }
                    isDriverChanged = false
                }
            }
        }

        ParamSetup {
            paramName: qsTr("Echogram")

            SpinBoxCustom {
                from: 0
                to: 1
                stepSize: 1
                value: 0
                devValue: dev !== null ? dev.datasetChart === 1 : 0
                isValid: dev !== null ? dev.datasetState : false
                editable: false

                onValueChanged: {
                    if (!isDriverChanged) {
                        if (value == 1) {
                            dev.datasetChart = 1
                        }
                        else {
                            dev.datasetChart = 0
                        }
                    }
                    isDriverChanged = false
                }

                property var items: [qsTr("Off"), qsTr("8-bit"), qsTr("16-bit")]
                textFromValue: function(value) {
                    return items[value];
                }
            }
        }

        ParamSetup {
            paramName: qsTr("Rangefinder")

            SpinBoxCustom {
                from: 0
                to: 2
                stepSize: 1
                value: 0
                devValue: dev !== null ? (dev.datasetDist === 1 ? 1 : dev.datasetSDDBT === 1 ? 2 : 0) : 0
                isValid: dev !== null ? dev.datasetState : false
                editable: false

                onValueChanged: {
                    if (!isDriverChanged) {
                        if (value == 1) {
                            dev.datasetDist = 1
                        }
                        else if (value == 2) {
                            dev.datasetSDDBT = 1
                        }
                        else {
                            dev.datasetDist = 0
                            dev.datasetSDDBT = 0
                        }
                    }
                    isDriverChanged = false
                }

                property var items: [qsTr("Off"), qsTr("On"), qsTr("NMEA")]
                textFromValue: function(value) {
                    return items[value];
                }
            }
        }

        ParamSetup {
            paramName: qsTr("AHRS")

            SpinBoxCustom {
                from: 0
                to: 1
                stepSize: 1
                editable: false
                value: 0
                devValue: dev !== null ? ((dev.datasetEuler & 1) === 1) : 0
                isValid: dev !== null ? dev.datasetState : false

                onValueChanged: {
                    if (!isDriverChanged) {
                        if (value == 1) {
                            dev.datasetEuler = 1
                        }
                        else if (dev.datasetEuler & 1) {
                            dev.datasetEuler = 0
                        }
                    }
                    isDriverChanged = false
                }

                property var items: [qsTr("Off"), qsTr("Euler"), qsTr("Quat.")]
                textFromValue: function(value) {
                    return items[value];
                }
            }
        }

        ParamSetup {
            paramName: qsTr("Temperature")

            SpinBoxCustom {
                from: 0
                to: 1
                stepSize: 1
                value: 0
                devValue: dev !== null ? ((dev.datasetTemp & 1) === 1) : 0
                isValid: dev !== null ? dev.datasetState : false
                editable: false

                onValueChanged: {
                    if (!isDriverChanged) {
                        if(value == 1) {
                            dev.datasetTemp = 1
                        }
                        else if (dev.datasetTemp & 1) {
                            dev.datasetTemp = 0
                        }
                    }
                    isDriverChanged = false
                }

                property var items: [qsTr("Off"), qsTr("On")]
                textFromValue: function(value) {
                    return items[value];
                }
            }
        }

        ParamSetup {
            paramName: qsTr("Timestamp")

            SpinBoxCustom {
                from: 0
                to: 1
                stepSize: 1
                value: 0
                devValue: dev !== null ? ((dev.datasetTimestamp & 1) === 1) : 0
                isValid: dev !== null ? dev.datasetState : false
                editable: false

                onValueChanged: {
                    if (!isDriverChanged) {
                        if (value == 1) {
                            dev.datasetTimestamp = 1
                        }
                        else if (dev.datasetTimestamp & 1) {
                            dev.datasetTimestamp = 0
                        }
                    }
                    isDriverChanged = false
                }

                property var items: [qsTr("Off"), qsTr("On")]
                textFromValue: function(value) {
                    return items[value];
                }
            }
        }
    }
}
