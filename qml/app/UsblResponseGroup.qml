import QtQuick 2.15
import kqml_types 1.0

// What this device does when someone interrogates IT: whether it answers at all, whose
// addresses it accepts, and the self-suppression windows.
//
// These frames only make sense for the transponder side, but the group stays editable
// regardless of the plan's selected role — the role governs what Apply writes, not what
// you may configure.
DeviceSettingsGroup {
    id: respGroup

    property var dev: null

    title: qsTr("USBL response")
    titlePixelSize: 13
    stateKey: "dev.usblResponse"
    collapsedByDefault: true
    visible: !!(dev && (dev.isUSBL || dev.isUSBLBeacon))

    // Host intent only: ID_USBL_CONTROL has no read-back, so nothing here can be
    // confirmed against the device or restored from it.
    property var acceptedSlots: [true, true, false, false, false, false, false, false]
    property int suppressResponseUs: 0
    property int suppressRequestUs: 0
    property bool receiveInIdle: false

    function _pushFilter() {
        if (!dev) return
        var addrs = []
        for (var i = 0; i < acceptedSlots.length; ++i)
            if (acceptedSlots[i]) addrs.push(i)
        dev.acousticResponceFilterSlots(addrs)
    }
    function _pushMonitor() {
        if (!dev) return
        dev.setUsblMonitorConfig(suppressResponseUs, suppressRequestUs, receiveInIdle)
    }

    KSwitch {
        id: respondSwitch
        width: parent.width
        text: qsTr("Respond to interrogation")
        checked: true
        onToggled: if (dev) dev.setUsblTransponderEnable(checked)
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
                        respGroup._pushFilter()
                    }
                }
            }
        }
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
            writeBack: function (v) { respGroup.suppressResponseUs = v; respGroup._pushMonitor() }
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
            writeBack: function (v) { respGroup.suppressRequestUs = v; respGroup._pushMonitor() }
        }
    }

    KSwitch {
        width: parent.width
        text: qsTr("Receive while idle")
        checked: respGroup.receiveInIdle
        onToggled: { respGroup.receiveInIdle = checked; respGroup._pushMonitor() }
    }

    Text {
        width: parent.width
        wrapMode: Text.WordWrap
        color: AppPalette.textMuted
        font.pixelSize: Tokens.fontXs
        text: qsTr("The device cannot report these back, so this shows what was last sent — not necessarily what the device holds.")
    }
}
