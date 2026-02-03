import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtQuick.Dialogs
import QtCore


// settings3D extra settings
MenuFrame {
    id: settings3DSettings

    property CheckButton settings3DCheckButton
    property alias showQualityLabel: showQualityLabelCheck.checked

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
            id: showQualityLabelCheck
            objectName: "showQualityLabelCheck"
            backColor: theme.controlBackColor
            borderColor: theme.controlBackColor
            checkedBorderColor: theme.controlBorderColor
            checked: true
            text: qsTr("Show surface quality")
            Layout.fillWidth: true

            onFocusChanged: {
                settings3DSettings.focus = true
            }

            Settings {
                property alias showQualityLabelCheck: showQualityLabelCheck.checked
            }
        }

        RowLayout { // forcing zoom
            visible: core.needForceZooming

            CheckButton {
                id: forceSingleZoomCheckButton
                objectName: "forceSingleZoomCheckButton"
                backColor: theme.controlBackColor
                borderColor: theme.controlBackColor
                checkedBorderColor: theme.controlBorderColor
                checked: false
                text: qsTr("Force zoom")
                Layout.fillWidth: true

                onToggled: {
                    Scene3dToolBarController.onForceSingleZoomCheckedChanged(checked)
                }

                onFocusChanged: {
                    settings3DSettings.focus = true
                }

                Component.onCompleted: {
                    Scene3dToolBarController.onForceSingleZoomCheckedChanged(checked)
                }

                Settings {
                    property alias forceSingleZoomCheckButton: forceSingleZoomCheckButton.checked
                }
            }

            SpinBoxCustom {
                id: forceSingleZoomSpinBox
                objectName: "forceSingleZoomSpinBox"
                from: 2
                to: 6
                stepSize: 1
                value: 5
                enabled: forceSingleZoomCheckButton.checked

                onValueChanged: {
                    Scene3dToolBarController.onForceSingleZoomValueChanged(value)
                }

                onFocusChanged: {
                    settings3DSettings.focus = true
                }

                Component.onCompleted: {
                    Scene3dToolBarController.onForceSingleZoomValueChanged(value)
                }

                Settings {
                    property alias forceSingleZoomSpinBox: forceSingleZoomSpinBox.value
                }
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
            text: qsTr("Sync echogram")
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

        RowLayout {
            CheckButton {
                id: gridCheckButton
                objectName: "gridCheckButton"
                backColor: theme.controlBackColor
                borderColor: theme.controlBackColor
                checkedBorderColor: theme.controlBorderColor
                checked: true
                iconSource: "qrc:/icons/ui/grid_4x4.svg"
                text: qsTr("Grid")
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

            ColumnLayout {
                visible: gridCheckButton.checked

                CheckButton {
                    id: gridTypeCheckButton
                    objectName: "gridTypeCheckButton"
                    backColor: theme.controlBackColor
                    borderColor: theme.controlBackColor
                    checkedBorderColor: theme.controlBorderColor
                    checked: true
                    //iconSource: "qrc:/icons/ui/gps.svg"
                    text: qsTr("Circle")
                    Layout.fillWidth: true

                    onToggled: {
                        Scene3dToolBarController.onPlaneGridTypeChanged(!checked)
                    }
                    onFocusChanged: {
                        settings3DSettings.focus = true
                    }
                    Component.onCompleted: {
                        Scene3dToolBarController.onPlaneGridTypeChanged(!checked)
                    }
                    Settings {
                        property alias gridTypeCheckButton: gridTypeCheckButton.checked
                    }
                }

                CheckButton {
                    visible: gridTypeCheckButton.checked

                    id: gridLabelsCheckButton
                    objectName: "gridLabelsCheckButton"
                    backColor: theme.controlBackColor
                    borderColor: theme.controlBackColor
                    checkedBorderColor: theme.controlBorderColor
                    checked: true
                    //iconSource: "qrc:/icons/ui/gps.svg"
                    text: qsTr("Labels")
                    Layout.fillWidth: true

                    onToggled: {
                        Scene3dToolBarController.onPlaneGridCircleGridLabelsChanged(checked)
                    }
                    onFocusChanged: {
                        settings3DSettings.focus = true
                    }
                    Component.onCompleted: {
                        Scene3dToolBarController.onPlaneGridCircleGridLabelsChanged(checked)
                    }
                    Settings {
                        property alias gridLabelsCheckButton: gridLabelsCheckButton.checked
                    }
                }

                RowLayout {
                    visible: gridTypeCheckButton.checked

                    ColumnLayout {
                        CText {
                            text: qsTr("Size:")
                        }
                        CText {
                            text: qsTr("Step:")
                        }
                        CText {
                            text: qsTr("Angle:")
                        }
                    }

                    Item {
                        Layout.fillWidth: true
                    }

                    ColumnLayout {
                        SpinBoxCustom {
                            id: circleGridSizeSpinBox
                            from: 1
                            to: 3
                            stepSize: 1
                            value: 1

                            onValueChanged: {
                                Scene3dToolBarController.onPlaneGridCircleGridSizeChanged(value)
                            }
                            onFocusChanged: {
                                settings3DSettings.focus = true
                            }
                            Component.onCompleted: {
                                Scene3dToolBarController.onPlaneGridCircleGridSizeChanged(value)
                            }
                            Settings {
                                property alias circleGridSizeSpinBox: circleGridSizeSpinBox.value
                            }
                        }

                        SpinBoxCustom {
                            id: circleGridStepSpinBox
                            from: 1
                            to: 20
                            stepSize: 1
                            value: 1

                            onValueChanged: {
                                Scene3dToolBarController.onPlaneGridCircleGridStepChanged(value)
                            }
                            onFocusChanged: {
                                settings3DSettings.focus = true
                            }
                            Component.onCompleted: {
                                Scene3dToolBarController.onPlaneGridCircleGridStepChanged(value)
                            }
                            Settings {
                                property alias circleGridStepSpinBox: circleGridStepSpinBox.value
                            }
                        }

                        SpinBoxCustom {
                            id: circleGridAngleSpinBox
                            from: 1
                            to: 5
                            stepSize: 1
                            value: 1

                            onValueChanged: {
                                Scene3dToolBarController.onPlaneGridCircleGridAngleChanged(value)
                            }
                            onFocusChanged: {
                                settings3DSettings.focus = true
                            }
                            Component.onCompleted: {
                                Scene3dToolBarController.onPlaneGridCircleGridAngleChanged(value)
                            }
                            Settings {
                                property alias circleGridAngleSpinBox: circleGridAngleSpinBox.value
                            }
                        }
                    }
                }
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
            text: qsTr("Boat")
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

        RowLayout {
            CheckButton {
                id: compassCheckButton
                objectName: "compassCheckButton"
                backColor: theme.controlBackColor
                borderColor: theme.controlBackColor
                checkedBorderColor: theme.controlBorderColor
                checked: true
                iconSource: "qrc:/icons/ui/compass.svg"
                text: qsTr("Compass")
                Layout.fillWidth: true

                onToggled: {
                    Scene3dToolBarController.onCompassButtonChanged(checked)
                }

                onFocusChanged: {
                    settings3DSettings.focus = true
                }

                Component.onCompleted: {
                    Scene3dToolBarController.onCompassButtonChanged(checked)
                }

                Settings {
                    property alias compassCheckButton: compassCheckButton.checked
                }
            }

            RowLayout {
                visible: compassCheckButton.checked

                ColumnLayout {
                    CText {
                        text: qsTr("Pos:")
                    }
                    CText {
                        text: qsTr("Size:")
                    }
                }

                Item {
                    Layout.fillWidth: true
                }

                ColumnLayout {
                    SpinBoxCustom {
                        id: compassPosSpinBox
                        from: 1
                        to: 3
                        stepSize: 1
                        value: 1

                        onValueChanged: {
                            Scene3dToolBarController.onCompassPosChanged(value)
                        }
                        onFocusChanged: {
                            settings3DSettings.focus = true
                        }
                        Component.onCompleted: {
                            Scene3dToolBarController.onCompassPosChanged(value)
                        }
                        Settings {
                            property alias compassPosSpinBox: compassPosSpinBox.value
                        }
                    }

                    SpinBoxCustom {
                        id: compassSizeSpinBox
                        from: 1
                        to: 5
                        stepSize: 1
                        value: 1

                        onValueChanged: {
                            Scene3dToolBarController.onCompassSizeChanged(value)
                        }
                        onFocusChanged: {
                            settings3DSettings.focus = true
                        }
                        Component.onCompleted: {
                            Scene3dToolBarController.onCompassSizeChanged(value)
                        }
                        Settings {
                            property alias compassSizeSpinBox: compassSizeSpinBox.value
                        }
                    }
                }
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
            text: qsTr("Map")
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
