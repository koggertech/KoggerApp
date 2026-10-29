import QtQuick 2.15
import kqml_types 1.0

Item {
    id: root

    readonly property var _helm: (typeof helm !== "undefined") ? helm : null
    readonly property var _dmw: (typeof deviceManagerWrapper !== "undefined") ? deviceManagerWrapper : null
    readonly property real _s: AppPalette.appScale
    readonly property int _mode: _dmw ? _dmw.pilotModeState : -1
    readonly property int _state: _helm ? _helm.state : 0
    readonly property bool _driving: _state === 3
    readonly property bool _stopping: _state === 4
    readonly property bool _manual: _driving && _mode === 0
    property bool cardOpen: false

    readonly property string _label: {
        if (!_helm) return ""
        if (_state === 1) return qsTr("Gamepad: checking")
        if (_state === 2) return qsTr("Gamepad: starting")
        if (_stopping) return qsTr("Gamepad: stopping")
        var mode = _dmw ? _dmw.autopilotModeName : ""
        return _helm.background ? qsTr("Gamepad: %1 · background").arg(mode) : qsTr("Gamepad: %1").arg(mode)
    }
    readonly property color _tone: _manual ? AppPalette.linkOkBorder : (_stopping ? AppPalette.dangerBorder : AppPalette.linkIdleBorder)

    visible: !!_helm && _helm.active && !cardOpen
    width: chip.width
    height: chip.height

    Rectangle {
        id: chip
        width: content.width + Math.round(28 * root._s)
        height: Math.round(44 * root._s)
        radius: height / 2
        color: AppPalette.card
        border.color: root._tone
        border.width: Math.max(2, Math.round(2 * root._s))

        Row {
            id: content
            anchors.centerIn: parent
            spacing: Math.round(10 * root._s)

            Rectangle {
                anchors.verticalCenter: parent.verticalCenter
                width: Math.round(10 * root._s)
                height: width
                radius: width / 2
                color: root._tone
            }

            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: root._label
                color: AppPalette.textStrong
                font.pixelSize: Math.round(14 * root._s)
                font.bold: true
            }

            Column {
                anchors.verticalCenter: parent.verticalCenter
                spacing: Math.round(4 * root._s)
                visible: root._driving

                HelmBar { value: root._helm ? root._helm.throttle : 0 }
                HelmBar { value: root._helm ? root._helm.steering : 0 }
            }
        }
    }
}
