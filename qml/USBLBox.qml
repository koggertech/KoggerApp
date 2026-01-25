import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Dialogs
import Qt.labs.qmlmodels


DevSettingsBox {
    id: control
    // isActive: dev ? (dev.isUSBLBeacon || dev.isUSBL): false
    isActive: true

    // property bool isUSBL: dev ? (dev.isUSBL) ? true : false : false
    // property bool isBeacon: dev ? (dev.isUSBLBeacon) ? true : false : false

    property bool scanEnable: scanEnableButton.checked
    property bool isUSBL: false //modeButton.checked

    ColumnLayout {
        id: columnItem
        Layout.margins: 10
        Layout.topMargin: 0
        Layout.bottomMargin: 0

        Item {
            id: root

            // your default timeout for acousticPingRequest
            property int pingTimeoutMs: 0

            // 0 = beacon first, 1 = sensor first
            property int _phase: 0

            function _doPing(address, cmd) {
                // Replace/adjust as needed:
                dev.acousticPingRequest(address, cmd, pingTimeoutMs)
            }

            function _tryPingBeacon() : bool {
                if (!beaconEnabled.checked)
                    return false
                _doPing(beaconAddress.value, beaconCmd.value)
                return true
            }

            function _tryPingSensor() : bool {
                if (!sensorEnabled.checked)
                    return false
                _doPing(sensorAddress.value, sensorCmd.value)
                return true
            }

            Timer {
                id: pingTimer
                interval: 1800
                repeat: true
                running: true  // set false if you want a master enable switch

                onTriggered: {
                    // Interleaved: try current phase first, then the other.
                    var sent = false

                    if (root._phase === 0) {
                        sent = root._tryPingBeacon()
                        if (!sent) sent = root._tryPingSensor()
                    } else {
                        sent = root._tryPingSensor()
                        if (!sent) sent = root._tryPingBeacon()
                    }

                    // advance phase regardless; this preserves "interleaving" intention
                    root._phase = 1 - root._phase
                }
            }
        }

        RowLayout {
            CCheck {
                id: beaconEnabled
                text: "Beacon"
                checked: false
            }

            SpinBoxCustom {
                id: beaconAddress
                from: 0
                to: 7
                value: 0
            }

            SpinBoxCustom {
                id: beaconCmd
                from: 0
                to: 7
                value: 0
            }
        }

        RowLayout {
            CCheck {
                id: sensorEnabled
                text: "Sensor"
                checked: false
            }

            SpinBoxCustom {
                id: sensorAddress
                from: 0
                to: 7
                value: 0
            }

            SpinBoxCustom {
                id: sensorCmd
                from: 0
                to: 7
                value: 2
            }
        }

        RowLayout {
            spacing: 10
            SpinBoxCustom {
                id: modemCmdSlot
                implicitWidth: 90
                from: 0
                to: 7
                value: 2
            }
            CTextField {
                id:modemResponsePayload
                // implicitWidth: 260
                Layout.fillWidth: true
                text: "modem"
            }

            CText {
                // how many bytes in modemResponsePayload
                width: 20
                text: modemResponsePayload.text.length + " B"
            }

            CheckButton {
                id:setModemResponse
                checkable: false
                text: "Set MSG"
                onClicked: {
                    dev.setCmdSlotAsModemResponse(modemCmdSlot.value, modemResponsePayload.text)
                }
            }
        }

        RowLayout {
            CText {
                width: 300
                text: ""
            }
        }


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

            function pingRequest(address = 0, cmd = 0) {
                addr = address
                var timeout = timerEnableButton.checked ? 0xFFFFFFFF : 0
                dev.acousticPingRequest(address, cmd, timeout)
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
