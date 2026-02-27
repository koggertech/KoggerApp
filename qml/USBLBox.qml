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
        spacing: 10

        ListModel {
            id: pingRowsModel
            ListElement {
                active: false
                isCurrent: false
                address: 0
                cmd: 0
                distanceM: "500.0"
                payloadHex: ""
            }
        }

        ListModel {
            id: requestCmdConfigModel
        }

        ListModel {
            id: responseCmdConfigModel
        }

        QtObject {
            id: pingRowsController
            property int autoPingIndex: -1
            property int currentPingIndex: -1
            property var activeOnceQueue: []
            property int activeOncePos: 0
            property bool activeOnceInProgress: false

            function clearCurrentPing() {
                currentPingIndex = -1
                for (var i = 0; i < pingRowsModel.count; i++) {
                    pingRowsModel.setProperty(i, "isCurrent", false)
                }
            }

            function setCurrentPing(index) {
                if (index < 0 || index >= pingRowsModel.count) {
                    clearCurrentPing()
                    return
                }
                currentPingIndex = index
                for (var i = 0; i < pingRowsModel.count; i++) {
                    pingRowsModel.setProperty(i, "isCurrent", i === index)
                }
            }

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
                    clearCurrentPing()
                    return
                }
                setCurrentPing(index)
                var row = pingRowsModel.get(index)
                var timeout = (pingTriggerSettingsButton.checked && pingGlobalTriggerButton.checked)
                    ? (pingGlobalTimeoutMsSpin.value * 1000)
                    : 0
                var distance = Number(row.distanceM)
                if (isNaN(distance) || distance < 0) {
                    distance = 500.0
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
                    activeOnceInProgress = false
                    if (!autoCycleButton.checked) {
                        clearCurrentPing()
                    }
                    return
                }

                pingRowsSendActiveTimer.stop()
                activeOnceQueue = queue
                activeOncePos = 0
                activeOnceInProgress = true

                // Send first item now, next items by timer interval.
                sendPingAt(activeOnceQueue[activeOncePos])
                activeOncePos += 1

                if (activeOncePos < activeOnceQueue.length) {
                    pingRowsSendActiveTimer.start()
                } else {
                    activeOnceInProgress = false
                    if (!autoCycleButton.checked) {
                        clearCurrentPing()
                    }
                }
            }
        }

        QtObject {
            id: cmdConfigController

            function payloadByteCount(hexText) {
                var text = String(hexText).trim()
                if (text.length === 0) {
                    return 0
                }
                var tokens = text.split(/[,\s]+/)
                var count = 0
                for (var i = 0; i < tokens.length; i++) {
                    var token = tokens[i].trim()
                    if (token.length === 0) {
                        continue
                    }
                    if (token.toLowerCase().startsWith("0x")) {
                        token = token.slice(2)
                    }
                    if (token.length === 0 || token.length > 2) {
                        return -1
                    }
                    if (!/^[0-9a-fA-F]+$/.test(token)) {
                        return -1
                    }
                    var value = parseInt(token, 16)
                    if (isNaN(value) || value < 0 || value > 255) {
                        return -1
                    }
                    count += 1
                }
                return count
            }

            function applyRow(modelObj, rowIndex, isResponseList) {
                if (!dev || rowIndex < 0 || rowIndex >= modelObj.count) {
                    return
                }
                var row = modelObj.get(rowIndex)
                if (!row.enabled) {
                    return
                }
                dev.setUsblCmdConfigRow(
                    isResponseList,
                    row.cmdId,
                    row.receiverChecked,
                    row.senderChecked,
                    true,
                    row.receiveBits,
                    row.sendingPayloadHex
                )
            }

            function applyAllRows(modelObj, isResponseList) {
                for (var i = 0; i < modelObj.count; i++) {
                    applyRow(modelObj, i, isResponseList)
                }
            }
        }

        QtObject {
            id: usblConfigController

            function toUInt32(textValue, fallback) {
                var s = String(textValue).trim()
                if (s.length === 0) {
                    return fallback
                }

                var n = Number(s)
                if (!isFinite(n) || isNaN(n)) {
                    return fallback
                }

                n = Math.round(n)
                if (n < 0) {
                    return 0
                }
                if (n > 4294967295) {
                    return 4294967295
                }
                return n
            }

            function toUInt32FromMs(textValue, fallbackUs) {
                var s = String(textValue).trim()
                if (s.length === 0) {
                    return fallbackUs
                }

                var ms = Number(s)
                if (!isFinite(ms) || isNaN(ms)) {
                    return fallbackUs
                }

                if (ms < 0) {
                    ms = 0
                }

                var us = Math.round(ms * 1000.0)
                if (us < 0) {
                    return 0
                }
                if (us > 4294967295) {
                    return 4294967295
                }
                return us
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
                } else {
                    pingRowsController.clearCurrentPing()
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
                    pingRowsController.activeOnceInProgress = false
                    if (!autoCycleButton.checked) {
                        pingRowsController.clearCurrentPing()
                    }
                    stop()
                    return
                }

                pingRowsController.sendPingAt(pingRowsController.activeOnceQueue[pingRowsController.activeOncePos])
                pingRowsController.activeOncePos += 1

                if (pingRowsController.activeOncePos >= pingRowsController.activeOnceQueue.length) {
                    pingRowsController.activeOnceInProgress = false
                    if (!autoCycleButton.checked) {
                        pingRowsController.clearCurrentPing()
                    }
                    stop()
                }
            }
        }

        RowLayout {
            spacing: 8
            CheckButton {
                icon.source: "qrc:/icons/ui/plus.svg"
                text: ""
                checkable: false
                onClicked: {
                    pingRowsModel.append({
                        "active": false,
                        "isCurrent": false,
                        "address": 0,
                        "cmd": 0,
                        "distanceM": "500.0",
                        "payloadHex": ""
                    })
                }
            }

            CheckButton {
                icon.source: "qrc:/icons/ui/click.svg"
                text: "Send Active"
                checkable: false
                active: pingRowsController.activeOnceInProgress || down
                onClicked: {
                    if (!pingRowsController.activeOnceInProgress) {
                        pingRowsController.startSendActiveOnce()
                    }
                }
            }

            CheckButton {
                id: autoCycleButton
                icon.source: "qrc:/icons/ui/repeat.svg"
                text: "Auto Cycle"
                checkable: true
                onCheckedChanged: {
                    if (!checked) {
                        pingRowsController.autoPingIndex = -1
                        if (!pingRowsController.activeOnceInProgress) {
                            pingRowsController.clearCurrentPing()
                        }
                    }
                }
            }

            CText { text: "ms" }
            SpinBoxCustom {
                id: autoIntervalSpin
                implicitWidth: 130
                from: 300
                to: 10000
                value: 1000
            }

            CheckButton {
                id: pingTriggerSettingsButton
                icon.source: "qrc:/icons/ui/settings.svg"
                text: ""
                checkable: true
                checked: false
            }
        }

        RowLayout {
            visible: pingTriggerSettingsButton.checked
            spacing: 8
            CheckButton {
                id: pingGlobalTriggerButton
                text: "Trig"
                checkable: true
                checked: false
            }
            SpinBoxCustom {
                id: pingGlobalTimeoutMsSpin
                from: 0
                to: 4294967
                value: 2147483
                visible: pingGlobalTriggerButton.checked
            }
            CText {
                text: "ms"
                visible: pingGlobalTriggerButton.checked
            }
        }

        Repeater {
            model: pingRowsModel
            delegate: Rectangle {
                property bool currentItemHighlight: (model.isCurrent === true)
                                                   && (autoCycleButton.checked || pingRowsController.activeOnceInProgress)
                Layout.fillWidth: true
                implicitHeight: pingRow.implicitHeight + 8
                radius: 6
                color: currentItemHighlight ? "#2AFFFFFF" : "#1AFFFFFF"
                border.color: currentItemHighlight ? "#66FFFFFF" : "#33FFFFFF"
                border.width: currentItemHighlight ? 2 : 1

                RowLayout {
                    id: pingRow
                    anchors.fill: parent
                    anchors.margins: 4
                    spacing: 6

                    CheckButton {
                        checked: model.active
                        icon.source:  checked ? "qrc:/icons/ui/access_point.svg" : "qrc:/icons/ui/access_point_off.svg"
                        checkable: true
                        onCheckedChanged: pingRowsModel.setProperty(index, "active", checked)
                    }

                    SpinBoxCustom {
                        implicitWidth: 85
                        from: 0
                        to: 7
                        value: address
                        onValueChanged: pingRowsModel.setProperty(index, "address", value)
                    }

                    SpinBoxCustom {
                        implicitWidth: 85
                        from: 0
                        to: 7
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
                        Layout.fillWidth: true
                        // implicitWidth: 80
                        text: payloadHex
                        // placeholderText: "AA 01 FF"
                        onTextChanged: pingRowsModel.setProperty(index, "payloadHex", text)
                    }

                    // CheckButton {
                    //     icon.source: "qrc:/icons/ui/click.svg"
                    //     text: ""
                    //     checkable: false
                    //     onClicked: pingRowsController.sendPingAt(index)
                    // }

                    CheckButton {
                        icon.source: "qrc:/icons/ui/x.svg"
                        text: ""
                        checkable: false
                        onClicked: {
                            var removedWasCurrent = index === pingRowsController.currentPingIndex
                            pingRowsModel.remove(index, 1)
                            if (removedWasCurrent) {
                                pingRowsController.clearCurrentPing()
                            } else if (index < pingRowsController.currentPingIndex) {
                                pingRowsController.setCurrentPing(pingRowsController.currentPingIndex - 1)
                            }
                            if (pingRowsController.autoPingIndex >= pingRowsModel.count) {
                                pingRowsController.autoPingIndex = pingRowsModel.count - 1
                            }
                        }
                    }
                }
            }
        }

        RowLayout {
            spacing: 8
            CText { text: "Initiator slots" }
            CheckButton {
                icon.source: "qrc:/icons/ui/plus.svg"
                text: ""
                checkable: false
                onClicked: {
                    requestCmdConfigModel.append({
                        "enabled": true,
                        "cmdId": 0,
                        "receiverChecked": true,
                        "senderChecked": false,
                        "receiveBits": 0,
                        "sendingPayloadHex": ""
                    })
                }
            }
            CheckButton {
                icon.source: "qrc:/icons/ui/file-check.svg"
                text: ""
                checkable: false
                onClicked: cmdConfigController.applyAllRows(requestCmdConfigModel, false)
            }
        }

        Repeater {
            model: requestCmdConfigModel
            delegate: Rectangle {
                Layout.fillWidth: true
                implicitHeight: reqColumn.implicitHeight + 8
                radius: 6
                color: "#1AFFFFFF"
                border.color: "#33FFFFFF"
                border.width: 1

                ColumnLayout {
                    id: reqColumn
                    anchors.fill: parent
                    anchors.margins: 4
                    spacing: 6
                    RowLayout {
                        spacing: 6
                        CheckButton {
                            icon.source: checked ? "qrc:/icons/ui/access_point.svg" : "qrc:/icons/ui/access_point_off.svg"
                            checkable: true
                            checked: enabled
                            text: ""
                            onCheckedChanged: requestCmdConfigModel.setProperty(index, "enabled", checked)
                        }
                        CText { text: "Cmd" }
                        SpinBoxCustom {
                            implicitWidth: 85
                            from: 0
                            to: 255
                            value: cmdId
                            onValueChanged: requestCmdConfigModel.setProperty(index, "cmdId", value)
                        }
                        Item { Layout.fillWidth: true }
                        CheckButton {
                            icon.source: "qrc:/icons/ui/file-check.svg"
                            text: ""
                            checkable: false
                            onClicked: cmdConfigController.applyRow(requestCmdConfigModel, index, false)
                        }
                        CheckButton {
                            icon.source: "qrc:/icons/ui/x.svg"
                            text: ""
                            checkable: false
                            onClicked: requestCmdConfigModel.remove(index, 1)
                        }
                    }

                    RowLayout {
                        spacing: 6
                        CText { text: "Receive params" }
                        CheckButton {
                            id: receiverButton
                            text: "BitArray"
                            checkable: true
                            checked: receiverChecked
                            onCheckedChanged: requestCmdConfigModel.setProperty(index, "receiverChecked", checked)
                        }
                        SpinBoxCustom {
                            visible: receiverButton.checked
                            implicitWidth: 100
                            from: 0
                            to: 65535
                            value: receiveBits
                            onValueChanged: requestCmdConfigModel.setProperty(index, "receiveBits", value)
                        }
                        Item { Layout.fillWidth: true }
                    }

                    RowLayout {
                        spacing: 6
                        CText { text: "Send params" }
                        CheckButton {
                            id: senderButton
                            text: "BitArray"
                            checkable: true
                            checked: senderChecked
                            onCheckedChanged: requestCmdConfigModel.setProperty(index, "senderChecked", checked)
                        }
                        CTextField {
                            visible: senderButton.checked
                            Layout.fillWidth: true
                            implicitWidth: 140
                            text: sendingPayloadHex
                            placeholderText: "AA 01 FF"
                            onTextChanged: requestCmdConfigModel.setProperty(index, "sendingPayloadHex", text)
                        }
                    }
                }
            }
        }

        RowLayout {
            spacing: 8
            CText { text: "Monitor slots" }
            CheckButton {
                icon.source: "qrc:/icons/ui/plus.svg"
                text: ""
                checkable: false
                onClicked: {
                    responseCmdConfigModel.append({
                        "enabled": true,
                        "cmdId": 0,
                        "receiverChecked": true,
                        "senderChecked": false,
                        "receiveBits": 0,
                        "sendingPayloadHex": ""
                    })
                }
            }
            CheckButton {
                icon.source: "qrc:/icons/ui/file-check.svg"
                text: ""
                checkable: false
                onClicked: cmdConfigController.applyAllRows(responseCmdConfigModel, true)
            }
        }

        Repeater {
            model: responseCmdConfigModel
            delegate: Rectangle {
                Layout.fillWidth: true
                implicitHeight: respColumn.implicitHeight + 8
                radius: 6
                color: "#1AFFFFFF"
                border.color: "#33FFFFFF"
                border.width: 1

                ColumnLayout {
                    id: respColumn
                    anchors.fill: parent
                    anchors.margins: 4
                    spacing: 6
                    RowLayout {
                        spacing: 6
                        CheckButton {
                            icon.source: checked ? "qrc:/icons/ui/access_point.svg" : "qrc:/icons/ui/access_point_off.svg"
                            checkable: true
                            checked: enabled
                            text: ""
                            onCheckedChanged: responseCmdConfigModel.setProperty(index, "enabled", checked)
                        }
                        CText { text: "Cmd" }
                        SpinBoxCustom {
                            implicitWidth: 85
                            from: 0
                            to: 255
                            value: cmdId
                            onValueChanged: responseCmdConfigModel.setProperty(index, "cmdId", value)
                        }
                        Item { Layout.fillWidth: true }
                        CheckButton {
                            icon.source: "qrc:/icons/ui/file-check.svg"
                            text: ""
                            checkable: false
                            onClicked: cmdConfigController.applyRow(responseCmdConfigModel, index, true)
                        }
                        CheckButton {
                            icon.source: "qrc:/icons/ui/x.svg"
                            text: ""
                            checkable: false
                            onClicked: responseCmdConfigModel.remove(index, 1)
                        }
                    }

                    RowLayout {
                        spacing: 6
                        CText { text: "Receive params" }
                        CheckButton {
                            id: receiverButton_resp
                            text: "BitArray"
                            checkable: true
                            checked: receiverChecked
                            onCheckedChanged: responseCmdConfigModel.setProperty(index, "receiverChecked", checked)
                        }
                        SpinBoxCustom {
                            visible: receiverButton_resp.checked
                            implicitWidth: 100
                            from: 0
                            to: 65535
                            value: receiveBits
                            onValueChanged: responseCmdConfigModel.setProperty(index, "receiveBits", value)
                        }
                        Item { Layout.fillWidth: true }
                    }

                    RowLayout {
                        spacing: 6
                        CText { text: "Send params" }
                        CheckButton {
                            id: senderButton_resp
                            text: "BitArray"
                            checkable: true
                            checked: senderChecked
                            onCheckedChanged: responseCmdConfigModel.setProperty(index, "senderChecked", checked)
                        }
                        CTextField {
                            visible: senderButton_resp.checked
                            Layout.fillWidth: true
                            implicitWidth: 140
                            text: sendingPayloadHex
                            placeholderText: "AA 01 FF"
                            onTextChanged: responseCmdConfigModel.setProperty(index, "sendingPayloadHex", text)
                        }
                    }
                }
            }
        }

        RowLayout {
            CText {
                text: "Received:"
            }

            CText {
                width: 300
                text: dev ? dev.modemLastPayload : ""
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

            CText {
                text: "Address filter:"
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

        Rectangle {
            Layout.fillWidth: true
            implicitHeight: transponderRow.implicitHeight + 8
            radius: 6
            color: "#1AFFFFFF"
            border.color: "#33FFFFFF"
            border.width: 1

            RowLayout {
                id: transponderRow
                anchors.fill: parent
                anchors.margins: 4
                spacing: 8

                CheckButton {
                    id: transponderEnableButton
                    checkable: true
                    checked: true
                    icon.source: checked ? "qrc:/icons/ui/access_point.svg" : "qrc:/icons/ui/access_point_off.svg"
                    text: "Transponder"
                }

                CheckButton {
                    id: transponderForceButton
                    text: "Force"
                    checkable: true
                    checked: true
                }

                CTextField {
                    id: transponderTimeoutField
                    visible: !transponderForceButton.checked
                    implicitWidth: 130
                    text: "400"
                    placeholderText: "timeout ms"
                }

                CheckButton {
                    icon.source: "qrc:/icons/ui/file-check.svg"
                    text: ""
                    checkable: false
                    onClicked: {
                        var timeoutUs = transponderForceButton.checked
                            ? 4294967295
                            : usblConfigController.toUInt32FromMs(transponderTimeoutField.text, 400000)
                        dev.setUsblTransponderEnable(transponderEnableButton.checked, timeoutUs)
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            implicitHeight: monitorRow.implicitHeight + 8
            radius: 6
            color: "#1AFFFFFF"
            border.color: "#33FFFFFF"
            border.width: 1

            ColumnLayout {
                id: monitorRow
                anchors.fill: parent
                anchors.margins: 4
                spacing: 8

                RowLayout {
                    CText {
                        text: "Monitor Suppress Response for, ms"
                    }

                    CTextField {
                        id: suppressResponseField
                        implicitWidth: 95
                        text: "400"
                        placeholderText: "resp ms"
                    }
                }


                RowLayout {
                    CText {
                        text: "Monitor Suppress Request for, ms"
                    }

                    CTextField {
                        id: suppressRequestField
                        implicitWidth: 95
                        text: "400"
                        placeholderText: "req ms"
                    }
                }

                CheckButton {
                    id: receiveInIdleButton
                    text: "Response In Idle"
                    checkable: true
                    checked: false
                }

                CheckButton {
                    icon.source: "qrc:/icons/ui/file-check.svg"
                    text: ""
                    checkable: false
                    onClicked: {
                        var suppressResp = usblConfigController.toUInt32FromMs(suppressResponseField.text, 400000)
                        var suppressReq = usblConfigController.toUInt32FromMs(suppressRequestField.text, 400000)
                        dev.setUsblMonitorConfig(suppressResp, suppressReq, receiveInIdleButton.checked)
                    }
                }
            }
        }
    }
}
