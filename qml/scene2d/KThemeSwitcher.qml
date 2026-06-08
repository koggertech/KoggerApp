import QtQuick 2.15
import kqml_types 1.0

Item {
    id: root

    property int buttonSize: Math.round(40 * (theme ? theme.resCoeff : 1.0))
    property bool menuOpen: false
    property int themeCount: 10
    property int currentId: 0
    property var stopsFor: null
    property var themeNames: []
    property real maxStripWidth: 0
    signal picked(int index)

    readonly property real _s: theme ? theme.resCoeff : 1.0
    readonly property int gap: Math.round(6 * _s)
    readonly property int _pad: Math.round(5 * _s)

    readonly property real _avail: maxStripWidth > 0 ? maxStripWidth : 100000
    readonly property int _fitCells: Math.max(1, Math.floor((_avail - _pad * 2 + gap) / (buttonSize + gap)))
    readonly property bool _wrapsTrigger: _fitCells >= (themeCount + 1)
    readonly property real _gridAvail: _wrapsTrigger ? 100000 : (_avail - buttonSize - gap - _pad * 2)
    readonly property int _cols: Math.max(1, Math.min(themeCount, Math.floor((_gridAvail + gap) / (buttonSize + gap))))
    readonly property int _rows: Math.ceil(themeCount / _cols)
    readonly property real _flowW: _cols * buttonSize + (_cols - 1) * gap
    readonly property real _flowH: _rows * buttonSize + (_rows - 1) * gap
    readonly property real _contentH: Math.max(buttonSize, _flowH)

    readonly property real _backingX: _wrapsTrigger ? -_pad : (buttonSize + gap)
    readonly property real _openW: _wrapsTrigger ? (_pad * 2 + buttonSize + gap + _flowW) : (_flowW + _pad * 2)
    readonly property real _flowXInBacking: _wrapsTrigger ? (_pad + buttonSize + gap) : _pad

    width: buttonSize
    height: buttonSize

    component Swatch: Item {
        id: sw

        property int themeId: 0
        property bool selected: false
        property alias hovered: swMouse.containsMouse
        signal activated()

        readonly property var stops: root.stopsFor ? root.stopsFor(themeId) : []

        onStopsChanged: gradientCanvas.requestPaint()
        onWidthChanged: gradientCanvas.requestPaint()
        onHeightChanged: gradientCanvas.requestPaint()

        Canvas {
            id: gradientCanvas
            anchors.fill: parent
            antialiasing: true

            onPaint: {
                var ctx = getContext("2d")
                ctx.reset()
                var s = sw.stops
                if (!s || s.length < 2)
                    return
                var cx = width / 2
                var cy = height / 2
                var r = Math.min(cx, cy) - 1
                if (r <= 0)
                    return
                var g = ctx.createLinearGradient(0, height, 0, 0)
                for (var i = 0; i < s.length; ++i)
                    g.addColorStop(s[i].pos, s[i].color)
                ctx.fillStyle = g
                ctx.beginPath()
                ctx.arc(cx, cy, r, 0, 2 * Math.PI)
                ctx.fill()
            }

            Component.onCompleted: requestPaint()
        }

        Rectangle {
            anchors.fill: parent
            radius: width / 2
            color: "transparent"
            border.width: sw.selected ? Math.max(2, Math.round(2 * root._s)) : 1
            border.color: sw.selected ? AppPalette.accentBar
                                      : (sw.hovered ? AppPalette.borderHover : AppPalette.border)
        }

        MouseArea {
            id: swMouse
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: sw.activated()
        }

        KToolTip {
            text: (root.themeNames && root.themeNames.length > sw.themeId) ? root.themeNames[sw.themeId] : ""
            targetItem: sw
            shown: sw.hovered && text.length > 0
        }
    }

    Rectangle {
        id: backing
        z: -1
        x: root._backingX
        anchors.top: parent.top
        anchors.topMargin: -root._pad
        width: root.menuOpen ? root._openW : 0
        height: root._contentH + root._pad * 2
        radius: Math.min(height / 2, root.buttonSize / 2 + root._pad)
        color: AppPalette.bg
        border.width: 1
        border.color: AppPalette.border
        clip: true
        opacity: root.menuOpen ? 1 : 0
        visible: opacity > 0.01

        Behavior on width { NumberAnimation { duration: 200; easing.type: Easing.OutCubic } }
        Behavior on opacity { NumberAnimation { duration: 170; easing.type: Easing.OutCubic } }

        Flow {
            id: flow
            x: root._flowXInBacking
            y: root._pad
            width: root._flowW
            spacing: root.gap

            Repeater {
                model: root.themeCount

                delegate: Swatch {
                    required property int index
                    width: root.buttonSize
                    height: root.buttonSize
                    themeId: index
                    selected: index === root.currentId
                    onActivated: {
                        root.picked(index)
                        root.menuOpen = false
                    }
                }
            }
        }
    }

    Swatch {
        id: trigger
        anchors.fill: parent
        themeId: root.currentId
        selected: root.menuOpen
        onActivated: root.menuOpen = !root.menuOpen
    }
}
