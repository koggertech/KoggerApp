import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import QtQuick.Dialogs
import QtQml.Models 2.15

DevSettingsBox {
    id: control
    isActive: !!(dev && dev.isRecorder)

    property bool standScanOrderValid: standScanOrderCombo.currentIndex >= 0
    property bool standAzimuthStepValid: standAzimuthStepSpin.value !== 0
    property bool standElevationStepValid: standElevationStepSpin.value !== 0
    property bool standSettleTimeValid: standSettleTimeSpin.value >= 0
    property bool standPostFireWaitValid: standPostFireWaitSpin.value >= 0
    property bool standFormValid: standScanOrderValid
                                 && standAzimuthStepValid
                                 && standElevationStepValid
                                 && standSettleTimeValid
                                 && standPostFireWaitValid

    readonly property int standDegUnitTenths: 18
    readonly property int standStepUnit: 16

    function stepsFromDegTenths(valueTenths) {
        return Math.round(valueTenths / standDegUnitTenths) * standStepUnit
    }

    function snapDegTenths(valueTenths) {
        return Math.round(valueTenths / standDegUnitTenths) * standDegUnitTenths
    }

    function standSpinText(value, locale, decimals) {
        return Number(value / 10).toLocaleString(locale, "f", decimals) + "°"
    }

    function standSpinValue(text, locale) {
        return snapDegTenths(Number.fromLocaleString(locale, text.replace("°", "").trim()) * 10)
    }

    function standValidationMessage() {
        if (!standScanOrderValid) {
            return qsTr("Choose a scan order before starting.")
        }
        if (!standAzimuthStepValid) {
            return qsTr("Azimuth step must not be 0.")
        }
        if (!standElevationStepValid) {
            return qsTr("Elevation step must not be 0.")
        }
        if (!standSettleTimeValid) {
            return qsTr("Settle time must be 0 or greater.")
        }
        if (!standPostFireWaitValid) {
            return qsTr("Post-fire wait must be 0 or greater.")
        }
        return ""
    }

    function submitStandStart() {
        if (!dev || !standFormValid) {
            return
        }

        let azStart = stepsFromDegTenths(standAzimuthStartSpin.value)
        let azEnd = stepsFromDegTenths(standAzimuthEndSpin.value)
        let azStep = stepsFromDegTenths(standAzimuthStepSpin.value)
        let elStart = stepsFromDegTenths(standElevationStartSpin.value)
        let elEnd = stepsFromDegTenths(standElevationEndSpin.value)
        let elStep = stepsFromDegTenths(standElevationStepSpin.value)

        let innerStart = standScanOrderCombo.currentIndex === 0 ? azStart : elStart
        let innerEnd = standScanOrderCombo.currentIndex === 0 ? azEnd : elEnd
        let innerStep = standScanOrderCombo.currentIndex === 0 ? azStep : elStep
        let outerStart = standScanOrderCombo.currentIndex === 0 ? elStart : azStart
        let outerEnd = standScanOrderCombo.currentIndex === 0 ? elEnd : azEnd
        let outerStep = standScanOrderCombo.currentIndex === 0 ? elStep : azStep

        dev.startStand(standScanOrderCombo.currentIndex,
                       innerStart,
                       innerEnd,
                       innerStep,
                       outerStart,
                       outerEnd,
                       outerStep,
                       standFiresPerStepSpin.value,
                       standSettleTimeSpin.value,
                       standPostFireWaitSpin.value)
    }

    ColumnLayout {
        id: columnItem
        spacing: 24
        Layout.margins: 24

        ParamGroup {
            groupName: qsTr("Stand")
            Layout.fillWidth: true

            GridLayout {
                Layout.fillWidth: true
                columns: 2
                columnSpacing: 10
                rowSpacing: 10

                CText {
                    text: qsTr("Scan order")
                }

                CCombo {
                    id: standScanOrderCombo
                    Layout.fillWidth: true
                    model: [
                        qsTr("Azimuth -> Elevation"),
                        qsTr("Elevation -> Azimuth")
                    ]
                    currentIndex: 0
                }

                CText {
                    text: qsTr("Fires per step")
                }

                SpinBoxCustom {
                    id: standFiresPerStepSpin
                    Layout.fillWidth: true
                    from: 0
                    to: 65535
                    value: 1
                    stepSize: 1
                }

                CText {
                    text: qsTr("Settle time (ms)")
                }

                SpinBoxCustom {
                    id: standSettleTimeSpin
                    Layout.fillWidth: true
                    from: 0
                    to: 65535
                    value: 250
                    stepSize: 10
                    isValid: control.standSettleTimeValid
                }

                CText {
                    text: qsTr("Post-fire wait (ms)")
                }

                SpinBoxCustom {
                    id: standPostFireWaitSpin
                    Layout.fillWidth: true
                    from: 0
                    to: 65535
                    value: 150
                    stepSize: 10
                    isValid: control.standPostFireWaitValid
                }
            }

            GridLayout {
                Layout.fillWidth: true
                columns: 4
                columnSpacing: 10
                rowSpacing: 10

                CText {
                    text: ""
                }

                CText {
                    text: qsTr("Start")
                    opacity: 0.75
                }

                CText {
                    text: qsTr("End")
                    opacity: 0.75
                }

                CText {
                    text: qsTr("Step")
                    opacity: 0.75
                }

                CText {
                    text: qsTr("Az")
                }

                SpinBoxCustom {
                    id: standAzimuthStartSpin
                    Layout.fillWidth: true
                    from: -2000000000
                    to: 2000000000
                    value: 0
                    stepSize: control.standDegUnitTenths
                    property int decimals: 1
                    validator: DoubleValidator {
                        bottom: Math.min(standAzimuthStartSpin.from / 10, standAzimuthStartSpin.to / 10)
                        top: Math.max(standAzimuthStartSpin.from / 10, standAzimuthStartSpin.to / 10)
                    }
                    textFromValue: function(value, locale) {
                        return control.standSpinText(value, locale, decimals)
                    }
                    valueFromText: function(text, locale) {
                        return control.standSpinValue(text, locale)
                    }
                }

                SpinBoxCustom {
                    id: standAzimuthEndSpin
                    Layout.fillWidth: true
                    from: -2000000000
                    to: 2000000000
                    value: 0
                    stepSize: control.standDegUnitTenths
                    property int decimals: 1
                    validator: DoubleValidator {
                        bottom: Math.min(standAzimuthEndSpin.from / 10, standAzimuthEndSpin.to / 10)
                        top: Math.max(standAzimuthEndSpin.from / 10, standAzimuthEndSpin.to / 10)
                    }
                    textFromValue: function(value, locale) {
                        return control.standSpinText(value, locale, decimals)
                    }
                    valueFromText: function(text, locale) {
                        return control.standSpinValue(text, locale)
                    }
                }

                SpinBoxCustom {
                    id: standAzimuthStepSpin
                    Layout.fillWidth: true
                    from: -2000000000
                    to: 2000000000
                    value: control.standDegUnitTenths
                    stepSize: control.standDegUnitTenths
                    isValid: control.standAzimuthStepValid
                    property int decimals: 1
                    validator: DoubleValidator {
                        bottom: Math.min(standAzimuthStepSpin.from / 10, standAzimuthStepSpin.to / 10)
                        top: Math.max(standAzimuthStepSpin.from / 10, standAzimuthStepSpin.to / 10)
                    }
                    textFromValue: function(value, locale) {
                        return control.standSpinText(value, locale, decimals)
                    }
                    valueFromText: function(text, locale) {
                        return control.standSpinValue(text, locale)
                    }
                }

                CText {
                    text: qsTr("El")
                }

                SpinBoxCustom {
                    id: standElevationStartSpin
                    Layout.fillWidth: true
                    from: -2000000000
                    to: 2000000000
                    value: 0
                    stepSize: control.standDegUnitTenths
                    property int decimals: 1
                    validator: DoubleValidator {
                        bottom: Math.min(standElevationStartSpin.from / 10, standElevationStartSpin.to / 10)
                        top: Math.max(standElevationStartSpin.from / 10, standElevationStartSpin.to / 10)
                    }
                    textFromValue: function(value, locale) {
                        return control.standSpinText(value, locale, decimals)
                    }
                    valueFromText: function(text, locale) {
                        return control.standSpinValue(text, locale)
                    }
                }

                SpinBoxCustom {
                    id: standElevationEndSpin
                    Layout.fillWidth: true
                    from: -2000000000
                    to: 2000000000
                    value: 0
                    stepSize: control.standDegUnitTenths
                    property int decimals: 1
                    validator: DoubleValidator {
                        bottom: Math.min(standElevationEndSpin.from / 10, standElevationEndSpin.to / 10)
                        top: Math.max(standElevationEndSpin.from / 10, standElevationEndSpin.to / 10)
                    }
                    textFromValue: function(value, locale) {
                        return control.standSpinText(value, locale, decimals)
                    }
                    valueFromText: function(text, locale) {
                        return control.standSpinValue(text, locale)
                    }
                }

                SpinBoxCustom {
                    id: standElevationStepSpin
                    Layout.fillWidth: true
                    from: -2000000000
                    to: 2000000000
                    value: control.standDegUnitTenths
                    stepSize: control.standDegUnitTenths
                    isValid: control.standElevationStepValid
                    property int decimals: 1
                    validator: DoubleValidator {
                        bottom: Math.min(standElevationStepSpin.from / 10, standElevationStepSpin.to / 10)
                        top: Math.max(standElevationStepSpin.from / 10, standElevationStepSpin.to / 10)
                    }
                    textFromValue: function(value, locale) {
                        return control.standSpinText(value, locale, decimals)
                    }
                    valueFromText: function(text, locale) {
                        return control.standSpinValue(text, locale)
                    }
                }
            }

            CText {
                Layout.fillWidth: true
                visible: text.length > 0
                color: "red"
                wrapMode: Text.WordWrap
                text: control.standValidationMessage()
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                CButton {
                    Layout.fillWidth: true
                    text: qsTr("Start")
                    enabled: control.standFormValid

                    onClicked: {
                        control.submitStandStart()
                    }
                }

                CButton {
                    Layout.fillWidth: true
                    text: qsTr("Pause")

                    onClicked: {
                        if (dev) {
                            dev.pauseStand()
                        }
                    }
                }

                CButton {
                    Layout.fillWidth: true
                    text: qsTr("Resume")

                    onClicked: {
                        if (dev) {
                            dev.resumeStand()
                        }
                    }
                }

                CButton {
                    Layout.fillWidth: true
                    text: qsTr("Stop")

                    onClicked: {
                        if (dev) {
                            dev.stopStand()
                        }
                    }
                }

                CButton {
                    Layout.fillWidth: true
                    text: qsTr("Home")

                    onClicked: {
                        if (dev) {
                            dev.homeStand()
                        }
                    }
                }
            }
        }

        ParamGroup {
            groupName: qsTr("Files")

            Component {
                id: fileItem

                Item {
                    id: wrapper
                    width: filesList.width
                    height: 28

                    RowLayout {
                        id: rowItem
                        spacing: 0
                        anchors.fill: parent

                        CTextField {
                            text: "#" + id
                            implicitWidth: 70
                            background: Rectangle {
                                color: recordState === 3 ? "red" : "transparent"
                                border.width: 1
                                border.color: theme.controlBorderColor
                            }
                        }

                        CTextField {
                            text: "31.12.21 11:11"
                            implicitWidth: 170
                            background: Rectangle {
                                color: "transparent"
                                border.width: 1
                                border.color: theme.controlBorderColor
                            }
                        }

                        CTextField {
                            Layout.fillWidth: true
                            text: Math.ceil(doneSize / (1024 * 1024)) + qsTr("MB / ") + Math.ceil(size / (1024 * 1024)) + qsTr(" MB")
                            background: Item {
                                Rectangle {
                                    height: parent.height
                                    anchors.bottom: parent.bottom
                                    color: "green"
                                    width: parent.width * doneSize / (size + 1)
                                }

                                Rectangle {
                                    anchors.fill: parent
                                    color: "transparent"
                                    border.width: 1
                                    border.color: theme.controlBorderColor
                                }
                            }

                            Timer {
                            }
                        }

                        CButton {
                            text: "D"
                            implicitWidth: 26
                            implicitHeight: 26
                            Layout.leftMargin: 4

                            onClicked: {
                                filesList.currentIndex = index
                                dev.requestStream(id)
                            }
                        }
                    }
                }
            }

            ListView {
                id: filesList
                model: deviceManagerWrapper.streamsList
                Layout.margins: 0
                Layout.topMargin: 20
                Layout.bottomMargin: 20
                Layout.fillWidth: true
                Layout.fillHeight: true
                height: 280
                delegate: fileItem
                focus: true
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                CButton {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 100
                    text: qsTr("Update list")

                    onClicked: {
                        dev.requestStreamList()
                    }
                }

                CButton {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 100
                    text: qsTr("Download")
                }

                CButton {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 60
                    text: qsTr("Pause")
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            CCombo {
                id: baudrateCombo
                Layout.fillWidth: true
                model: [9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600, 1200000, 2000000, 4000000, 5000000]
                currentIndex: 4
            }

            CButton {
                text: qsTr("Set baudrate")

                onClicked: {
                    dev.baudrate = Number(baudrateCombo.currentText)
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            CButton {
                Layout.fillWidth: true
                Layout.preferredWidth: 100
                text: qsTr("Flash settings")

                onClicked: {
                    dev.flashSettings()
                }
            }

            CButton {
                Layout.fillWidth: true
                Layout.preferredWidth: 100
                text: qsTr("Erase settings")

                onClicked: {
                    dev.resetSettings()
                }
            }

            CButton {
                Layout.fillWidth: true
                Layout.preferredWidth: 60
                text: qsTr("Reboot")

                onClicked: {
                    dev.reboot()
                }
            }
        }
    }
}
