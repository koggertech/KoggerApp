import QtQuick 2.15
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
    }

    Component.onCompleted: if (root.store) root.store.applyConsoleMaxRows()

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
        model: core.consoleList
        groups: [ DelegateModelGroup { name: "selected" } ]
        delegate: RowLayout {
            width: logList.width

            TextEdit {
                Layout.fillWidth: true
                textFormat: root._colorize ? TextEdit.RichText : TextEdit.PlainText
                text: root._colorize ? root.buildLogHtml(time, payload, category)
                                     : (time + "  " + payload)
                font.pixelSize: Math.round(13 * root._s)
                font.family: "Consolas"
                color: AppPalette.text
                readOnly: true
                selectByMouse: true
                wrapMode: TextEdit.NoWrap
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
                    text: qsTr("Console")
                    color: AppPalette.textStrong
                    font.pixelSize: Tokens.fontBase
                    font.bold: true
                    Layout.alignment: Qt.AlignVCenter
                }

                Toggle {
                    id: consScrollEnable
                    checked: true
                    label: qsTr("Auto scroll")
                    Settings { category: "main/console"; property alias consScrollEnable: consScrollEnable.checked }
                }

                Toggle {
                    id: protoBinConsoled
                    checked: false
                    label: qsTr("Binary")
                    onToggled: deviceManagerWrapper.setProtoBinConsoled(protoBinConsoled.checked)
                    Component.onCompleted: deviceManagerWrapper.setProtoBinConsoled(protoBinConsoled.checked)
                    Settings { category: "main/console"; property alias protoBinConsoled: protoBinConsoled.checked }
                }

                Toggle {
                    id: nmeaConsoled
                    checked: true
                    label: qsTr("NMEA")
                    onToggled: deviceManagerWrapper.setNmeaConsoled(nmeaConsoled.checked)
                    Component.onCompleted: deviceManagerWrapper.setNmeaConsoled(nmeaConsoled.checked)
                    Settings { category: "main/console"; property alias nmeaConsoled: nmeaConsoled.checked }
                }

                Item { Layout.fillWidth: true }

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

            ListView {
                id: logList
                model: visualModel
                Layout.leftMargin: root._pad
                Layout.rightMargin: Tokens.spaceMd
                Layout.bottomMargin: Tokens.spaceSm
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true

                onCountChanged: {
                    if (consScrollEnable.checked)
                        Qt.callLater(positionViewAtEnd)
                }

                ScrollBar.vertical: ScrollBar {
                    policy: ScrollBar.AsNeeded
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
