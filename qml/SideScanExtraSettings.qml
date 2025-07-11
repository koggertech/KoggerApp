import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import Qt.labs.settings 1.1
import QtQuick.Dialogs 1.2


// side-scan extra settings
MenuFrame {
    id: sideScanViewSettings

    property CheckButton sideScanViewCheckButton

    function updateMosaic() {
        updateMosaicButton.clicked();
    }

    visible: Qt.platform.os === "android"
             ? (sideScanViewCheckButton.sideScanLongPressTriggered || sideScanTheme.activeFocus)
             : (sideScanViewCheckButton.hovered                    ||
                isHovered                                          ||
                sideScanTheme.activeFocus)

    z: sideScanViewSettings.visible
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
            sideScanViewCheckButton.sideScanLongPressTriggered = false
        }
    }

    RowLayout {
        ColumnLayout {
            CheckButton {
                id: realtimeProcessingButton
                text: qsTr("Realtime processing")
                Layout.fillWidth: true
                Layout.preferredWidth: 200
                checked: false

                onToggled: {
                    SideScanViewControlMenuController.onUpdateStateChanged(checked)
                }

                onFocusChanged: {
                    sideScanViewSettings.focus = true
                }

                Component.onCompleted: {
                    //realtimeProcessingButton.checked = core.isSeparateReading
                    SideScanViewControlMenuController.onUpdateStateChanged(checked)
                }

                Settings {
                    property alias realtimeProcessingButton: realtimeProcessingButton.checked
                }
            }
            RowLayout {
                CText {
                    text: qsTr("Theme:")
                }
                Item {
                    Layout.fillWidth: true
                }
                CCombo  {
                    id: sideScanTheme
                    Layout.preferredWidth: 300
                    model: [qsTr("Blue"), qsTr("Sepia"), qsTr("WRGBD"), qsTr("WhiteBlack"), qsTr("BlackWhite")]
                    currentIndex: 0
                    onCurrentIndexChanged: {
                        SideScanViewControlMenuController.onThemeChanged(currentIndex)
                    }

                    onFocusChanged: {
                        if (Qt.platform.os === 'android') {
                            sideScanViewSettings.focus = true
                        }
                    }

                    Component.onCompleted: {
                        SideScanViewControlMenuController.onThemeChanged(currentIndex)
                    }

                    Settings {
                        property alias sideScanTheme: sideScanTheme.currentIndex
                    }
                }
            }
            RowLayout {
                CText {
                    text: qsTr("Angle offset, °")
                }
                Item {
                    Layout.fillWidth: true
                }
                ColumnLayout {
                    RowLayout {
                        Item {
                            Layout.fillWidth: true
                        }
                        CText {
                            text: qsTr("left:")
                        }
                        SpinBoxCustom  {
                            id: sideScanLAngleOffset
                            implicitWidth: 200
                            from: -90
                            to: 90
                            stepSize: 1
                            value: 0
                            editable: false

                            onValueChanged: {
                                SideScanViewControlMenuController.onSetLAngleOffset(value)
                            }

                            onFocusChanged: {
                                sideScanViewSettings.focus = true
                            }

                            Component.onCompleted: {
                                SideScanViewControlMenuController.onSetLAngleOffset(value)
                            }

                            Settings {
                                property alias sideScanLAngleOffset: sideScanLAngleOffset.value
                            }
                        }
                    }
                    RowLayout {
                        Item {
                            Layout.fillWidth: true
                        }
                        CText {
                            text: qsTr("right:")
                        }
                        SpinBoxCustom  {
                            id: sideScanRAngleOffset
                            implicitWidth: 200
                            from: -90
                            to: 90
                            stepSize: 1
                            value: 0
                            editable: false

                            onValueChanged: {
                                SideScanViewControlMenuController.onSetRAngleOffset(value)
                            }

                            onFocusChanged: {
                                sideScanViewSettings.focus = true
                            }

                            Component.onCompleted: {
                                SideScanViewControlMenuController.onSetRAngleOffset(value)
                            }

                            Settings {
                                property alias sideScanRAngleOffset: sideScanRAngleOffset.value
                            }
                        }
                    }
                }
            }

            ColumnLayout {
                RowLayout {
                    //visible: core.isSeparateReading

                    CText {
                        text: qsTr("Tile side pixel size:")
                    }
                    Item {
                        Layout.fillWidth: true
                    }
                    SpinBoxCustom {
                        id: sideScanTileSidePixelSizeSpinBox
                        implicitWidth: 200
                        from: 32
                        to: 2048
                        stepSize: 1
                        value: 256
                        editable: false

                        onFocusChanged: {
                            sideScanViewSettings.focus = true
                        }
                    }
                }

                RowLayout {
                    //visible: core.isSeparateReading

                    CText {
                        text: qsTr("Tile height matrix ratio:")
                    }
                    Item {
                        Layout.fillWidth: true
                    }
                    SpinBoxCustom {
                        id: sideScanTileHeightMatrixRatioSpinBox
                        implicitWidth: 200
                        from: 2
                        to: 256
                        stepSize: 1
                        value: 16
                        editable: false

                        onFocusChanged: {
                            sideScanViewSettings.focus = true
                        }
                    }
                }

                RowLayout {
                    CText {
                        text: qsTr("Tile resolution, pix/m:")
                    }
                    Item {
                        Layout.fillWidth: true
                    }
                    SpinBoxCustom {
                        id: sideScanTileResolutionSpinBox
                        implicitWidth: 200
                        from: 1
                        to: 100
                        stepSize: 5
                        value: 10
                        editable: false

                        onFocusChanged: {
                            sideScanViewSettings.focus = true
                        }
                    }
                }

                CButton {
                    text: qsTr("Reinit global mesh")
                    Layout.fillWidth: true
                    Layout.preferredWidth: 200
                    enabled: !core.isMosaicUpdatingInThread && !core.isFileOpening

                    onClicked: {
                        SideScanViewControlMenuController.onGlobalMeshChanged(
                                    sideScanTileSidePixelSizeSpinBox.value, sideScanTileHeightMatrixRatioSpinBox.value, 1 / sideScanTileResolutionSpinBox.value)
                    }

                    onFocusChanged: {
                        sideScanViewSettings.focus = true
                    }
                }
            }
            CheckButton {
                id: sideScanUseLinearFilter
                text: qsTr("Use linear filter")
                Layout.fillWidth: true
                Layout.preferredWidth: 200
                //visible: core.isSeparateReading

                onClicked: {
                    SideScanViewControlMenuController.onUseFilterChanged(checked)
                }

                onFocusChanged: {
                    sideScanViewSettings.focus = true
                }

                Component.onCompleted: {
                    SideScanViewControlMenuController.onUseFilterChanged(checked)
                }

                Settings {
                    property alias sideScanUseLinearFilter: sideScanUseLinearFilter.checked
                }
            }
            CheckButton {
                id: sideScanGridContourVisible
                text: qsTr("Grid/contour visible")
                Layout.fillWidth: true
                Layout.preferredWidth: 200
                checked: false
                //visible: core.isSeparateReading

                onClicked: {
                    SideScanViewControlMenuController.onGridVisibleChanged(checked)
                }

                onFocusChanged: {
                    sideScanViewSettings.focus = true
                }

                Component.onCompleted: {
                    SideScanViewControlMenuController.onGridVisibleChanged(checked)
                }

                Settings {
                    property alias sideScanGridContourVisible: sideScanGridContourVisible.checked
                }
            }
            CheckButton {
                id: sideScanMeasLinesVisible
                text: qsTr("Measuse lines visible")
                Layout.fillWidth: true
                Layout.preferredWidth: 200
                checked: false
                //visible: core.isSeparateReading

                onClicked: {
                    SideScanViewControlMenuController.onMeasLineVisibleChanged(checked)
                }

                onFocusChanged: {
                    sideScanViewSettings.focus = true
                }

                Component.onCompleted: {
                    SideScanViewControlMenuController.onMeasLineVisibleChanged(checked)
                }

                Settings {
                    property alias sideScanMeasLinesVisible: sideScanMeasLinesVisible.checked
                }
            }
            CheckButton {
                id: sideScanGenerateGridContour
                text: qsTr("Generate grid/contour")
                Layout.fillWidth: true
                Layout.preferredWidth: 200
                checked: false
                //visible: core.isSeparateReading

                onClicked: {
                    SideScanViewControlMenuController.onGenerateGridContourChanged(checked)
                }

                onFocusChanged: {
                    sideScanViewSettings.focus = true
                }

                Component.onCompleted: {
                    SideScanViewControlMenuController.onGenerateGridContourChanged(checked)
                }

                Settings {
                    property alias sideScanGenerateGridContour: sideScanGenerateGridContour.checked
                }
            }

            CButton {
                text: qsTr("Clear")
                Layout.fillWidth: true
                Layout.preferredWidth: 200
                //visible: core.isSeparateReading
                enabled: !core.isMosaicUpdatingInThread && !core.isFileOpening

                onClicked: {
                    SideScanViewControlMenuController.onClearClicked()
                }

                onFocusChanged: {
                    sideScanViewSettings.focus = true
                }
            }

            Item {
                Layout.fillHeight: true
            }

            CButton {
                id: updateMosaicButton
                text: qsTr("Update")
                Layout.fillWidth: true
                Layout.preferredWidth: 200
                enabled: !core.isMosaicUpdatingInThread && !core.isFileOpening

                onClicked: {
                    SideScanViewControlMenuController.onUpdateClicked()
                }

                onFocusChanged: {
                    sideScanViewSettings.focus = true
                }
            }
        }

        // levels
        ColumnLayout {
            CText {
                Layout.fillWidth: true
                Layout.topMargin: 0
                Layout.preferredWidth: theme.controlHeight*1.2
                horizontalAlignment: Text.AlignHCenter
                text: sideScanLevelsSlider.stopValue
                small: true
            }

            ChartLevel {
                Layout.fillHeight: true
                Layout.fillWidth: true
                Layout.preferredWidth: theme.controlHeight * 1.2
                id: sideScanLevelsSlider
                Layout.alignment: Qt.AlignHCenter

                onStartValueChanged: {
                   SideScanViewControlMenuController.onLevelChanged(startValue, stopValue);
                }

                onStopValueChanged: {
                   SideScanViewControlMenuController.onLevelChanged(startValue, stopValue);
                }

                Component.onCompleted: {
                    SideScanViewControlMenuController.onLevelChanged(startValue, stopValue);
                }

                Settings {
                    property alias sideScanLevelsStart: sideScanLevelsSlider.startValue
                    property alias sideScanLevelsStop: sideScanLevelsSlider.stopValue
                }
            }

            CText {
                Layout.fillWidth: true
                Layout.preferredWidth: theme.controlHeight * 1.2
                Layout.bottomMargin: 0
                horizontalAlignment: Text.AlignHCenter

                text: sideScanLevelsSlider.startValue
                small: true
            }
        }
    }
}
