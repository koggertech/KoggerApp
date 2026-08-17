import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Window 2.15
import Qt5Compat.GraphicalEffects
import kqml_types 1.0
import "RecorderStatus.js" as RecorderStatus

Item {
    id: view

    property var groups: []
    property var activeDevice
    signal deviceClicked(var device)

    readonly property int _gap: Tokens.spaceSm
    readonly property int _cellMinW: Math.round(80 * AppPalette.scale)
    readonly property int _maxCols: 4
    readonly property int _btnH: Math.round(42 * AppPalette.scale)
    readonly property int _pad: Math.round(7 * AppPalette.scale)
    readonly property int _contentW: Math.max(0, width - 2 * _pad)

    readonly property int _cols: Tokens.gridColumns(_contentW, _cellMinW, _gap, _maxCols)
    readonly property real _cellW: _cols > 0 ? Math.max(0, (_contentW - _gap * (_cols - 1)) / _cols) : _contentW

    function _linkLabel(g) {
        if (!g)
            return ""
        if (g.portName && g.portName.length > 0)
            return g.baudrate > 0 ? g.portName + " " + g.baudrate : g.portName
        if (g.address && g.address.length > 0)
            return g.destinationPort > 0 ? g.address + ":" + g.destinationPort : g.address
        return ""
    }

    // Same mapping as HotActionsPanel.iconForDevice (the hot-actions/"hotkey" panel).
    function _deviceIcon(d) {
        if (!d)                          return "qrc:/icons/ui/device-unknown.svg"
        if (d.isSonar)                   return "qrc:/icons/ui/device-transducer.svg"
        if (d.isDoppler)                 return "qrc:/icons/ui/device-doppler.svg"
        if (d.isUSBLBeacon || d.isUSBL)  return "qrc:/icons/ui/device-usbl.svg"
        if (d.isRecorder)                return "qrc:/icons/ui/device-recorder.svg"
        return "qrc:/icons/ui/device-unknown.svg"
    }

    readonly property var _rows: {
        var rows = []
        var cur = []
        var curBtns = 0
        for (var i = 0; i < groups.length; ++i) {
            var g = groups[i]
            var mem = (g && g.members) ? g.members : []
            if (mem.length === 0)
                continue
            if (cur.length > 0 && curBtns + mem.length > _cols) {
                rows.push(cur); cur = []; curBtns = 0
            }
            cur.push(g)
            curBtns += mem.length
        }
        if (cur.length > 0)
            rows.push(cur)
        return rows
    }

    function _flat(rowGroups) {
        var cells = []
        for (var gi = 0; gi < rowGroups.length; ++gi) {
            var g = rowGroups[gi]
            if (gi > 0)
                cells.push({ kind: "gap" })
            var mem = g.members || []
            var label = _linkLabel(g)
            var masterDev = g.master ? g.master.device : null
            for (var mi = 0; mi < mem.length; ++mi) {
                if (mi > 0)
                    cells.push({ kind: "conn" })
                var n = mem[mi]
                cells.push({ kind: "btn",
                             device: n.device,
                             port: (n.port !== undefined ? n.port : -1),
                             master: masterDev,
                             showLink: mi === 0,
                             linkLabel: label })
            }
        }
        return cells
    }

    implicitHeight: rowsCol.implicitHeight + 2 * _pad

    component DevPill: Rectangle {
        id: pill

        property var device
        property var master
        property bool showLink: false
        property string linkLabel: ""
        property int port: -1

        readonly property bool _selected: device === view.activeDevice
        readonly property string _state: RecorderStatus.pillState(device, master, port)
        readonly property string _sub: showLink ? linkLabel
                                                : (port >= 0 ? qsTr("Port %1").arg(port) : "")
        readonly property bool _linkTrunc: showLink && subText.overflow
        readonly property string _icon: view._deviceIcon(device)

        radius: Tokens.radiusLg
        readonly property color _baseColor: _state === "ok"   ? AppPalette.linkOkBg
                                          : _state === "warn" ? AppPalette.linkIdleBg
                                          : _state === "down" ? AppPalette.linkDownBg
                                          : _selected ? AppPalette.accentBg
                                          : AppPalette.card
        color: pillMouse.pressed       ? Qt.darker(_baseColor, 1.1)
             : pillMouse.containsMouse ? Qt.lighter(_baseColor, 1.15)
             : _baseColor
        border.width: _selected ? Math.max(2, Math.round(2 * AppPalette.scale))
                     : _state !== "" ? Math.max(1, Math.round(1 * AppPalette.scale))
                     : 0
        border.color: _selected ? AppPalette.accentBorder
                    : _state === "ok"   ? AppPalette.linkOkBorder
                    : _state === "warn" ? AppPalette.linkIdleBorder
                    : _state === "down" ? AppPalette.linkDownBorder
                    : "transparent"
        scale: pillMouse.pressed ? 0.97
             : (_selected ? 1.05 : 1.0) + (pillMouse.containsMouse ? 0.03 : 0)

        Behavior on color { ColorAnimation { duration: 110; easing.type: Easing.OutCubic } }
        Behavior on scale { NumberAnimation { duration: 120; easing.type: Easing.OutCubic } }

        Row {
            anchors.fill: parent
            anchors.leftMargin: Tokens.spaceSm
            anchors.rightMargin: Tokens.spaceSm
            spacing: Tokens.spaceSm

            Item {
                id: devIcon
                width: Math.round(22 * AppPalette.scale); height: width
                anchors.verticalCenter: parent.verticalCenter
                visible: pill._icon !== ""
                Image {
                    id: devIconImg
                    anchors.fill: parent
                    source: pill._icon
                    sourceSize.width: Math.max(1, Math.round(width * Screen.devicePixelRatio))
                    sourceSize.height: Math.max(1, Math.round(height * Screen.devicePixelRatio))
                    fillMode: Image.PreserveAspectFit
                    smooth: true
                    visible: false
                }
                ColorOverlay {
                    anchors.fill: devIconImg
                    source: devIconImg
                    color: AppPalette.textStrong
                    smooth: true
                }
            }

            Column {
                anchors.verticalCenter: parent.verticalCenter
                width: parent.width - (devIcon.visible ? devIcon.width + parent.spacing : 0)
                spacing: Math.round(1 * AppPalette.scale)

                Text {
                    width: parent.width
                    elide: Text.ElideRight
                    text: pill.device ? pill.device.devName : ""
                    color: AppPalette.textStrong
                    font.pixelSize: Tokens.fontBase
                    font.bold: true
                }
                Item {
                    id: subArea
                    width: parent.width
                    height: subText.implicitHeight
                    visible: pill._sub.length > 0
                    readonly property int fadeW: Math.round(16 * AppPalette.scale)
                    readonly property int over: Math.max(2, Math.round(2 * AppPalette.scale))
                    readonly property real _leftFadeW:  (subText.overflow && (-subText.x) > 0.5) ? Math.max(over, Math.min(fadeW, -subText.x)) : 0
                    readonly property real _rightFadeW: (subText.overflow && (subText.x - subText.leftEnd) > 0.5) ? Math.max(over, Math.min(fadeW, subText.x - subText.leftEnd)) : 0

                    Item {
                        id: subClip
                        anchors.fill: parent
                        clip: true

                        Text {
                            id: subText
                            anchors.verticalCenter: parent.verticalCenter
                            text: pill._sub
                            color: AppPalette.textSecond
                            font.pixelSize: Tokens.fontXs
                            readonly property bool overflow: width > subClip.width
                            readonly property real leftEnd: subClip.width - width   // negative: scrolled so the tail shows
                            onOverflowChanged: _resync()
                            onLeftEndChanged: if (overflow) _resync()
                            Component.onCompleted: _resync()
                            function _resync() {
                                subMarquee.stop()
                                x = 0
                                if (overflow)
                                    subMarquee.start()
                            }
                            SequentialAnimation {
                                id: subMarquee
                                loops: Animation.Infinite
                                PauseAnimation { duration: 1500 }
                                NumberAnimation { target: subText; property: "x"; to: subText.leftEnd; duration: Math.max(1500, subText.width * 6); easing.type: Easing.InOutSine }
                                PauseAnimation { duration: 1500 }
                                NumberAnimation { target: subText; property: "x"; to: 0; duration: Math.max(1500, subText.width * 6); easing.type: Easing.InOutSine }
                            }
                        }
                    }

                    Rectangle {   // left edge fade
                        anchors { left: parent.left; top: parent.top; bottom: parent.bottom; leftMargin: -subArea.over }
                        width: subArea._leftFadeW + subArea.over
                        visible: subArea._leftFadeW > 0.5
                        gradient: Gradient {
                            orientation: Gradient.Horizontal
                            GradientStop { position: 0.0; color: pill.color }
                            GradientStop { position: 0.5; color: pill.color }
                            GradientStop { position: 1.0; color: "transparent" }
                        }
                    }
                    Rectangle {   // right edge fade
                        anchors { right: parent.right; top: parent.top; bottom: parent.bottom; rightMargin: -subArea.over }
                        width: subArea._rightFadeW + subArea.over
                        visible: subArea._rightFadeW > 0.5
                        gradient: Gradient {
                            orientation: Gradient.Horizontal
                            GradientStop { position: 0.0; color: "transparent" }
                            GradientStop { position: 0.5; color: pill.color }
                            GradientStop { position: 1.0; color: pill.color }
                        }
                    }
                }
            }
        }

        MouseArea {
            id: pillMouse
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: view.deviceClicked(pill.device)
        }

        KToolTip {
            text: pill.linkLabel
            targetItem: pill
            shown: pillMouse.containsMouse && pill._linkTrunc
        }
    }

    Column {
        id: rowsCol
        x: view._pad
        y: view._pad
        width: view._contentW
        spacing: view._gap

        Repeater {
            model: view._rows
            delegate: Row {
                required property var modelData
                spacing: 0

                Repeater {
                    model: view._flat(modelData)
                    delegate: Item {
                        required property var modelData

                        readonly property var cell: modelData
                        readonly property bool _isBtn: cell.kind === "btn"

                        width: _isBtn ? view._cellW : view._gap
                        height: view._btnH
                        z: _isBtn ? 1 : 0

                        DevPill {
                            anchors.fill: parent
                            visible: parent._isBtn
                            device: parent._isBtn ? parent.cell.device : null
                            master: parent._isBtn ? parent.cell.master : null
                            port: parent._isBtn ? parent.cell.port : -1
                            showLink: parent._isBtn ? parent.cell.showLink : false
                            linkLabel: parent._isBtn ? parent.cell.linkLabel : ""
                        }

                        Rectangle {
                            anchors.centerIn: parent
                            width: parent.width
                            height: Math.max(2, Math.round(2 * AppPalette.scale))
                            color: AppPalette.textMuted
                            visible: parent.cell.kind === "conn"
                        }
                    }
                }
            }
        }
    }
}
