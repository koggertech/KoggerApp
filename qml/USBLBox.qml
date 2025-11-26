import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import QtQuick.Dialogs


DevSettingsBox {
    id: control
    isActive: dev ? (dev.isUSBLBeacon || dev.isUSBL): false

    // property bool isUSBL: dev ? (dev.isUSBL) ? true : false : false
    // property bool isBeacon: dev ? (dev.isUSBLBeacon) ? true : false : false

    property bool scanEnable: scanEnableButton.checked
    property bool isUSBL: false //modeButton.checked

    ColumnLayout {
        id: columnItem
        Layout.margins: 10
        Layout.topMargin: 0
        Layout.bottomMargin: 0

        RowLayout {
            CheckButton {
                icon.source: "qrc:/icons/ui/submarine.svg"
                enabled: false
                checkable: false
                borderColor: "transparent"
                text: "1"

                onClicked: {
                    dev.enableBeaconOnce(3);
                }
            }

            CheckButton {
                Layout.leftMargin: 12
                enabled: false
                borderColor: "transparent"
                icon.source: "qrc:/icons/ui/ruler_measure.svg"
            }

            CText {
                Layout.fillWidth: true
                text: "100.0 m"
            }
        }

        RowLayout {
            id: usblControl

            CText {
                text: "Request for:"
            }

            CheckButton {
                id: scanEnableButton
                visible: false
                iconSource: "qrc:/icons/ui/repeat.svg"
                onCheckedChanged: {
                    // deviceManagerWrapper.isbeaconDirectQueueAsk = checked
                    scanEnable = checked
                }
            }

            property int addr: 0xFF

            function pingRequest(address = 0) {
                addr = address
                var timeout = timerEnableButton.checked ? 0xFFFFFFFF : 0
                dev.acousticPingRequest(address, timeout)
            }

            CheckButton {
                checkable: scanEnable
                icon.source: "qrc:/icons/ui/radar.svg"
                onClicked: { usblControl.pingRequest(0); }
            }

            CheckButton {
                checkable: scanEnable
                text: "1"
                onClicked: { usblControl.pingRequest(1); }
            }

            CheckButton {
                checkable: scanEnable
                text: "2"
                onClicked: { usblControl.pingRequest(2); }
            }

            CheckButton {
                checkable: scanEnable
                text: "3"
                onClicked: { usblControl.pingRequest(3); }
            }

            CheckButton {
                checkable: scanEnable
                text: "4"
                onClicked: { usblControl.pingRequest(4); }
            }

            CheckButton {
                checkable: scanEnable
                text: "5"
                onClicked: { usblControl.pingRequest(5); }
            }

            CheckButton {
                checkable: scanEnable
                text: "6"
                onClicked: { usblControl.pingRequest(6); }
            }

            CheckButton {
                checkable: scanEnable
                text: "7"
                onClicked: { usblControl.pingRequest(7); }
            }

            CheckButton {
                checkable: scanEnable
                text: "8"
                onClicked: { usblControl.pingRequest(8); }
            }

            CheckButton {
                id: timerEnableButton
                Layout.alignment: Qt.AlignRight
                icon.source: "qrc:/icons/ui/separator_horizontal.svg"
                checkable: true
                text: "Trigger"
                onCheckedChanged: {}
            }
        }

        RowLayout {
            id: filterControl

            function responseFilter() {
                var address =  responceFilterButton.checked ? responceBroadcastButton.checked ? addressSpin.value : 0  : 0xFF
                dev.acousticResponceFilter(address)
            }

            CheckButton {
                // checkable: false
                checked: false
                id: responceFilterButton
                icon.source: "qrc:/icons/ui/filter_cog.svg"
                text: "Filter"
                onCheckedChanged: { filterControl.responseFilter() }
            }

            CheckButton {
                visible: responceFilterButton.checked
                checkable: true
                checked: true
                id: responceBroadcastButton
                icon.source: "qrc:/icons/ui/focus_2.svg"
                text: checked ? "Address" : "Broadcast"
                onCheckedChanged: { filterControl.responseFilter() }
            }

            SpinBoxCustom {
                id: addressSpin
                visible: responceBroadcastButton.checked && responceFilterButton.checked
                from: 1
                to: 8
                value: 1
                onValueChanged: { filterControl.responseFilter() }
            }
        }

        RowLayout {
            id: responseControl

            function responseTimeout() {
                var responseTriggerTime =  responseEnableButton.checked ?  0xFFFFFFFF : 0
                dev.acousticResponceTimeout(responseTriggerTime)
            }

            CheckButton {
                id: responseEnableButton
                checkable: true
                checked: true
                icon.source: checked ? "qrc:/icons/ui/access_point.svg" : "qrc:/icons/ui/access_point_off.svg"
                text: "Response"
                onCheckedChanged: { responseControl.responseTimeout() }
            }

            SpinBoxCustom {
                id: addressPayloadSpin
                visible: responseEnableButton.checked
                from: 0
                to: 8
                value: 1
                onValueChanged: {  }
            }
        }
    }
}
