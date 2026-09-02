import QtQuick
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQml.Models 2.15
import QtCore
import kqml_types 1.0

Item {
    id: root

    property var store: null
    property bool consoleOpen: false
    property real maxHeight: 800

    signal interacted()          // any press inside the console (for last-active routing)
    property bool maximized: false
    property real hotActionsRight: 0

    property real openRatio: 0.3
    readonly property real openHeight: {
        var mh = maxHeight > 0 ? maxHeight : 800
        return Math.max(mh * 0.2, Math.min(mh * 0.8, mh * openRatio))
    }

    readonly property real _s: AppPalette.scale
    readonly property int _pad: Tokens.spaceLg
    readonly property int _btnSize: Math.round(34 * _s)
    readonly property bool _colorize: !!store && store.consoleColorize

    readonly property int srcAll: 0
    readonly property int srcApp: 1
    readonly property int srcProto: 2

    readonly property string tabLabelAll: qsTr("All")
    readonly property string tabLabelApp: qsTr("App")
    readonly property string tabLabelProto: qsTr("Protocol")
    readonly property int _tabBarWidth: {
        var textW = Math.max(tabTextAll.width, tabTextApp.width, tabTextProto.width)
        var segment = textW + Math.round(22 * _s)
        return Math.ceil(3 * segment + 2 * Math.round(6 * _s) + 2 * Math.round(4 * _s))
    }

    property int sourceTab: srcAll
    readonly property int _headerRest: Math.round(570 * _s)
    readonly property bool _headerFits: width >= _tabBarWidth + _headerRest
    readonly property bool _headerRoomy: width >= _tabBarWidth + _headerRest + Math.round(90 * _s)
    readonly property bool protoTogglesVisible: sourceTab !== srcApp && _headerFits
    readonly property var sourceModel: {
        if (!core)
            return null
        if (sourceTab === srcApp)
            return core.consoleListApp
        if (sourceTab === srcProto)
            return core.consoleListProto
        return core.consoleList
    }

    property int selAnchor: -1
    property int selCursor: -1
    property bool _pressToggles: false
    readonly property int selFirst: (selAnchor < 0 || selCursor < 0) ? -1 : Math.min(selAnchor, selCursor)
    readonly property int selLast:  (selAnchor < 0 || selCursor < 0) ? -1 : Math.max(selAnchor, selCursor)
    readonly property int selCount: selFirst < 0 ? 0 : (selLast - selFirst + 1)
    readonly property color _selBg: Qt.rgba(AppPalette.accent.r, AppPalette.accent.g, AppPalette.accent.b, 0.30)

    function clearSelection() {
        selAnchor = -1
        selCursor = -1
    }

    onSourceModelChanged: {
        clearSelection()
        if (logList && consScrollEnable && consScrollEnable.checked)
            tailPin.restart()
    }

    Timer {
        id: tailPin
        interval: 16
        repeat: true
        triggeredOnStart: true
        property int ticks: 0

        onRunningChanged: if (running) ticks = 0

        onTriggered: {
            if (logList)
                logList.positionViewAtEnd()
            if (++ticks >= 4)
                stop()
        }
    }

    function selectAll() {
        var n = logList.count
        if (n <= 0)
            return
        selAnchor = 0
        selCursor = n - 1
    }

    function _copyRange(from, to) {
        if (!core || !sourceModel || from < 0)
            return
        var text = sourceModel.rangeText(from, to)
        if (text.length)
            core.copyToClipboard(text)
    }

    function copySelection() {
        _copyRange(selFirst, selLast)
    }

    function copyAll() {
        if (logList.count > 0)
            _copyRange(0, logList.count - 1)
    }

    function _rowAt(viewportY) {
        var n = logList.count
        if (n <= 0)
            return -1

        var y = Math.max(0, Math.min(logList.height - 1, viewportY))
        var idx = logList.indexAt(logList.contentX + 1, logList.contentY + y)
        if (idx < 0 && logList.contentHeight > 0) {
            var rowH = logList.contentHeight / n
            idx = Math.floor((logList.contentY - logList.originY + y) / rowH)
        }
        return Math.max(0, Math.min(n - 1, idx))
    }

    function _selectTo(viewportY) {
        var idx = _rowAt(viewportY)
        if (idx < 0)
            return
        if (selAnchor < 0)
            selAnchor = idx
        selCursor = idx
    }

    function _tapRow(viewportY, modifiers) {
        var idx = _rowAt(viewportY)
        if (idx < 0)
            return
        if (modifiers & Qt.ShiftModifier) {
            _selectTo(viewportY)
            return
        }
        if (selCount === 1 && selFirst === idx) {
            clearSelection()
            return
        }
        selAnchor = idx
        selCursor = idx
    }

    // Observes any press inside the drawer (passive — doesn't steal from the
    // log/controls) to mark the console as the last-active scroll surface.
    TapHandler {
        acceptedButtons: Qt.AllButtons
        gesturePolicy: TapHandler.DragThreshold
        onPressedChanged: if (pressed) root.interacted()
    }

    // Keyboard scrolling (kind: "up"/"down"/"top"/"bottom") — driven by MainWindow.
    function kbdScroll(kind) {
        var f = logList
        if (!f)
            return
        if ((kind === "up" || kind === "down") && f.contentHeight <= f.height)
            return
        var top  = f.originY
        var maxY = top + Math.max(0, f.contentHeight - f.height)
        var y = kind === "top"    ? top
              : kind === "bottom" ? maxY
              : kind === "down"   ? Math.min(maxY, f.contentY + f.height * 0.9)
              :                     Math.max(top, f.contentY - f.height * 0.9)
        kbdConsoleAnim.stop()
        kbdConsoleAnim.from = f.contentY
        kbdConsoleAnim.to = y
        kbdConsoleAnim.start()
    }

    TextMetrics { id: tabTextAll;   font.pixelSize: Tokens.fontBase; font.bold: true; text: root.tabLabelAll }
    TextMetrics { id: tabTextApp;   font.pixelSize: Tokens.fontBase; font.bold: true; text: root.tabLabelApp }
    TextMetrics { id: tabTextProto; font.pixelSize: Tokens.fontBase; font.bold: true; text: root.tabLabelProto }

    NumberAnimation {
        id: kbdConsoleAnim
        target: logList
        property: "contentY"
        duration: 180
        easing.type: Easing.OutCubic
    }

    function _logEsc(s) {
        return String(s).replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;").replace(/ /g, "&nbsp;")
    }
    function _logSpan(color, text) {
        return "<span style=\"color:" + color + "\">" + _logEsc(text) + "</span>"
    }
    function buildLogHtml(time, payload, category) {
        var c = AppPalette.consoleSyntax
        var html = _logSpan(c.time, String(time) + "  ")
        var m = /^(-->>|<<--) (.*)$/.exec(payload)
        if (m) {
            var dir = m[1]
            var rest = m[2]
            html += _logSpan(dir === "-->>" ? c.dirIn : c.dirOut, dir + " ")
            var hexAt = rest.indexOf(" [ ")
            var head = hexAt >= 0 ? rest.substring(0, hexAt) : rest
            var hex = hexAt >= 0 ? rest.substring(hexAt) : ""
            var hm = /^(KG\[\d+\]: id \d+ v\d+, )([A-Za-z]+)(, len )(\d+)(;)(.*)$/.exec(head)
            if (hm) {
                html += _logSpan(c.info, hm[1]) + _logSpan(c.mode, hm[2]) + _logSpan(c.info, hm[3]) + _logSpan(c.num, hm[4]) + _logSpan(c.info, hm[5])
                var cm = hm[6]
                if (cm.trim().length)
                    html += _logSpan(cm.indexOf("Error") !== -1 ? c.error : c.comment, cm)
                else if (cm.length)
                    html += _logSpan(c.info, cm)
            } else {
                html += _logSpan(c.info, head)
            }
            if (hex.length)
                html += _logSpan(c.payload, hex)
            return html
        }

        var g = /^>> (.*)$/.exec(payload)
        if (g) {
            var body = g[1]
            html += _logSpan(c.dirIn, ">> ")
            var nm = /^NMEA: (.*)$/.exec(body)
            if (nm) {
                html += _logSpan(c.mode, "NMEA: ")
                var sent = nm[1]
                var csm = /^(.*?)(\*[0-9A-Fa-f]{2})$/.exec(sent)
                var sentCore = csm ? csm[1] : sent
                var idm = /^(\$[A-Za-z0-9]+)(.*)$/.exec(sentCore)
                if (idm)
                    html += _logSpan(c.num, idm[1]) + _logSpan(c.info, idm[2])
                else
                    html += _logSpan(c.info, sentCore)
                if (csm)
                    html += _logSpan(c.comment, csm[2])
                return html
            }
            var mav = /^(MAVLink v\d+: )(ID )(\d+)(, comp\. id )(\d+)(, seq numb )(\d+)(, len )(\d+)$/.exec(body)
            if (mav) {
                html += _logSpan(c.mode, mav[1]) + _logSpan(c.info, mav[2]) + _logSpan(c.num, mav[3])
                     + _logSpan(c.info, mav[4]) + _logSpan(c.num, mav[5])
                     + _logSpan(c.info, mav[6]) + _logSpan(c.num, mav[7])
                     + _logSpan(c.info, mav[8]) + _logSpan(c.num, mav[9])
                return html
            }
            html += _logSpan(c.info, body)
            return html
        }

        var col = category === 1 ? c.warn : ((category === 2 || category === 3) ? c.error : c.plain)
        var pm = /(^|\s)((?:[A-Za-z]:[\\\/]|\\\\|\/(?:[^\s\/]+\/)+)[^\s]*)/.exec(payload)
        if (pm) {
            var pStart = pm.index + pm[1].length
            var pEnd = pStart + pm[2].length
            if (pStart > 0)
                html += _logSpan(col, payload.substring(0, pStart))
            html += _logSpan(c.dirOut, payload.substring(pStart, pEnd))
            if (pEnd < payload.length)
                html += _logSpan(col, payload.substring(pEnd))
        } else {
            html += _logSpan(col, payload)
        }
        return html
    }

    clip: true

    height: consoleOpen ? (maximized ? maxHeight : openHeight) : 0
    Behavior on height {
        enabled: !resizeHandle.pressed
        NumberAnimation { duration: 220; easing.type: Easing.OutCubic }
    }

    Settings {
        category: "main/console"
        property alias consoleOpenRatio: root.openRatio
        property alias consoleSourceTab: root.sourceTab
    }

    Component.onCompleted: if (root.store) root.store.applyConsoleMaxRows()

    Connections {
        target: root.sourceModel

        function onRowsTrimmed(count) {
            if (root.selFirst < 0)
                return
            var a = root.selAnchor - count
            var c = root.selCursor - count
            if (Math.max(a, c) < 0) {
                root.clearSelection()
                return
            }
            root.selAnchor = Math.max(0, a)
            root.selCursor = Math.max(0, c)
        }
    }

    component Toggle: MouseArea {
        id: tgRoot
        property string label: ""
        property bool checked: false
        signal toggled(bool value)

        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        implicitHeight: Math.round(26 * root._s)
        implicitWidth: tgRow.implicitWidth
        Layout.alignment: Qt.AlignVCenter

        onClicked: {
            tgRoot.checked = !tgRoot.checked
            tgRoot.toggled(tgRoot.checked)
        }

        Row {
            id: tgRow
            anchors.verticalCenter: parent.verticalCenter
            spacing: Math.round(6 * root._s)

            Rectangle {
                id: track
                width: Math.round(38 * root._s)
                height: Math.round(20 * root._s)
                radius: height / 2
                anchors.verticalCenter: parent.verticalCenter
                color: tgRoot.checked ? AppPalette.toggleOn : AppPalette.trackOff
                border.width: 1
                border.color: tgRoot.checked ? AppPalette.toggleOnBorder : AppPalette.trackOffBorder
                Behavior on color { ColorAnimation { duration: 120 } }

                Rectangle {
                    readonly property int m: Math.max(2, Math.round(2 * root._s))
                    width: track.height - 2 * m
                    height: width
                    radius: width / 2
                    y: m
                    x: tgRoot.checked ? track.width - width - m : m
                    color: AppPalette.knob
                    border.width: 1
                    border.color: AppPalette.knobBorder
                    Behavior on x { NumberAnimation { duration: 120; easing.type: Easing.OutCubic } }
                }
            }

            Text {
                text: tgRoot.label
                color: AppPalette.textStrong
                font.pixelSize: Tokens.fontBase
                anchors.verticalCenter: parent.verticalCenter
                Behavior on color { ColorAnimation { duration: 110 } }
            }
        }
    }

    DelegateModel {
        id: visualModel
        model: root.sourceModel
        groups: [ DelegateModelGroup { name: "selected" } ]
        delegate: Rectangle {
            width: logList.width
            height: rowText.implicitHeight
            color: (index >= root.selFirst && index <= root.selLast) ? root._selBg : "transparent"

            Text {
                id: rowText
                anchors.left: parent.left
                anchors.right: parent.right
                textFormat: root._colorize ? Text.RichText : Text.PlainText
                text: root._colorize ? root.buildLogHtml(time, payload, category)
                                     : (time + "  " + payload)
                font.pixelSize: Math.round(13 * root._s)
                font.family: "Consolas"
                color: AppPalette.text
                wrapMode: Text.NoWrap
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        color: AppPalette.bg

        Rectangle {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: 1
            color: AppPalette.border
            z: 1
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.topMargin: 1
            spacing: 0

            RowLayout {
                Layout.fillWidth: true
                Layout.leftMargin: root.maximized ? Math.max(root._pad, root.hotActionsRight + Tokens.spaceMd) : root._pad
                Layout.rightMargin: Tokens.spaceMd
                Layout.topMargin: Tokens.spaceXs
                Layout.bottomMargin: Tokens.spaceXs
                Layout.preferredHeight: root._btnSize + Tokens.spaceMd
                spacing: Tokens.spaceLg

                Text {
                    visible: root._headerRoomy
                    text: qsTr("Console")
                    color: AppPalette.textStrong
                    font.pixelSize: Tokens.fontBase
                    font.bold: true
                    Layout.alignment: Qt.AlignVCenter
                }

                KTabBar {
                    id: sourceTabs
                    Layout.preferredWidth: root._tabBarWidth
                    Layout.minimumWidth: root._tabBarWidth
                    Layout.preferredHeight: root._btnSize
                    Layout.alignment: Qt.AlignVCenter
                    buttonHeight: root._btnSize - 2 * verticalPadding
                    fontPixelSize: Tokens.fontBase
                    trackColor: AppPalette.bgDeep
                    options: [
                        { label: root.tabLabelAll,   value: root.srcAll },
                        { label: root.tabLabelApp,   value: root.srcApp },
                        { label: root.tabLabelProto, value: root.srcProto }
                    ]
                    currentValue: root.sourceTab
                    onValueSelected: function(v) { root.sourceTab = v }
                }

                Toggle {
                    id: consScrollEnable
                    checked: true
                    label: qsTr("Auto scroll")
                    Settings { category: "main/console"; property alias consScrollEnable: consScrollEnable.checked }
                }

                Toggle {
                    id: protoBinConsoled
                    visible: root.protoTogglesVisible
                    label: qsTr("Binary")
                    checked: !!root.store && root.store.consoleProtoBin
                    onToggled: function(value) {
                        if (root.store)
                            root.store.consoleProtoBin = value
                        checked = Qt.binding(function() { return !!root.store && root.store.consoleProtoBin })
                    }
                }

                Toggle {
                    id: nmeaConsoled
                    visible: root.protoTogglesVisible
                    label: qsTr("NMEA")
                    checked: !!root.store && root.store.consoleNmea
                    onToggled: function(value) {
                        if (root.store)
                            root.store.consoleNmea = value
                        checked = Qt.binding(function() { return !!root.store && root.store.consoleNmea })
                    }
                }

                Item { Layout.fillWidth: true }

                KCircleIconButton {
                    implicitWidth: root._btnSize
                    implicitHeight: root._btnSize
                    rounded: false
                    cornerRadius: Tokens.radiusMd
                    iconSource: "qrc:/icons/ui/copy.svg"
                    iconTintColor: AppPalette.textStrong
                    fillColor: AppPalette.bgDeep
                    fillHoverColor: AppPalette.bgHover
                    borderWidth: 0
                    enabled: logList.count > 0
                    toolTipText: root.selCount > 0
                                 ? qsTr("Copy selected lines")
                                 : qsTr("Copy the whole log")
                    Layout.alignment: Qt.AlignVCenter
                    onClicked: root.selCount > 0 ? root.copySelection() : root.copyAll()
                }

                KCircleIconButton {
                    implicitWidth: root._btnSize
                    implicitHeight: root._btnSize
                    rounded: false
                    cornerRadius: Tokens.radiusMd
                    iconSource: root.maximized
                                ? "qrc:/icons/ui/chevron-down.svg"
                                : "qrc:/icons/ui/chevron-up.svg"
                    iconTintColor: AppPalette.textStrong
                    fillColor: AppPalette.bgDeep
                    fillHoverColor: AppPalette.bgHover
                    borderWidth: 0
                    toolTipText: root.maximized ? qsTr("Restore") : qsTr("Maximize")
                    Layout.alignment: Qt.AlignVCenter
                    onClicked: root.maximized = !root.maximized
                }

                KCircleIconButton {
                    implicitWidth: root._btnSize
                    implicitHeight: root._btnSize
                    rounded: false
                    cornerRadius: Tokens.radiusMd
                    iconSource: "qrc:/icons/ui/settings.svg"
                    iconTintColor: AppPalette.textStrong
                    fillColor: AppPalette.bgDeep
                    fillHoverColor: AppPalette.bgHover
                    borderWidth: 0
                    toolTipText: qsTr("Settings")
                    Layout.alignment: Qt.AlignVCenter
                    onClicked: if (root.store) root.store.openConsoleSettings()
                }

                KCircleIconButton {
                    implicitWidth: root._btnSize
                    implicitHeight: root._btnSize
                    rounded: false
                    cornerRadius: Tokens.radiusMd
                    glyph: "×"
                    glyphPixelSize: Math.round(18 * root._s)
                    glyphColor: AppPalette.textStrong
                    fillColor: AppPalette.bgDeep
                    fillHoverColor: AppPalette.bgHover
                    borderWidth: 0
                    Layout.alignment: Qt.AlignVCenter
                    onClicked: if (theme) theme.consoleVisible = false
                }
            }

            Item {
                Layout.leftMargin: root._pad
                Layout.rightMargin: Tokens.spaceMd
                Layout.bottomMargin: Tokens.spaceSm
                Layout.fillWidth: true
                Layout.fillHeight: true

                ListView {
                    id: logList
                    anchors.fill: parent
                    model: visualModel
                    clip: true

                    onCountChanged: {
                        if (consScrollEnable.checked && !selArea.pressed)
                            Qt.callLater(positionViewAtEnd)
                    }

                    TapHandler {
                        id: touchTap
                        acceptedDevices: PointerDevice.TouchScreen
                        gesturePolicy: TapHandler.DragThreshold
                        onSingleTapped: function(point) {
                            if (point.timeHeld < touchExtend.longPressThreshold)
                                root._tapRow(point.position.y, Qt.NoModifier)
                        }
                    }

                    TapHandler {
                        id: touchExtend
                        acceptedDevices: PointerDevice.TouchScreen
                        longPressThreshold: 0.45
                        onLongPressed: root._selectTo(touchExtend.point.position.y)
                    }

                    ScrollBar.vertical: ScrollBar {
                        id: logScroll
                        policy: ScrollBar.AsNeeded
                    }
                }

                HoverHandler {
                    id: pointerHover
                    acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
                }

                MouseArea {
                    id: selArea
                    anchors.fill: parent
                    anchors.rightMargin: logScroll.visible ? logScroll.width : 0
                    enabled: pointerHover.hovered
                    preventStealing: true
                    acceptedButtons: Qt.LeftButton
                    cursorShape: Qt.IBeamCursor

                    onPressed: function(mouse) {
                        var row = root._rowAt(mouse.y)
                        root._pressToggles = (root.selCount === 1 && root.selFirst === row)
                        if (mouse.modifiers & Qt.ShiftModifier) {
                            root._pressToggles = false
                            if (root.selAnchor < 0)
                                root.selAnchor = row
                            root.selCursor = row
                        } else {
                            root.selAnchor = row
                            root.selCursor = row
                        }
                        selEdgeScroll.start()
                    }

                    onPositionChanged: function(mouse) {
                        if (!pressed)
                            return
                        var row = root._rowAt(mouse.y)
                        if (row !== root.selCursor) {
                            root._pressToggles = false
                            root.selCursor = row
                        }
                    }

                    onReleased: {
                        selEdgeScroll.stop()
                        if (root._pressToggles)
                            root.clearSelection()
                        root._pressToggles = false
                    }

                    onCanceled: {
                        selEdgeScroll.stop()
                        root._pressToggles = false
                    }
                }

                Timer {
                    id: selEdgeScroll
                    interval: 30
                    repeat: true

                    onTriggered: {
                        if (!selArea.pressed) {
                            stop()
                            return
                        }
                        var y = selArea.mouseY
                        var step = y < 0 ? Math.max(-60, y)
                                         : (y > selArea.height ? Math.min(60, y - selArea.height) : 0)
                        if (step !== 0) {
                            var maxY = logList.originY + Math.max(0, logList.contentHeight - logList.height)
                            logList.contentY = Math.max(logList.originY, Math.min(maxY, logList.contentY + step))
                            root.selCursor = root._rowAt(y)
                        }
                    }
                }
            }
        }
    }

    MouseArea {
        id: resizeHandle
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        width: Math.round(AppPalette.dragBarLengthPx * root._s) + Tokens.spaceXl
        height: Math.round(12 * root._s)
        hoverEnabled: true
        cursorShape: root.maximized ? Qt.ArrowCursor : Qt.SizeVerCursor
        enabled: !root.maximized
        z: 3

        property real _startGlobalY: 0
        property real _startH: 0

        onPressed: function(mouse) {
            _startGlobalY = mapToGlobal(mouse.x, mouse.y).y
            _startH = root.openHeight
        }

        onPositionChanged: function(mouse) {
            if (!pressed || root.maxHeight <= 0) return
            var curGlobalY = mapToGlobal(mouse.x, mouse.y).y
            var delta = _startGlobalY - curGlobalY
            root.openRatio = Math.max(0.2, Math.min(0.8, (_startH + delta) / root.maxHeight))
        }

        Rectangle {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.verticalCenter: parent.verticalCenter
            width: Math.round(AppPalette.dragBarLengthPx * root._s)
            height: Math.max(4, Math.round(5 * root._s))
            radius: height / 2
            color: resizeHandle.containsMouse ? AppPalette.borderHover : AppPalette.border
            visible: !root.maximized
            opacity: resizeHandle.containsMouse || resizeHandle.pressed ? 1.0 : 0.8
            Behavior on opacity { NumberAnimation { duration: 140 } }
            Behavior on color { ColorAnimation { duration: 140 } }
        }
    }
}
