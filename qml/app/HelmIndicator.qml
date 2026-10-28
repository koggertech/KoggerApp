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
    readonly property int _barW: Math.round(84 * _s)
    readonly property int _barH: Math.round(6 * _s)

    readonly property string _label: {
        if (!_helm) return ""
        if (_state === 1) return qsTr("Gamepad: checking")
        if (_state === 2) return qsTr("Gamepad: starting")
        if (_stopping) return qsTr("Gamepad: stopping")
        var mode = _dmw ? _dmw.autopilotModeName : ""
        return _helm.background ? qsTr("Gamepad: %1 · background").arg(mode) : qsTr("Gamepad: %1").arg(mode)
    }
    readonly property color _tone: _manual ? AppPalette.linkOkBorder : (_stopping ? AppPalette.dangerBorder : AppPalette.linkIdleBorder)

    visible: !!_helm && _helm.active
    width: chip.width
    height: chip.height

    component CenterBar: Item {
        property real value: 0
        width: root._barW
        height: root._barH

        Rectangle {
            anchors.fill: parent
            radius: height / 2
            color: AppPalette.trackOff
        }
        Rectangle {
            x: parent.width / 2 + Math.min(0, parent.value) * parent.width / 2
            width: Math.abs(parent.value) * parent.width / 2
            height: parent.height
            radius: height / 2
            color: AppPalette.accentBar
        }
        Rectangle {
            x: parent.width / 2 - width / 2
            width: Math.max(1, Math.round(root._s))
            height: parent.height
            color: AppPalette.textMuted
        }
    }

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

                CenterBar { value: root._helm ? root._helm.throttle : 0 }
                CenterBar { value: root._helm ? root._helm.steering : 0 }
            }

            Text {
                anchors.verticalCenter: parent.verticalCenter
                visible: root._driving
                text: qsTr("%1%").arg(root._helm ? Math.round(root._helm.forwardLimit * 100) : 0)
                color: AppPalette.textSecond
                font.pixelSize: Math.round(12 * root._s)
                font.bold: true
            }
        }
    }
}
