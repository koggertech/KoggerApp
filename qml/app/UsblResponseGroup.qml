import QtQuick 2.15
import kqml_types 1.0

DeviceSettingsGroup {
    id: respGroup

    property var dev: null

    title: qsTr("USBL response")
    titlePixelSize: 13
    stateKey: "dev.usblResponse"
    collapsedByDefault: true
    visible: !!(dev && (dev.isUSBL || dev.isUSBLBeacon))

    readonly property int fwSuppressUs: 400000

    property bool respond: false
    property var acceptedSlots: [true, false, false, false, false, false, false, false]
    property int suppressResponseUs: fwSuppressUs
    property int suppressRequestUs: fwSuppressUs
    property bool receiveInIdle: false
    property bool _edited: false

    readonly property bool _known: !!(dev && dev.usblRespKnown)
    readonly property int _applyState: dev ? dev.usblRespApplyState : 0
    readonly property var _accepted: {
        var out = []
        for (var i = 0; i < acceptedSlots.length; ++i)
            if (acceptedSlots[i]) out.push(i)
        return out
    }
    readonly property bool _needsApply: _edited || !_known || _applyState === 4 || _applyState === 5

    function _deviceSlots() {
        var s = [false, false, false, false, false, false, false, false]
        if (dev && dev.usblRespFilterKnown) {
            var a = dev.usblRespAcceptedAddresses || []
            for (var i = 0; i < a.length; ++i)
                if (a[i] >= 0 && a[i] < s.length) s[a[i]] = true
        } else {
            s[0] = true
        }
        return s
    }

    function _loadFromDevice() {
        respond = !!(dev && dev.usblRespTransponderKnown && dev.usblRespEnabled)
        acceptedSlots = _deviceSlots()
        var mon = !!(dev && dev.usblRespMonitorKnown)
        suppressResponseUs = mon ? dev.usblRespSuppressResponseUs : fwSuppressUs
        suppressRequestUs = mon ? dev.usblRespSuppressRequestUs : fwSuppressUs
        receiveInIdle = mon ? dev.usblRespReceiveInIdle : false
        if (respondSwitch) respondSwitch.checked = respond
        if (idleSwitch) idleSwitch.checked = receiveInIdle
        _edited = false
    }

    function apply() {
        if (!dev) return
        dev.setUsblMonitorConfig(suppressResponseUs, suppressRequestUs, receiveInIdle)
        dev.acousticResponceFilterSlots(_accepted)
        dev.setUsblTransponderEnable(respond)
        _edited = false
    }

    onDevChanged: _loadFromDevice()
    Component.onCompleted: _loadFromDevice()

    Connections {
        target: respGroup.dev
        ignoreUnknownSignals: true
        function onUsblResponseConfigChanged() { if (!respGroup._edited) respGroup._loadFromDevice() }
    }

    readonly property var _status: {
        if (_edited)
            return { text: qsTr("Changed here, not sent yet — press Apply."), color: AppPalette.linkIdleText }
        switch (_applyState) {
        case 1: return { text: qsTr("Waiting for the device to connect; sent as soon as it does."), color: AppPalette.textSecond }
        case 2: return { text: qsTr("Sending…"), color: AppPalette.textSecond }
        case 3: return { text: qsTr("Confirmed by the device. Sent again automatically when the device restarts or reconnects."), color: AppPalette.linkOkText }
        case 4: return { text: qsTr("The device rejected these settings."), color: AppPalette.linkDownText }
        case 5: return { text: qsTr("No confirmation from the device. Check the connection and press Apply again."), color: AppPalette.linkIdleText }
        }
        return { text: qsTr("Nothing sent from the app since this device connected, so its settings are unknown. After power-up a device does not answer and accepts only address 0."),
                 color: AppPalette.textSecond }
    }

    KSwitch {
        id: respondSwitch
        width: parent.width
        text: qsTr("Respond to interrogation")
        checked: respGroup.respond
        onToggled: { respGroup.respond = checked; respGroup._edited = true }
    }

    Text {
        text: qsTr("Accept addresses")
        color: AppPalette.textMuted
        font.pixelSize: Tokens.fontSm; font.bold: true
    }

    Grid {
        id: acceptGrid
        width: parent.width
        columns: 8
        columnSpacing: Tokens.spaceXxs
        rowSpacing: Tokens.spaceXxs
        readonly property real cellW: (width - columnSpacing * 7) / 8
        Repeater {
            model: 8
            delegate: Rectangle {
                id: acceptCell
                required property int index
                readonly property bool _on: respGroup.acceptedSlots[acceptCell.index]
                width: acceptGrid.cellW
                implicitHeight: Tokens.controlHMd
                radius: Tokens.radiusSm
                color: acceptCell._on ? AppPalette.accentBg : AppPalette.card
                border.width: Math.max(1, Math.round(1 * AppPalette.scale))
                border.color: acceptCell._on ? AppPalette.accentBorder : AppPalette.border
                Text {
                    anchors.centerIn: parent
                    text: String(acceptCell.index)
                    color: acceptCell._on ? AppPalette.textStrong : AppPalette.textMuted
                    font.pixelSize: Tokens.fontSm; font.bold: true
                }
                KTapArea {
                    anchors.fill: parent
                    onTapped: {
                        var next = respGroup.acceptedSlots.slice()
                        next[acceptCell.index] = !next[acceptCell.index]
                        respGroup.acceptedSlots = next
                        respGroup._edited = true
                    }
                }
            }
        }
    }

    Text {
        width: parent.width
        visible: respGroup._accepted.length === 0
        wrapMode: Text.WordWrap
        color: AppPalette.linkDownText
        font.pixelSize: Tokens.fontXs; font.bold: true
        text: qsTr("No address selected: the device will not hear any request.")
    }

    Rectangle { width: parent.width; height: 1; color: AppPalette.border }

    Row {
        width: parent.width; height: Tokens.controlHMd; spacing: Tokens.spaceMd
        Text {
            text: qsTr("Suppress self-response, µs")
            color: AppPalette.textStrong; font.pixelSize: Tokens.fontLg
            width: Math.max(0, parent.width - parent.spacing - _supResp.width)
            elide: Text.ElideRight
            anchors.verticalCenter: parent.verticalCenter
        }
        UsblSpin {
            id: _supResp
            from: 0; to: 2000000; stepSize: 1000
            devValue: respGroup.suppressResponseUs
            anchors.verticalCenter: parent.verticalCenter
            writeBack: function (v) { respGroup.suppressResponseUs = v; respGroup._edited = true }
        }
    }

    Row {
        width: parent.width; height: Tokens.controlHMd; spacing: Tokens.spaceMd
        Text {
            text: qsTr("Suppress self-request, µs")
            color: AppPalette.textStrong; font.pixelSize: Tokens.fontLg
            width: Math.max(0, parent.width - parent.spacing - _supReq.width)
            elide: Text.ElideRight
            anchors.verticalCenter: parent.verticalCenter
        }
        UsblSpin {
            id: _supReq
            from: 0; to: 2000000; stepSize: 1000
            devValue: respGroup.suppressRequestUs
            anchors.verticalCenter: parent.verticalCenter
            writeBack: function (v) { respGroup.suppressRequestUs = v; respGroup._edited = true }
        }
    }

    KSwitch {
        id: idleSwitch
        width: parent.width
        text: qsTr("Receive while idle")
        checked: respGroup.receiveInIdle
        onToggled: { respGroup.receiveInIdle = checked; respGroup._edited = true }
    }

    Rectangle { width: parent.width; height: 1; color: AppPalette.border }

    Row {
        width: parent.width; spacing: Tokens.spaceMd
        UsblButton {
            id: applyButton
            height: Tokens.controlHMd
            fontPixelSize: Tokens.fontMd
            enabled: !!respGroup.dev
            text: qsTr("Apply")
            toolTipText: qsTr("Send all settings of this group to the device")
            normalBorder: respGroup._needsApply ? AppPalette.linkIdleBorder : AppPalette.border
            borderWidth: respGroup._needsApply ? Math.max(1, Math.round(1.5 * AppPalette.scale)) : Tokens.cardBorderWidth
            anchors.verticalCenter: parent.verticalCenter
            onClicked: respGroup.apply()
        }
        UsblButton {
            height: Tokens.controlHMd
            fontPixelSize: Tokens.fontMd
            visible: respGroup._edited
            text: qsTr("Revert")
            toolTipText: qsTr("Discard the changes made here")
            anchors.verticalCenter: parent.verticalCenter
            onClicked: respGroup._loadFromDevice()
        }
    }

    Text {
        width: parent.width
        wrapMode: Text.WordWrap
        color: respGroup._status.color
        font.pixelSize: Tokens.fontXs
        text: respGroup._status.text
    }
}
