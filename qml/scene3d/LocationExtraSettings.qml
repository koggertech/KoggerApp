import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Dialogs
import QtCore
import kqml_types 1.0
import "../controls"
import "../menus"


MenuFrame {
    id: locationSettings

    property CheckButton locationCheckButton

    visible: Qt.platform.os === "android"
             ? locationCheckButton.locationLongPressTriggered
             : (locationCheckButton.hovered || isHovered)

    z: locationSettings.visible
    Layout.alignment: Qt.AlignCenter
    backgroundColor: AppPalette.bg
    horizontalMargins: Tokens.spaceLg
    verticalMargins: Tokens.spaceLg
    spacing: Tokens.spaceMd

    onIsHoveredChanged: {
        if (Qt.platform.os === "android" && isHovered) {
            isHovered = false
        }
    }

    onVisibleChanged: {
        if (visible) {
            focus = true
        }
    }

    onFocusChanged: {
        if (Qt.platform.os === "android" && !focus) {
            Qt.callLater(function() {
                if (!locationSettings.focus) {
                    locationCheckButton.locationLongPressTriggered = false
                }
            })
        }
    }

    readonly property int _rowWidth: Math.round(260 * AppPalette.scale)

    ColumnLayout {
        spacing: Tokens.spaceMd

        RowLayout {
            Layout.fillWidth: true
            spacing: Tokens.spaceMd

            Rectangle { Layout.fillWidth: true; height: 1; color: AppPalette.border }
            Text {
                text: qsTr("Location settings")
                color: AppPalette.textSecond
                font.pixelSize: Tokens.fontMd
                font.bold: true
            }
            Rectangle { Layout.fillWidth: true; height: 1; color: AppPalette.border }
        }

        KToggleRow {
            id: useAngleButton
            objectName: "useAngleButton"
            label: qsTr("Use angle")
            iconSource: "qrc:/icons/ui/focus_2.svg"
            Layout.preferredWidth: locationSettings._rowWidth

            onToggled: function(v) { Scene3dToolBarController.onUseAngleLocationButtonChanged(v) }
            Component.onCompleted: Scene3dToolBarController.onUseAngleLocationButtonChanged(checked)
            Settings { property alias useAngleButton: useAngleButton.checked }
        }

        KToggleRow {
            id: navigationViewButton
            objectName: "navigationViewButton"
            label: qsTr("Navigator view")
            iconSource: "qrc:/icons/ui/arrow_bar_to_down.svg"
            Layout.preferredWidth: locationSettings._rowWidth

            onToggled: function(v) { Scene3dToolBarController.onNavigatorLocationButtonChanged(v) }
            Component.onCompleted: Scene3dToolBarController.onNavigatorLocationButtonChanged(checked)
            Settings { property alias navigationViewButton: navigationViewButton.checked }
        }
    }
}
