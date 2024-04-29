import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import QtQuick.Dialogs 1.2
import Qt.labs.settings 1.1

DevSettingsBox {
    id: control
    isActive: dev !== null ? dev.isChartSupport : false

    MenuBlock {
    }

    ColumnLayout {
        id: columnItem
        spacing: 24
        Layout.margins: 24

        ParamGroup {
            groupName: "Echogram"

            ParamSetup {
                paramName: "Resolution, mm"

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
                paramName: "Number of Samples"

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
                paramName: "Offset of Samples"

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
            groupName: "Rangefinder"

            ParamSetup {
                paramName: "Max distance, mm"

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
                paramName: "Dead zone, mm"

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
                paramName: "Confidence threshold, %"

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
            groupName: "Transducer"

            ParamSetup {
                paramName: "Pulse count"

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
                paramName: "Frequency, kHz"

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
                paramName: "Booster"

                SpinBoxCustom {
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
                paramName: "Speed of Sound, m/s"

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
            groupName: "Dataset"

            ParamSetup {
                paramName: "Period, ms"

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
                paramName: "Echogram"

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

                    property var items: ["Off", "8-bit", "16-bit"]
                    textFromValue: function(value) {
                        return items[value];
                    }
                }
            }

            ParamSetup {
                paramName: "Rangefinder"

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

                    property var items: ["Off", "On", "NMEA"]
                    textFromValue: function(value) {
                        return items[value];
                    }
                }
            }

            ParamSetup {
                paramName: "AHRS"

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

                    property var items: ["Off", "Euler", "Quat."]
                    textFromValue: function(value) {
                        return items[value];
                    }
                }
            }

            ParamSetup {
                paramName: "Temperature"

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

                    property var items: ["Off", "On"]
                    textFromValue: function(value) {
                        return items[value];
                    }
                }
            }

            ParamSetup {
                paramName: "Timestamp"

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
                    model: [9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600, 1200000, 2000000]
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

        ParamGroup {
            groupName: "Settings"

            FileDialog {
                id: importFileDialog
                title: "Open file"
                selectExisting: true
                nameFilters: ["XML files (*.xml)"]

                onAccepted: {
                    var selectedFile = importFileDialog.fileUrl
                    if (selectedFile !== "") {
                        var filePath = selectedFile.toString();
                        if (Qt.platform.os === "windows")
                            filePath = filePath.substring(8)
                        dev.importSettingsFromXML(filePath)
                    }
                }
            }

            FileDialog {
                id: exportFileDialog
                title: "Save as file"
                selectExisting: false
                nameFilters: ["XML files (*.xml)"]

                onAccepted: {
                    var selectedFile = exportFileDialog.fileUrl
                    if (selectedFile !== "") {
                        var filePath = selectedFile.toString();
                        if (Qt.platform.os === "windows")
                            filePath = filePath.substring(8)
                        dev.exportSettingsToXML(filePath)
                    }
                }
            }

            ColumnLayout {
                RowLayout {
                    CButton {
                        text: "Import"
                        Layout.fillWidth: true
                        onClicked: {
                            importFileDialog.open()
                        }
                    }
                    CButton {
                        text: "Export"
                        Layout.fillWidth: true
                        onClicked: {
                            exportFileDialog.open()
                        }
                    }
                }
            }

        }

    }
}
