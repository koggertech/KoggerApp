import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtQuick.Dialogs
import QtCore


// location extra settings
MenuFrame {
    id: locationSettings

    property CheckButton locationCheckButton

    visible: Qt.platform.os === "android"
             ? locationCheckButton.locationLongPressTriggered
             : (locationCheckButton.hovered                    ||
                isHovered)

    z: locationSettings.visible
    Layout.alignment: Qt.AlignCenter

    onIsHoveredChanged: {
        if (Qt.platform.os === "android") {
            if (isHovered) {
                isHovered = false
            }
        }
    }

    onVisibleChanged: {
        if (visible) {
            focus = true;
        }
    }

    onFocusChanged: {
        if (Qt.platform.os === "android" && !focus) {
            locationCheckButton.locationLongPressTriggered = false
        }
    }

    ColumnLayout {
        RowLayout {
            spacing: 16

            Rectangle {
                Layout.fillWidth: true
                height: 2
                color: "#808080"
            }

            CText {
                text: qsTr("Location settings")
            }

            Rectangle {
                Layout.fillWidth: true
                height: 2
                color: "#808080"
            }
        }

        CheckButton {
            id: useAngleButton
            objectName: "useAngleButton"
            backColor: theme.controlBackColor
            borderColor: theme.controlBackColor
            checkedBorderColor: theme.controlBorderColor
            iconSource: "qrc:/icons/ui/focus_2.svg"
            text: qsTr("Use angle")
            Layout.fillWidth: true

            onToggled: {
                Scene3dToolBarController.onUseAngleLocationButtonChanged(checked)
            }

            onFocusChanged: {
                locationSettings.focus = true
            }

            Component.onCompleted: {
                Scene3dToolBarController.onUseAngleLocationButtonChanged(checked)
            }

            Settings {
                property alias useAngleButton: useAngleButton.checked
            }
        }

        CheckButton {
            id: navigationViewButton
            objectName: "navigationViewButton"
            backColor: theme.controlBackColor
            borderColor: theme.controlBackColor
            checkedBorderColor: theme.controlBorderColor
            iconSource: "qrc:/icons/ui/arrow_bar_to_down.svg"
            text: qsTr("Navigator view")
            Layout.fillWidth: true

            onToggled: {
                Scene3dToolBarController.onNavigatorLocationButtonChanged(checked)
            }

            onFocusChanged: {
                locationSettings.focus = true
            }

            Component.onCompleted: {
                Scene3dToolBarController.onNavigatorLocationButtonChanged(checked)
            }

            Settings {
                property alias navigationViewButton: navigationViewButton.checked
            }
        }
    }
}
