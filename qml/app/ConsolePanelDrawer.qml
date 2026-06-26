import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQml.Models 2.15
import QtCore
import kqml_types 1.0

Item {
    id: root

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

    clip: true

    height: consoleOpen ? (maximized ? maxHeight : openHeight) : 0
    Behavior on height {
        enabled: !resizeHandle.pressed
        NumberAnimation { duration: 220; easing.type: Easing.OutCubic }
    }

    Settings {
        property alias consoleOpenRatio: root.openRatio
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
                color: tgRoot.checked ? AppPalette.accentBg : AppPalette.trackOff
                border.width: 1
                border.color: tgRoot.checked ? AppPalette.accentBorder : AppPalette.trackOffBorder
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
                color: tgRoot.containsMouse ? AppPalette.text : AppPalette.textSecond
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
                text: time + "  " + payload
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
                    color: AppPalette.textSecond
                    font.pixelSize: Tokens.fontBase
                    font.bold: true
                    Layout.alignment: Qt.AlignVCenter
                }

                Toggle {
                    id: consScrollEnable
                    checked: true
                    label: qsTr("Auto scroll")
                    Settings { property alias consScrollEnable: consScrollEnable.checked }
                }

                Toggle {
                    id: protoBinConsoled
                    checked: false
                    label: qsTr("Binary")
                    onToggled: deviceManagerWrapper.setProtoBinConsoled(protoBinConsoled.checked)
                    Component.onCompleted: deviceManagerWrapper.setProtoBinConsoled(protoBinConsoled.checked)
                    Settings { property alias protoBinConsoled: protoBinConsoled.checked }
                }

                Item { Layout.fillWidth: true }

                KCircleIconButton {
                    implicitWidth: root._btnSize
                    implicitHeight: root._btnSize
                    rounded: false
                    cornerRadius: Tokens.radiusMd
                    iconSource: root.maximized
                                ? "qrc:/icons/ui/square-chevron-down.svg"
                                : "qrc:/icons/ui/square-chevron-up.svg"
                    iconTintColor: AppPalette.textSecond
                    toolTipText: root.maximized ? qsTr("Restore") : qsTr("Maximize")
                    Layout.alignment: Qt.AlignVCenter
                    onClicked: root.maximized = !root.maximized
                }

                KCircleIconButton {
                    implicitWidth: root._btnSize
                    implicitHeight: root._btnSize
                    rounded: false
                    cornerRadius: Tokens.radiusMd
                    glyph: "×"
                    glyphPixelSize: Math.round(18 * root._s)
                    glyphColor: AppPalette.textSecond
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
        anchors.left: parent.left
        anchors.right: parent.right
        height: Math.round(8 * root._s)
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
            height: Math.max(2, Math.round(3 * root._s))
            radius: height / 2
            color: resizeHandle.containsMouse ? AppPalette.borderHover : AppPalette.border
            visible: !root.maximized
            opacity: resizeHandle.containsMouse || resizeHandle.pressed ? 1.0 : 0.6
            Behavior on opacity { NumberAnimation { duration: 140 } }
            Behavior on color { ColorAnimation { duration: 140 } }
        }
    }
}
