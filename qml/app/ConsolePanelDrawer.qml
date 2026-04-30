import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQml.Models 2.15
import QtCore
import kqml_types 1.0
import "../controls"

Item {
    id: root

    property bool consoleOpen: false
    property real openHeight: 200
    property real maxHeight: 800
    property bool maximized: false
    property real hotActionsRight: 0

    clip: true

    height: consoleOpen ? (maximized ? maxHeight : openHeight) : 0
    Behavior on height {
        enabled: !resizeHandle.pressed
        NumberAnimation { duration: 220; easing.type: Easing.OutCubic }
    }

    Settings {
        property alias consoleOpenHeight: root.openHeight
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
                height: 16
                font.pointSize: 10
                font.family: "Console"
                color: theme ? theme.textColor : "#FFFFFF"
                readOnly: true
                selectByMouse: true
            }
        }
    }

    Rectangle {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 1
        color: AppPalette.border
        z: 1
    }

    MouseArea {
        id: resizeHandle
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 6
        cursorShape: root.maximized ? Qt.ArrowCursor : Qt.SizeVerCursor
        enabled: !root.maximized
        z: 2

        property real _startGlobalY: 0
        property real _startH: 0

        onPressed: function(mouse) {
            _startGlobalY = mapToGlobal(mouse.x, mouse.y).y
            _startH = root.openHeight
        }

        onPositionChanged: function(mouse) {
            if (!pressed) return
            var curGlobalY = mapToGlobal(mouse.x, mouse.y).y
            var delta = _startGlobalY - curGlobalY
            root.openHeight = Math.max(root.maxHeight / 5, Math.min(root.maxHeight * 4 / 5, _startH + delta))
        }
    }

    Rectangle {
        anchors.fill: parent
        anchors.topMargin: 1
        color: theme ? theme.menuBackColor : "#0B1220"

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            RowLayout {
                Layout.fillWidth: true
                Layout.leftMargin: root.maximized ? Math.max(10, root.hotActionsRight + 8) : 10
                Layout.rightMargin: 8
                Layout.topMargin: 2
                Layout.bottomMargin: 2
                height: 30
                spacing: 8

                Text {
                    text: qsTr("Console")
                    color: AppPalette.textSecond
                    font.pixelSize: 12
                    font.bold: true
                    Layout.alignment: Qt.AlignVCenter
                }

                CCheck {
                    id: consScrollEnable
                    checked: true
                    text: qsTr("Auto scroll")
                    Layout.alignment: Qt.AlignVCenter
                    Settings { property alias consScrollEnable: consScrollEnable.checked }
                }

                CCheck {
                    id: protoBinConsoled
                    checked: false
                    text: qsTr("Binary")
                    Layout.alignment: Qt.AlignVCenter
                    onCheckedChanged: deviceManagerWrapper.setProtoBinConsoled(protoBinConsoled.checked)
                    Component.onCompleted: deviceManagerWrapper.setProtoBinConsoled(protoBinConsoled.checked)
                    Settings { property alias protoBinConsoled: protoBinConsoled.checked }
                }

                Item { Layout.fillWidth: true }

                CheckButton {
                    checkable: false
                    iconSource: root.maximized
                               ? "qrc:/icons/ui/square-chevron-down.svg"
                               : "qrc:/icons/ui/square-chevron-up.svg"
                    implicitWidth: 26
                    implicitHeight: 26
                    Layout.alignment: Qt.AlignVCenter
                    onClicked: root.maximized = !root.maximized
                }

                CheckButton {
                    checkable: false
                    iconSource: "qrc:/icons/ui/x.svg"
                    implicitWidth: 26
                    implicitHeight: 26
                    Layout.alignment: Qt.AlignVCenter
                    onClicked: if (theme) theme.consoleVisible = false
                }
            }

            ListView {
                id: logList
                model: visualModel
                Layout.leftMargin: 10
                Layout.rightMargin: 10
                Layout.bottomMargin: 4
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true

                onCountChanged: {
                    if (consScrollEnable.checked)
                        Qt.callLater(positionViewAtEnd)
                }

                ScrollBar.vertical: ScrollBar { }
            }
        }
    }
}
