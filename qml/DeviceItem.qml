import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import QtQuick.Dialogs


ColumnLayout {
    id: columnItem
    spacing: 0
    Layout.margins: 0
    property var dev: null

    function safeNum(val, fallback) {
        return (val === undefined || val === null) ? fallback : val
    }

    function safeBool(val) {
        return val === true
    }

    ParamGroup {
        groupName: qsTr("Echogram")

        ParamSetup {
            paramName: qsTr("Resolution, mm")

            SpinBoxCustom {
                from: 10
                to: 100
                stepSize: 10
                value: 0
                devValue: safeNum(dev && dev.chartResolution, 0)
                isValid: safeBool(dev && dev.chartSetupState)

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
                devValue: safeNum(dev && dev.chartSamples, 0)
                isValid: safeBool(dev && dev.chartSetupState)

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
                devValue: safeNum(dev && dev.chartOffset, 0)
                isValid: safeBool(dev && dev.chartSetupState)

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
                devValue: safeNum(dev && dev.distMax, 0)
                isValid: safeBool(dev && dev.distSetupState)

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
                devValue: safeNum(dev && dev.distDeadZone, 0)
                isValid: safeBool(dev && dev.distSetupState)

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
                devValue: safeNum(dev && dev.distConfidence, 0)
                isValid: safeBool(dev && dev.distSetupState)

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
                devValue: safeNum(dev && dev.transPulse, 0)
                isValid: safeBool(dev && dev.transcState)

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
                devValue: safeNum(dev && dev.transFreq, 0)
                isValid: safeBool(dev && dev.transcState)

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
                devValue: safeNum(dev && dev.transBoost, 0)
                isValid: safeBool(dev && dev.transcState)

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
                devValue: safeNum(dev && dev.dspHorSmooth, 0)
                isValid: safeBool(dev && dev.dspState)

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
                devValue: safeNum(dev && dev.soundSpeed !== undefined ? dev.soundSpeed / 1000 : undefined, 0)
                isValid: safeBool(dev && dev.soundState)

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
                devValue: safeNum(dev && dev.ch1Period, 0)
                isValid: safeBool(dev && dev.datasetState)

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
                devValue: dev ? (dev.datasetChart === 1 ? 1 : 0) : 0
                isValid: safeBool(dev && dev.datasetState)
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
                devValue: dev ? (dev.datasetDist === 1 ? 1 : dev.datasetSDDBT === 1 ? 2 : 0) : 0
                isValid: safeBool(dev && dev.datasetState)
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
                devValue: dev ? ((dev.datasetEuler & 1) === 1 ? 1 : 0) : 0
                isValid: safeBool(dev && dev.datasetState)

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
                devValue: dev ? ((dev.datasetTemp & 1) === 1 ? 1 : 0) : 0
                isValid: safeBool(dev && dev.datasetState)
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
                devValue: dev ? ((dev.datasetTimestamp & 1) === 1 ? 1 : 0) : 0
                isValid: safeBool(dev && dev.datasetState)
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
