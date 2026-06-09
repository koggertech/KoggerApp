import QtQuick 2.15
import Qt5Compat.GraphicalEffects
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
    readonly property real _viewAvail: _avail - _pad * 2 - buttonSize - gap
    readonly property int _maxFit: Math.max(1, Math.floor((_viewAvail + gap) / (buttonSize + gap)))
    readonly property int visibleCount: Math.max(1, Math.min(5, Math.min(themeCount, _maxFit)))
    readonly property real viewportW: visibleCount * buttonSize + (visibleCount - 1) * gap
    readonly property real contentW: themeCount * buttonSize + (themeCount - 1) * gap
    readonly property real _openW: _pad * 2 + buttonSize + gap + viewportW
    readonly property int _fadeW: Math.round(buttonSize / 2) + _pad
    readonly property color _bgClear: Qt.rgba(AppPalette.bg.r, AppPalette.bg.g, AppPalette.bg.b, 0)

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
                if (!ctx)
                    return
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
        x: -root._pad
        anchors.top: parent.top
        anchors.topMargin: -root._pad
        width: root.menuOpen ? root._openW : 0
        height: root.buttonSize + root._pad * 2
        radius: height / 2
        color: AppPalette.bg
        border.width: 1
        border.color: AppPalette.border
        opacity: root.menuOpen ? 1 : 0
        visible: opacity > 0.01

        Behavior on width { NumberAnimation { duration: 200; easing.type: Easing.OutCubic } }
        Behavior on opacity { NumberAnimation { duration: 170; easing.type: Easing.OutCubic } }

        Flickable {
            id: strip
            x: root._pad + root.buttonSize + root.gap
            y: root._pad
            width: root.viewportW
            height: root.buttonSize
            contentWidth: row.width
            contentHeight: root.buttonSize
            interactive: contentWidth > width + 0.5
            flickableDirection: Flickable.HorizontalFlick
            boundsBehavior: Flickable.StopAtBounds
            layer.enabled: true
            layer.effect: OpacityMask {
                maskSource: ShaderEffectSource {
                    hideSource: true
                    sourceItem: Rectangle {
                        width: strip.width
                        height: strip.height
                        radius: strip.height / 2
                    }
                }
            }

            Row {
                id: row
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

            WheelHandler {
                acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
                onWheel: function(event) {
                    var d = event.angleDelta.y !== 0 ? event.angleDelta.y : event.angleDelta.x
                    var maxX = Math.max(0, strip.contentWidth - strip.width)
                    strip.contentX = Math.max(0, Math.min(maxX, strip.contentX - d))
                }
            }
        }

        Item {
            id: fadeOverlay
            x: strip.x
            y: strip.y
            width: strip.width
            height: strip.height
            z: 1
            layer.enabled: true
            layer.effect: OpacityMask {
                maskSource: ShaderEffectSource {
                    hideSource: true
                    sourceItem: Rectangle {
                        width: fadeOverlay.width
                        height: fadeOverlay.height
                        radius: fadeOverlay.height / 2
                    }
                }
            }

            Rectangle {
                anchors.left: parent.left
                width: root._fadeW
                height: parent.height
                visible: opacity > 0.01
                opacity: (root.menuOpen && strip.contentX > 0.5) ? 1 : 0
                gradient: Gradient {
                    orientation: Gradient.Horizontal
                    GradientStop { position: 0.0; color: AppPalette.bg }
                    GradientStop { position: 1.0; color: root._bgClear }
                }
                Behavior on opacity { NumberAnimation { duration: 120 } }
            }

            Rectangle {
                anchors.right: parent.right
                width: root._fadeW
                height: parent.height
                visible: opacity > 0.01
                opacity: (root.menuOpen && strip.contentX < (strip.contentWidth - strip.width - 0.5)) ? 1 : 0
                gradient: Gradient {
                    orientation: Gradient.Horizontal
                    GradientStop { position: 0.0; color: root._bgClear }
                    GradientStop { position: 1.0; color: AppPalette.bg }
                }
                Behavior on opacity { NumberAnimation { duration: 120 } }
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
