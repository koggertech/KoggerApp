import QtQuick 2.12
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.12
import QtQuick.Dialogs 1.2
import Qt.labs.settings 1.1


DevSettingsBox {
    id: control
    isActive: dev.isChartSupport

    MenuBlock {
    }

    ColumnLayout {
        id: columnItem
        spacing: 24
        Layout.margins: 24

//        TitleMenuBox {
//            titleText: "Chart"
//            Layout.fillWidth: true

//            RowLayout {
//                id: switchDatasetChart
//                property int lastChannel: 1

//                CCheck {
//                    id:switch_ch1
//                    text: "On"
//                    checked: dev.datasetChart > 0
//                    onCheckedChanged: {
//                        if(checked == true && dev.datasetChart === 0) {
//                            dev.datasetChart = switchDatasetChart.lastChannel
//                        } else if(checked == false && dev.datasetChart > 0) {
//                            switchDatasetChart.lastChannel = dev.datasetChart
//                            dev.datasetChart = 0
//                        }
//                    }
//                }

//                CButton {
//                    text: "Shot"
//                    Layout.preferredWidth: 48
//                    Layout.preferredHeight: 22
//                    Layout.leftMargin: 10

//                    onClicked: {
//                        dev.requestChart();
//                    }
//                }
//            }
//        }




        ParamGroup {
            groupName: "Echogram"

            ParamSetup {
                paramName: "Resolution, mm"

                SpinBoxCustom {
                    from: 10
                    to: 100
                    stepSize: 10
                    value: dev.chartResolution
                    onValueChanged: {
                        dev.chartResolution = value
                    }
                }
            }

            ParamSetup {
                paramName: "Number of Samples"

                SpinBoxCustom {
                    from: 100
                    to: 5000
                    stepSize: 100
                    value: dev.chartSamples
                    onValueChanged: dev.chartSamples = value
                }
            }

            ParamSetup {
                paramName: "Offset of Samples"

                SpinBoxCustom {
                    from: 0; to: 5000; stepSize: 100
                    value: dev.chartOffset
                    onValueChanged: dev.chartOffset = value
                }
            }
        }

        ParamGroup {
            groupName: "Rangefinder"

            ParamSetup {
                paramName: "Max distance, mm"

                SpinBoxCustom {
                    from: 0; to: 50000; stepSize: 1000
                    value: dev.distMax
                    onValueChanged: dev.distMax = value
                }
            }

            ParamSetup {
                paramName: "Dead zone, mm"

                SpinBoxCustom {
                    from: 0; to: 50000; stepSize: 100
                    value: dev.distDeadZone
                    onValueChanged: dev.distDeadZone = value
                }
            }

            ParamSetup {
                paramName: "Confidence threshold, %"

                SpinBoxCustom {
                    from: 0; to: 100; stepSize: 1
                    value: dev.distConfidence
                    onValueChanged: dev.distConfidence = value
                }
            }
        }

        ParamGroup {
            groupName: "Transducer"

            ParamSetup {
                paramName: "Pulse count"

                SpinBoxCustom {
                    from: 0; to: 30; stepSize: 1
                    value: dev.transPulse
                    onValueChanged: dev.transPulse = value
                }
            }

            ParamSetup {
                paramName: "Frequency, kHz"

                SpinBoxCustom {
                    from: 100; to: 6000; stepSize: 5
                    value: dev.transFreq
                    onValueChanged:  dev.transFreq = value
                }
            }

            ParamSetup {
                paramName: "Booster"

                SpinBoxCustom {
                    from: 0; to: 1; stepSize: 1
                    value: dev.transBoost
                    onValueChanged: dev.transBoost = value

                    property var items: ["Off", "On"]

                    validator: RegExpValidator {
                        regExp: new RegExp("(Off|On)", "i")
                    }

                    textFromValue: function(value) {
                        return items[value];
                    }

                    valueFromText: function(text) {
                        for (var i = 0; i < items.length; ++i) {
                            if (items[i].toLowerCase().indexOf(text.toLowerCase()) === 0)
                                return i
                        }
                        return sb.value
                    }
                }
            }
        }

        ParamGroup {
            groupName: "DSP"

            ParamSetup {
                paramName: "Horizontal smoothing factor"

                SpinBoxCustom {
                    from: 0
                    to: 4
                    stepSize: 1
                    value: dev.dspHorSmooth
                    onValueChanged: dev.dspHorSmooth = value
                }
            }

            ParamSetup {
                paramName: "Speed of Sound, m/s"

                SpinBoxCustom {
                    from: 300
                    to: 6000
                    stepSize: 5
                    value: dev.soundSpeed/1000
                    onValueChanged: dev.soundSpeed = value*1000
                }
            }
        }


        ParamGroup {
            groupName: "Dataset"

            ParamSetup {
                paramName: ""
                Layout.alignment: Qt.AlignHCenter

                CText {
                    Layout.preferredWidth: 150
                    horizontalAlignment: Text.AlignHCenter
                    small: true

                    text: "Group #1"
                }

                CText {
                    Layout.preferredWidth: 150
                    horizontalAlignment: Text.AlignHCenter
                    small: true

                    text: "Group #2"
                }
            }

            ParamSetup {
                paramName: "Period, ms"

                SpinBoxCustom {
                    from: 0; to: 2000; stepSize: 50
                    value: dev.ch1Period
                    onValueChanged: dev.ch1Period = value
                }

                SpinBoxCustom {
                    from: 0; to: 2000; stepSize: 50
                    value: dev.ch2Period
                    onValueChanged: dev.ch2Period = value
                }
            }

            ParamSetup {
                paramName: "Echogram"

                SpinBoxCustom {
                    editable: false
                    from: 0; to: 1; stepSize: 1

                    value: dev.datasetChart === 1
                    onValueChanged: {
                        if(value == 1) { dev.datasetChart = 1
                        } else { dev.datasetChart = 0 }
                    }

                    property var items: ["Off", "8-bit", "16-bit"]
                    textFromValue: function(value) {
                        return items[value];
                    }
                }

                SpinBoxCustom {
                    editable: false
                    from: 0; to: 2; stepSize: 1
                    value: dev.datasetChart === 2
                    enabled: false
                    opacity: 0.5

                    onValueChanged: {
                        if(value == 1) { dev.datasetChart = 2
                        } else { dev.datasetChart = 0 }
                    }

                    property var items: ["Off", "8-bit", "16-bit"]
                    textFromValue: function(value) {
                        return items[value];
                    }
                }
            }

            ParamSetup {
                paramName: "Rangefinder"

                SpinBoxCustom {
                    from: 0; to: 2; stepSize: 1
                    value: dev.datasetDist === 1 ? 1 : dev.datasetSDDBT === 1 ? 2 : 0
                    editable: false

                    onValueChanged: {
                        console.debug(value)
                        if(value == 1) {
                            dev.datasetDist = 1
                        } else if(value == 2) {
                            dev.datasetSDDBT = 1
                        } else { dev.datasetDist = 0
                            dev.datasetSDDBT = 0}
                    }

                    property var items: ["Off", "On", "NMEA"]
                    textFromValue: function(value) {
                        return items[value];
                    }
                }

                SpinBoxCustom {
                    from: 0; to: 2; stepSize: 1
                    value: dev.datasetDist === 2 ? 1 : dev.datasetSDDBT === 2 ? 2 : 0
                    editable: false
                    enabled: false
                    opacity: 0.5

                    onValueChanged: {
                        console.debug(value)
                        if(value == 1) {
                            dev.datasetDist = 2
                        } else if(value == 2) {
                            dev.datasetSDDBT = 2
                        } else { dev.datasetDist = 0
                            dev.datasetSDDBT = 0}
                    }

                    property var items: ["Off", "On", "NMEA"]
                    textFromValue: function(value) {
                        return items[value];
                    }
                }
            }

            ParamSetup {
                paramName: "AHRS"

                SpinBoxCustom {
                    from: 0; to: 1; stepSize: 1
                    editable: false

                    value: (dev.datasetEuler & 1) === 1
                    onValueChanged: {
                        if(value == 1) { dev.datasetEuler = 1
                        } else if(dev.datasetEuler & 1) { dev.datasetEuler = 0 }
                    }

                    property var items: ["Off", "Euler", "Quat."]
                    textFromValue: function(value) {
                        return items[value];
                    }
                }

                SpinBoxCustom {
                    from: 0; to: 1; stepSize: 1
                    editable: false

                    value: (dev.datasetEuler & 2) === 2
                    onValueChanged: {
                        if(value == 1) { dev.datasetEuler = 2
                        } else if(dev.datasetEuler & 2) { dev.datasetEuler = 0 }
                    }

                    property var items: ["Off", "Euler", "Quat."]
                    textFromValue: function(value) {
                        return items[value];
                    }
                }
            }

            ParamSetup {
                paramName: "Temperature"

                SpinBoxCustom {
                    from: 0; to: 1; stepSize: 1
                    editable: false

                    value: (dev.datasetTemp & 1) === 1
                    onValueChanged: {
                        if(value == 1) { dev.datasetTemp = 1
                        } else if(dev.datasetTemp & 1) { dev.datasetTemp = 0 }
                    }

                    property var items: ["Off", "On"]
                    textFromValue: function(value) {
                        return items[value];
                    }
                }

                SpinBoxCustom {
                    from: 0; to: 1; stepSize: 1
                    editable: false

                    value: (dev.datasetTemp & 2) === 2
                    onValueChanged: {
                        if(value == 1) { dev.datasetTemp = 2
                        } else if(dev.datasetTemp & 2) { dev.datasetTemp = 0 }
                    }

                    property var items: ["Off", "On"]
                    textFromValue: function(value) {
                        return items[value];
                    }
                }
            }

            ParamSetup {
                paramName: "Timestamp"

                SpinBoxCustom {
                    from: 0; to: 1; stepSize: 1
                    editable: false

                    value: (dev.datasetTimestamp & 1) === 1
                    onValueChanged: {
                        if(value == 1) { dev.datasetTimestamp = 1
                        } else if(dev.datasetTimestamp & 1) { dev.datasetTimestamp = 0 }
                    }

                    property var items: ["Off", "On"]
                    textFromValue: function(value) {
                        return items[value];
                    }
                }

                SpinBoxCustom {
                    from: 0; to: 1; stepSize: 1
                    editable: false

                    value: (dev.datasetTimestamp & 2) === 2
                    onValueChanged: {
                        if(value == 1) { dev.datasetTimestamp = 2
                        } else if(dev.datasetTimestamp & 2) { dev.datasetTimestamp = 0 }
                    }

                    property var items: ["Off", "On"]
                    textFromValue: function(value) {
                        return items[value];
                    }
                }
            }
        }

        ParamGroup {
            groupName: "Actions"

//            ParamSetup {
//                paramName: "Upgrade"

//                FileDialog {
//                    id: fileDialog
//                    title: "Please choose a file"
//                    folder: shortcuts.home
//                    nameFilters: ["Upgrade files (*.ufw)"]
//                    onAccepted: {
//                        pathText.text = fileDialog.fileUrl.toString()
//                    }
//                    onRejected: {
//                    }
//                }

//                Settings {
//                    property alias upgradeFolder: fileDialog.folder
//                }

//                RowLayout {
//                    Layout.fillWidth: true

//                    TextField {
//                        id: pathText
//                        hoverEnabled: true
//                        Layout.fillWidth: true
//                        height: 22
//                        implicitHeight: 22

//                        color: "#F0F0F0"
//                        padding: 0

//                        text: ""
//                        font.pixelSize: 16
//                        placeholderText: qsTr("Enter path")

//                        background: Rectangle {
//                            color: "#252525"
//                        }
//                    }

//                    CButton {
//                        text: "..."
//                        Layout.fillWidth: false
//                        implicitHeight: 22
//                        implicitWidth: 22
//                        font.pixelSize: 16
//                        onClicked: {
//                            fileDialog.open()
//                        }
//                    }

//                    CButton {
//                        text: "UPGRADE"
//                        Layout.fillWidth: false
//                        Layout.leftMargin: 10
//                        implicitHeight: 22
//                        font.pixelSize: 16
//                        enabled: pathText.text != ""

//                        onClicked: {
//                            core.upgradeFW(pathText.text, dev)
//                        }
//                    }
//                }
//            }


            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                CButton {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 100
                    text: "Flash settings"

                    onClicked: {
                        dev.flashSettings()
                    }
                }

                CButton {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 100
                    text: "Erase settings"

                    onClicked: {
                        dev.resetSettings()
                    }
                }

                CButton {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 60
                    text: "Reboot"

                    onClicked: {
                        dev.reboot()
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                CCombo  {
                    id: baudrateCombo
                    Layout.fillWidth: true
                    model: [9600, 18200, 38400, 57600, 115200, 230400, 460800, 921600]
                    currentIndex: 4
                }

                CButton {
                    text: "Set baudrate"

                    onClicked: {
                        dev.baudrate = Number(baudrateCombo.currentText)
                    }
                }
            }
        }

    }
}
