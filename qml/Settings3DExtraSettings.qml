import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtQuick.Dialogs
import QtCore


// settings3D extra settings
MenuFrame {
    id: settings3DSettings

    property CheckButton settings3DCheckButton

    visible: Qt.platform.os === "android"
             ? (settings3DCheckButton.settings3DLongPressTriggered)
             : (settings3DCheckButton.hovered ||
                isHovered)

    z: settings3DSettings.visible
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
            settings3DCheckButton.settings3DLongPressTriggered = false
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
                text: qsTr("3d scene settings")
            }

            Rectangle {
                Layout.fillWidth: true
                height: 2
                color: "#808080"
            }
        }

        CheckButton{
            id: cancelZoomViewButton
            iconSource: "qrc:/icons/ui/zoom_cancel.svg"
            backColor: theme.controlBackColor
            borderColor: theme.controlBackColor
            checkedBorderColor: theme.controlBorderColor
            checkable: false
            checked: false
            text: qsTr("Reset depth zoom")
            Layout.fillWidth: true
            visible: Qt.platform.os !== "android"

            onClicked: {
                Scene3dToolBarController.onCancelZoomButtonClicked()
            }
        }

        CheckButton {
            id: isNorthViewButton
            objectName: "isNorthViewButton"
            backColor: theme.controlBackColor
            borderColor: theme.controlBackColor
            checkedBorderColor: theme.controlBorderColor
            iconSource: "qrc:/icons/ui/location_pin.svg"
            text: qsTr("North mode")
            Layout.fillWidth: true

            onToggled: {
                Scene3dToolBarController.onIsNorthLocationButtonChanged(checked)
            }

            onFocusChanged: {
                settings3DSettings.focus = true
            }

            Component.onCompleted: {
                Scene3dToolBarController.onIsNorthLocationButtonChanged(checked)
            }

            Settings {
                property alias isNorthViewButton: isNorthViewButton.checked
            }
        }

        CheckButton {
            id: selectionToolButton
            objectName: "selectionToolButton"
            backColor: theme.controlBackColor
            borderColor: theme.controlBackColor
            checkedBorderColor: theme.controlBorderColor
            iconSource: "qrc:/icons/ui/click.svg"
            text: qsTr("Sync with echogram")
            Layout.fillWidth: true

            onToggled: {
                Scene3dToolBarController.onBottomTrackVertexEditingModeButtonChecked(checked)
            }

            onFocusChanged: {
                settings3DSettings.focus = true
            }

            Component.onCompleted: {
                Scene3dToolBarController.onBottomTrackVertexEditingModeButtonChecked(checked)
            }

            Settings {
                property alias selectionToolButton: selectionToolButton.checked
            }
        }

        // CheckButton {
        //     id: trackLastDataCheckButton
        //     objectName: "trackLastDataCheckButton"
        //     backColor: theme.controlBackColor
        //     borderColor: theme.controlBackColor
        //     checkedBorderColor: theme.controlBorderColor
        //     checked: false
        //     iconSource: "qrc:/icons/ui/location.svg"
        //     text: qsTr("Follow last location")
        //     Layout.fillWidth: true

        //     onToggled: {
        //         Scene3dToolBarController.onTrackLastDataCheckButtonCheckedChanged(checked)
        //     }

        //     onFocusChanged: {
        //         settings3DSettings.focus = true
        //     }

        //     Component.onCompleted: {
        //         Scene3dToolBarController.onTrackLastDataCheckButtonCheckedChanged(checked)
        //     }

        //     Settings {
        //         property alias trackLastDataCheckButton: trackLastDataCheckButton.checked
        //     }
        // }

        CheckButton {
            id: gridCheckButton
            objectName: "gridCheckButton"
            backColor: theme.controlBackColor
            borderColor: theme.controlBackColor
            checkedBorderColor: theme.controlBorderColor
            checked: true
            iconSource: "qrc:/icons/ui/grid_4x4.svg"
            text: qsTr("Grid visibility")
            Layout.fillWidth: true

            onToggled: {
                Scene3dToolBarController.onGridVisibilityCheckedChanged(checked)
            }

            onFocusChanged: {
                settings3DSettings.focus = true
            }

            Component.onCompleted: {
                Scene3dToolBarController.onGridVisibilityCheckedChanged(checked)
            }

            Settings {
                property alias gridCheckButton: gridCheckButton.checked
            }
        }

        CheckButton {
            id: navigationArrowCheckButton
            objectName: "navigationArrowCheckButton"
            backColor: theme.controlBackColor
            borderColor: theme.controlBackColor
            checkedBorderColor: theme.controlBorderColor
            checked: true
            iconSource: "qrc:/icons/ui/speedboat.svg"
            text: qsTr("Boat visibility")
            Layout.fillWidth: true

            onToggled: {
                NavigationArrowControlMenuController.onVisibilityCheckBoxCheckedChanged(checked)
            }

            onFocusChanged: {
                settings3DSettings.focus = true
            }

            Component.onCompleted: {
                NavigationArrowControlMenuController.onVisibilityCheckBoxCheckedChanged(checked)
            }

            Settings {
                property alias navigationArrowCheckButton: navigationArrowCheckButton.checked
            }
        }

        CheckButton {
            id: mapViewCheckButton
            objectName: "mapViewCheckButton"
            backColor: theme.controlBackColor
            borderColor: theme.controlBackColor
            checkedBorderColor: theme.controlBorderColor
            checked: true
            iconSource: "qrc:/icons/ui/map.svg"
            text: qsTr("Map visibility")
            Layout.fillWidth: true

            implicitWidth: theme.controlHeight

            onToggled: {
                MapViewControlMenuController.onVisibilityChanged(checked)
            }

            onFocusChanged: {
                settings3DSettings.focus = true
            }

            Component.onCompleted: {
                MapViewControlMenuController.onVisibilityChanged(checked)
            }

            Settings {
                property alias mapViewCheckButton: mapViewCheckButton.checked
            }
        }
    }
}
