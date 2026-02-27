import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Dialogs
import Qt.labs.qmlmodels


DevSettingsBox {
    id: control
    isActive: dev ? (dev.isUSBLBeacon || dev.isUSBL): false
    // isActive: true

    // property bool isUSBL: dev ? (dev.isUSBL) ? true : false : false
    // property bool isBeacon: dev ? (dev.isUSBLBeacon) ? true : false : false

    property bool isUSBL: false //modeButton.checked

    ColumnLayout {
        id: columnItem
        Layout.margins: 10
        Layout.topMargin: 0
        Layout.bottomMargin: 0

        ListModel {
            id: pingRowsModel
            ListElement {
                active: true
                address: 0
                cmd: 0
                distanceM: "20.0"
                payloadHex: ""
                triggerEnabled: false
                timeoutUs: 0
            }
        }

        QtObject {
            id: pingRowsController
            property int autoPingIndex: -1
            property var activeOnceQueue: []
            property int activeOncePos: 0

            function nextActiveIndex(startIndex) {
                if (pingRowsModel.count === 0) {
                    return -1
                }
                for (var step = 0; step < pingRowsModel.count; step++) {
                    var idx = (startIndex + step) % pingRowsModel.count
                    if (pingRowsModel.get(idx).active) {
                        return idx
                    }
                }
                return -1
            }

            function sendPingAt(index) {
                if (!dev || index < 0 || index >= pingRowsModel.count) {
                    return
                }
                var row = pingRowsModel.get(index)
                var timeout = row.triggerEnabled ? row.timeoutUs : 0
                var distance = Number(row.distanceM)
                if (isNaN(distance) || distance < 0) {
                    distance = 20.0
                }
                dev.acousticPingRequestEx(row.address, row.cmd, distance, timeout, row.payloadHex)
            }

            function startSendActiveOnce() {
                var queue = []
                for (var i = 0; i < pingRowsModel.count; i++) {
                    if (pingRowsModel.get(i).active) {
                        queue.push(i)
                    }
                }

                if (queue.length === 0) {
                    return
                }

                pingRowsSendActiveTimer.stop()
                activeOnceQueue = queue
                activeOncePos = 0

                // Send first item now, next items by timer interval.
                sendPingAt(activeOnceQueue[activeOncePos])
                activeOncePos += 1

                if (activeOncePos < activeOnceQueue.length) {
                    pingRowsSendActiveTimer.start()
                }
            }
        }

        Timer {
            id: pingRowsTimer
            interval: autoIntervalSpin.value
            repeat: true
            running: autoCycleButton.checked
            onTriggered: {
                var next = pingRowsController.nextActiveIndex(pingRowsController.autoPingIndex + 1)
                if (next >= 0) {
                    pingRowsController.autoPingIndex = next
                    pingRowsController.sendPingAt(next)
                }
            }
        }

        Timer {
            id: pingRowsSendActiveTimer
            interval: autoIntervalSpin.value
            repeat: true
            running: false
            onTriggered: {
                if (pingRowsController.activeOncePos >= pingRowsController.activeOnceQueue.length) {
                    stop()
                    return
                }

                pingRowsController.sendPingAt(pingRowsController.activeOnceQueue[pingRowsController.activeOncePos])
                pingRowsController.activeOncePos += 1

                if (pingRowsController.activeOncePos >= pingRowsController.activeOnceQueue.length) {
                    stop()
                }
            }
        }

        RowLayout {
            spacing: 8
            CheckButton {
                text: "Add Row"
                checkable: false
                onClicked: {
                    pingRowsModel.append({
                        "active": true,
                        "address": 0,
                        "cmd": 0,
                        "distanceM": "20.0",
                        "payloadHex": "",
                        "triggerEnabled": false,
                        "timeoutUs": 0
                    })
                }
            }

            CheckButton {
                text: "Send Active"
                checkable: false
                onClicked: pingRowsController.startSendActiveOnce()
            }

            CheckButton {
                id: autoCycleButton
                text: "Auto Cycle"
                checkable: true
                onCheckedChanged: {
                    if (!checked) {
                        pingRowsController.autoPingIndex = -1
                    }
                }
            }

            CText { text: "ms" }
            SpinBoxCustom {
                id: autoIntervalSpin
                from: 100
                to: 10000
                value: 1800
            }
        }

        Repeater {
            model: pingRowsModel
            delegate: RowLayout {
                spacing: 6

                CCheck {
                    checked: active
                    text: ""
                    onCheckedChanged: pingRowsModel.setProperty(index, "active", checked)
                }

                SpinBoxCustom {
                    from: 0
                    to: 7
                    value: address
                    onValueChanged: pingRowsModel.setProperty(index, "address", value)
                }

                SpinBoxCustom {
                    from: 0
                    to: 255
                    value: cmd
                    onValueChanged: pingRowsModel.setProperty(index, "cmd", value)
                }

                CTextField {
                    implicitWidth: 80
                    text: distanceM
                    placeholderText: "m"
                    onTextChanged: pingRowsModel.setProperty(index, "distanceM", text)
                }

                CTextField {
                    implicitWidth: 180
                    text: payloadHex
                    placeholderText: "AA 01 FF"
                    onTextChanged: pingRowsModel.setProperty(index, "payloadHex", text)
                }

                CheckButton {
                    text: "Trig"
                    checkable: true
                    checked: triggerEnabled
                    onCheckedChanged: pingRowsModel.setProperty(index, "triggerEnabled", checked)
                }

                SpinBoxCustom {
                    from: 0
                    to: 2147483647
                    value: timeoutUs
                    enabled: triggerEnabled
                    onValueChanged: pingRowsModel.setProperty(index, "timeoutUs", value)
                }

                CheckButton {
                    text: "Send"
                    checkable: false
                    onClicked: pingRowsController.sendPingAt(index)
                }

                CheckButton {
                    text: "Del"
                    checkable: false
                    onClicked: {
                        pingRowsModel.remove(index, 1)
                        if (pingRowsController.autoPingIndex >= pingRowsModel.count) {
                            pingRowsController.autoPingIndex = pingRowsModel.count - 1
                        }
                    }
                }
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
                implicitWidth: 260
                // Layout.fillWidth: true
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
                text: "Cmd"
            }

            SpinBoxCustom {
                id: modemCmdReceiver
                implicitWidth: 110
                from: 0
                to: 7
                value: 2
            }

            CText {
                width: 300
                text: "Bytes"
            }

            SpinBoxCustom {
                id: modemByteNumber
                implicitWidth: 110
                from: 0
                to: 20
                value: 12
            }

            CheckButton {
                id:setModemReceiver
                checkable: false
                text: "Read"
                onClicked: {
                    dev.setCmdSlotAsModemReceiver(modemCmdReceiver.value, modemByteNumber.value)
                }
            }
        }

        RowLayout {
            CText {
                text: "Received:"
            }

            CText {
                // id: receiveMsg
                width: 300
                text: dev ? dev.modemLastPayload : ""
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
            id: filterControl

            property var requestFilterEnabled: [false, false, false, false, false, false, false, false]

            function sendRequestFilter() {
                var enabled = []
                for (var i = 0; i < 8; i++) {
                    if (requestFilterEnabled[i]) {
                        enabled.push(i)
                    }
                }
                dev.acousticResponceFilterSlots(enabled)
            }

            CheckButton {
                id: requestFilterLabel
                icon.source: "qrc:/icons/ui/filter_cog.svg"
                checkable: false
                text: "Req Filter"
            }

            Repeater {
                model: 8
                delegate: CheckButton {
                    text: String(index)
                    checked: filterControl.requestFilterEnabled[index]
                    onClicked: {
                        filterControl.requestFilterEnabled[index] = checked
                        filterControl.sendRequestFilter()
                    }
                }
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
