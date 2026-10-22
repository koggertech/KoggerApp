import QtQuick 2.15
import QtCore
import kqml_types 1.0

Item {
    id: root

    property bool open: false
    property string title: ""
    property string subtitle: ""
    property string settingsCategory: "main/nodesWindow"
    property real minWidth: Math.round(520 * AppPalette.scale)
    property real minHeight: Math.round(320 * AppPalette.scale)
    property real defaultWidth: Math.round(1080 * AppPalette.scale)
    property real defaultHeight: Math.round(620 * AppPalette.scale)
    property real margin: Tokens.spaceMd
    property int transparency: geo.transparency
    property bool collapsed: geo.collapsed
    // Fills the parent minus these insets; the floating geometry is kept and comes back on restore.
    property bool maximized: geo.maximized
    property real maxInsetTop: 0
    property real maxInsetLeft: 0
    property real maxInsetRight: 0
    property real maxInsetBottom: 0
    // Whether the last press in the application window landed on this window: keyboard shortcuts follow it.
    property bool keysInside: false

    default property alias content: body.data
    property alias headerTools: toolsRow.data

    readonly property real headerHeight: Tokens.controlHMd + 2 * Tokens.spaceXs
    readonly property real _bgAlpha: Math.max(0.08, 1 - transparency / 100)
    readonly property real _contentOpacity: Math.max(0.55, 1 - 0.45 * transparency / 100)
    readonly property rect windowRect: Qt.rect(win.x, win.y, win.width, win.height)

    property real _rx: margin
    property real _ry: margin
    property real _rw: defaultWidth
    property real _rh: defaultHeight

    signal closeRequested()
    signal interacted()

    visible: open

    Settings {
        id: geo
        category: root.settingsCategory
        property real wx: -1
        property real wy: -1
        property real ww: 0
        property real wh: 0
        property int transparency: 10
        property bool collapsed: false
        property bool maximized: true
    }

    function setTransparency(v) { geo.transparency = Math.max(0, Math.min(90, Math.round(v))) }
    function setCollapsed(v) { geo.collapsed = !!v; _clampIntoView() }
    function setMaximized(v) { geo.maximized = !!v; _clampIntoView() }

    function _clampIntoView() {
        if (width <= 0 || height <= 0) return
        var w = Math.max(minWidth, Math.min(_rw, width - 2 * margin))
        var h = Math.max(minHeight, Math.min(_rh, height - 2 * margin))
        var shownH = collapsed ? headerHeight : h
        _rw = w
        _rh = h
        _rx = Math.max(margin, Math.min(_rx, width - w - margin))
        _ry = Math.max(margin, Math.min(_ry, height - shownH - margin))
    }

    function _commit() {
        geo.wx = _rx; geo.wy = _ry; geo.ww = _rw; geo.wh = _rh
    }

    function _restore() {
        var w = geo.ww > 0 ? geo.ww : Math.min(defaultWidth, width - 2 * margin)
        var h = geo.wh > 0 ? geo.wh : Math.min(defaultHeight, height - 2 * margin)
        _rw = Math.max(minWidth, w)
        _rh = Math.max(minHeight, h)
        _rx = geo.wx >= 0 ? geo.wx : Math.max(margin, width - _rw - margin)
        _ry = geo.wy >= 0 ? geo.wy : Math.max(margin, height - _rh - margin * 6)
        _clampIntoView()
    }

    function resetGeometry() {
        geo.wx = -1; geo.wy = -1; geo.ww = 0; geo.wh = 0
        _restore()
        _commit()
    }

    onWidthChanged: if (_ready) _clampIntoView()
    onHeightChanged: if (_ready) _clampIntoView()
    onOpenChanged: {
        keysInside = open
        if (open && _ready) _clampIntoView()
    }
    property bool _ready: false
    Component.onCompleted: { _restore(); _ready = true }

    PointHandler {
        acceptedButtons: Qt.AllButtons
        onActiveChanged: if (active) root.keysInside = root._inWindow(point.position)
    }
    function _inWindow(p) {
        var g = _gripT
        return p.x >= win.x - g && p.x < win.x + win.width + g && p.y >= win.y - g && p.y < win.y + win.height + g
    }

    component WindowMark: Item {
        property bool restore: false
        property color ink: AppPalette.text
        readonly property real _s: Math.round(9 * AppPalette.scale)
        readonly property real _t: Math.max(1, Math.round(1.4 * AppPalette.scale))
        width: _s + (restore ? Math.round(3 * AppPalette.scale) : 0)
        height: width
        Rectangle {
            visible: parent.restore
            x: Math.round(3 * AppPalette.scale); y: 0
            width: parent._s; height: parent._s
            color: "transparent"
            border.width: parent._t
            border.color: parent.ink
        }
        Rectangle {
            x: 0; y: parent.restore ? Math.round(3 * AppPalette.scale) : 0
            width: parent._s; height: parent._s
            color: parent.restore ? AppPalette.headerBg : "transparent"
            border.width: parent._t
            border.color: parent.ink
        }
    }

    Rectangle {
        id: win
        x: root.maximized ? root.maxInsetLeft : root._rx
        y: root.maximized ? root.maxInsetTop : root._ry
        width: root.maximized ? Math.max(root.minWidth, root.width - root.maxInsetLeft - root.maxInsetRight) : root._rw
        height: root.collapsed ? root.headerHeight
                               : (root.maximized ? Math.max(root.headerHeight, root.height - root.maxInsetTop - root.maxInsetBottom)
                                                 : root._rh)
        radius: root.maximized ? 0 : Tokens.radiusLg
        color: Qt.rgba(AppPalette.bg.r, AppPalette.bg.g, AppPalette.bg.b, root._bgAlpha)
        border.width: root.maximized ? 0 : Math.max(1, Math.round(AppPalette.scale))
        border.color: AppPalette.border
        clip: true

        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.AllButtons
            hoverEnabled: true
            onPressed: function (mouse) { root.interacted(); mouse.accepted = true }
            onWheel: function (wheel) { wheel.accepted = true }
        }

        Rectangle {
            id: header
            width: parent.width
            height: root.headerHeight
            color: Qt.rgba(AppPalette.headerBg.r, AppPalette.headerBg.g, AppPalette.headerBg.b,
                           Math.max(0.35, root._bgAlpha))
            radius: win.radius

            Rectangle {
                anchors.bottom: parent.bottom
                width: parent.width; height: parent.radius
                color: parent.color
                visible: !root.collapsed && parent.radius > 0
            }

            Item {
                id: dragZone
                anchors.left: parent.left
                anchors.right: toolsRow.left
                anchors.top: parent.top
                anchors.bottom: parent.bottom

                property real _sx: 0
                property real _sy: 0

                HoverHandler {
                    cursorShape: root.maximized ? Qt.ArrowCursor
                                                : (dragHandler.active ? Qt.ClosedHandCursor : Qt.OpenHandCursor)
                }
                TapHandler {
                    onTapped: root.interacted()
                    onDoubleTapped: root.setMaximized(!root.maximized)
                }
                DragHandler {
                    id: dragHandler
                    target: null
                    enabled: !root.maximized
                    onActiveChanged: {
                        if (active) { dragZone._sx = root._rx; dragZone._sy = root._ry; root.interacted() }
                        else root._commit()
                    }
                    onTranslationChanged: {
                        if (!active) return
                        root._rx = Math.max(root.margin, Math.min(dragZone._sx + translation.x, root.width - win.width - root.margin))
                        root._ry = Math.max(root.margin, Math.min(dragZone._sy + translation.y, root.height - win.height - root.margin))
                    }
                }

                Row {
                    anchors.left: parent.left
                    anchors.leftMargin: Tokens.spaceLg
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: Tokens.spaceSm
                    Text {
                        text: root.title
                        color: AppPalette.textStrong
                        font.pixelSize: Tokens.fontSm; font.bold: true
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Text {
                        text: root.subtitle
                        visible: text.length > 0
                        color: AppPalette.textSecond
                        font.pixelSize: Tokens.fontSm
                        elide: Text.ElideMiddle
                        width: Math.min(implicitWidth, Math.max(0, dragZone.width - Tokens.spaceLg * 3 - 160 * AppPalette.scale))
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
            }

            Row {
                id: toolsRow
                anchors.right: windowButtons.left
                anchors.rightMargin: Tokens.spaceSm
                anchors.verticalCenter: parent.verticalCenter
                spacing: Tokens.spaceSm
            }

            Row {
                id: windowButtons
                anchors.right: parent.right
                anchors.rightMargin: Tokens.spaceSm
                anchors.verticalCenter: parent.verticalCenter
                spacing: Tokens.spaceXs

                Item {
                    width: _opacityRow.implicitWidth
                    height: Tokens.controlHMd
                    anchors.verticalCenter: parent.verticalCenter
                    Row {
                        id: _opacityRow
                        spacing: Tokens.spaceXs
                        anchors.verticalCenter: parent.verticalCenter
                        Text {
                            text: "◐"
                            color: AppPalette.textSecond
                            font.pixelSize: Tokens.fontSm
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        KSlider {
                            id: transparencySlider
                            from: 0; to: 90; stepSize: 1
                            value: root.transparency
                            showValueTip: true
                            valueSuffix: "%"
                            knobSize: Math.round(18 * AppPalette.scale)
                            trackHeight: Math.round(5 * AppPalette.scale)
                            implicitWidth: Math.round(96 * AppPalette.scale)
                            implicitHeight: Tokens.controlHMd
                            toolTipText: qsTr("Transparency")
                            anchors.verticalCenter: parent.verticalCenter
                            onValueModified: function (v) { root.setTransparency(v) }
                        }
                    }
                }
                KCircleIconButton {
                    glyph: root.collapsed ? "▢" : "–"
                    width: Tokens.controlHMd; height: Tokens.controlHMd
                    glyphPixelSize: Math.round(13 * AppPalette.scale)
                    borderWidth: Tokens.cardBorderWidth
                    toolTipText: root.collapsed ? qsTr("Expand") : qsTr("Collapse to the title bar")
                    anchors.verticalCenter: parent.verticalCenter
                    onClicked: root.setCollapsed(!root.collapsed)
                }
                KCircleIconButton {
                    id: maxBtn
                    width: Tokens.controlHMd; height: Tokens.controlHMd
                    borderWidth: Tokens.cardBorderWidth
                    toolTipText: root.maximized ? qsTr("Restore the floating window (double-click the title)")
                                                : qsTr("Fill the whole window (double-click the title)")
                    anchors.verticalCenter: parent.verticalCenter
                    onClicked: root.setMaximized(!root.maximized)
                    WindowMark {
                        anchors.centerIn: parent
                        restore: root.maximized
                        ink: maxBtn.hovered ? AppPalette.textStrong : AppPalette.text
                    }
                }
                KCircleIconButton {
                    glyph: "×"
                    width: Tokens.controlHMd; height: Tokens.controlHMd
                    glyphPixelSize: Math.round(14 * AppPalette.scale)
                    borderWidth: Tokens.cardBorderWidth
                    toolTipText: qsTr("Close — the cycle keeps running")
                    anchors.verticalCenter: parent.verticalCenter
                    onClicked: root.closeRequested()
                }
            }
        }

        Item {
            id: body
            anchors.top: header.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            visible: !root.collapsed
            opacity: root._contentOpacity
            clip: true
        }
    }

    component Grip: MouseArea {
        id: grip
        property int edges: 0
        property real _sx: 0
        property real _sy: 0
        property real _sw: 0
        property real _sh: 0
        property point _p0: Qt.point(0, 0)
        hoverEnabled: true
        visible: !root.collapsed && !root.maximized
        preventStealing: true
        z: 20
        onPressed: function (mouse) {
            _sx = root._rx; _sy = root._ry; _sw = root._rw; _sh = root._rh
            _p0 = mapToItem(root, mouse.x, mouse.y)
            root.interacted()
        }
        onPositionChanged: function (mouse) {
            if (!pressed) return
            var p = mapToItem(root, mouse.x, mouse.y)
            var dx = p.x - _p0.x, dy = p.y - _p0.y
            var x = _sx, y = _sy, w = _sw, h = _sh
            if (edges & 1) { w = Math.max(root.minWidth, _sw - dx); x = Math.max(root.margin, _sx + _sw - w) ; w = _sx + _sw - x }
            if (edges & 2) { w = Math.max(root.minWidth, Math.min(_sw + dx, root.width - _sx - root.margin)) }
            if (edges & 4) { h = Math.max(root.minHeight, _sh - dy); y = Math.max(root.margin, _sy + _sh - h); h = _sy + _sh - y }
            if (edges & 8) { h = Math.max(root.minHeight, Math.min(_sh + dy, root.height - _sy - root.margin)) }
            root._rx = x; root._ry = y; root._rw = w; root._rh = h
        }
        onReleased: root._commit()
    }

    readonly property real _gripT: Math.round(6 * AppPalette.scale)
    readonly property real _gripC: Math.round(14 * AppPalette.scale)

    Grip { edges: 1; x: win.x - root._gripT / 2; y: win.y + root._gripC; width: root._gripT; height: win.height - 2 * root._gripC; cursorShape: Qt.SizeHorCursor }
    Grip { edges: 2; x: win.x + win.width - root._gripT / 2; y: win.y + root._gripC; width: root._gripT; height: win.height - 2 * root._gripC; cursorShape: Qt.SizeHorCursor }
    Grip { edges: 4; x: win.x + root._gripC; y: win.y - root._gripT / 2; width: win.width - 2 * root._gripC; height: root._gripT; cursorShape: Qt.SizeVerCursor }
    Grip { edges: 8; x: win.x + root._gripC; y: win.y + win.height - root._gripT / 2; width: win.width - 2 * root._gripC; height: root._gripT; cursorShape: Qt.SizeVerCursor }
    Grip { edges: 1 | 4; x: win.x - root._gripT / 2; y: win.y - root._gripT / 2; width: root._gripC; height: root._gripC; cursorShape: Qt.SizeFDiagCursor }
    Grip { edges: 2 | 8; x: win.x + win.width - root._gripC + root._gripT / 2; y: win.y + win.height - root._gripC + root._gripT / 2; width: root._gripC; height: root._gripC; cursorShape: Qt.SizeFDiagCursor }
    Grip { edges: 2 | 4; x: win.x + win.width - root._gripC + root._gripT / 2; y: win.y - root._gripT / 2; width: root._gripC; height: root._gripC; cursorShape: Qt.SizeBDiagCursor }
    Grip { edges: 1 | 8; x: win.x - root._gripT / 2; y: win.y + win.height - root._gripC + root._gripT / 2; width: root._gripC; height: root._gripC; cursorShape: Qt.SizeBDiagCursor }
}
