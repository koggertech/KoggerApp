import QtQuick 2.15
import kqml_types 1.0

Item {
    id: dial

    property real angle: 0.0
    property real size: Math.round(96 * AppPalette.scale)
    property color lineColor: AppPalette.textStrong
    property color knobColor: AppPalette.accent
    property color faceColor: AppPalette.card
    property color faceBorder: AppPalette.border
    property color tickColor: AppPalette.textMuted

    signal pressed()
    signal moved(real degrees)
    signal released(real degrees)

    width: size
    height: size

    readonly property real _r: size / 2
    readonly property real _rad: angle * Math.PI / 180.0

    function _degreesAt(x, y) {
        var dx = x - dial._r
        var dy = y - dial._r
        if (dx * dx + dy * dy < 1e-6) return dial.angle
        var deg = Math.atan2(dx, -dy) * 180.0 / Math.PI
        deg = Math.round(deg)
        if (deg < 0) deg += 360
        return deg % 360
    }

    Rectangle {
        anchors.fill: parent
        radius: width / 2
        color: dial.faceColor
        border.color: area.pressed ? AppPalette.borderFocus : dial.faceBorder
        border.width: 1
    }

    Repeater {
        model: 12
        delegate: Rectangle {
            required property int index
            readonly property bool major: index % 3 === 0
            readonly property real a: index * 30 * Math.PI / 180.0
            readonly property real dist: dial._r - Math.round(6 * AppPalette.scale) - height / 2
            width: Math.max(1, Math.round((major ? 2 : 1) * AppPalette.scale))
            height: Math.round((major ? 8 : 4) * AppPalette.scale)
            radius: width / 2
            color: dial.tickColor
            x: dial._r + Math.sin(a) * dist - width / 2
            y: dial._r - Math.cos(a) * dist - height / 2
            rotation: index * 30
        }
    }

    Text {
        text: "N"
        color: dial.tickColor
        font.pixelSize: Tokens.fontXs
        font.bold: true
        anchors.horizontalCenter: parent.horizontalCenter
        y: Math.round(13 * AppPalette.scale)
    }

    Rectangle {
        id: line
        width: Math.max(2, Math.round(2 * AppPalette.scale))
        height: dial.size - Math.round(28 * AppPalette.scale)
        radius: width / 2
        color: dial.lineColor
        anchors.centerIn: parent
        rotation: dial.angle
    }

    Rectangle {
        id: knob
        readonly property real dist: line.height / 2
        width: Math.round(18 * AppPalette.scale)
        height: width
        radius: width / 2
        color: dial.knobColor
        border.color: AppPalette.accentBorder
        border.width: 1
        x: dial._r + Math.sin(dial._rad) * dist - width / 2
        y: dial._r - Math.cos(dial._rad) * dist - height / 2
        scale: area.pressed ? 1.15 : 1.0
        Behavior on scale { NumberAnimation { duration: Anim.controlMs; easing.type: Anim.controlEasing } }
    }

    Text {
        anchors.centerIn: parent
        text: Math.round(dial.angle) + "°"
        color: dial.lineColor
        font.pixelSize: Tokens.fontSm
        font.bold: true
        Rectangle {
            anchors.centerIn: parent
            width: parent.implicitWidth + Tokens.spaceSm
            height: parent.implicitHeight + Tokens.spaceXxs
            radius: Tokens.radiusSm
            color: dial.faceColor
            z: -1
        }
    }

    MouseArea {
        id: area
        anchors.fill: parent
        preventStealing: true
        cursorShape: Qt.PointingHandCursor
        onPressed: function(mouse) {
            dial.pressed()
            dial.moved(dial._degreesAt(mouse.x, mouse.y))
        }
        onPositionChanged: function(mouse) {
            if (!pressed) return
            dial.moved(dial._degreesAt(mouse.x, mouse.y))
        }
        onReleased: function(mouse) { dial.released(dial._degreesAt(mouse.x, mouse.y)) }
        onCanceled: dial.released(dial.angle)
    }
}
