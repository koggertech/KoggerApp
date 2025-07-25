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

    function setChannelNamesToBackend() {
        //plotDatasetChannelFromStrings(channel1Combo.currentText, channel2Combo.currentText)
        core.setSideScanChannels(channel1Combo.currentText, channel2Combo.currentText);
        //plotCursorChanged(indx, cursorFrom(), cursorTo())
    }

    visible: Qt.platform.os === "android"
             ? (sideScanViewCheckButton.sideScanLongPressTriggered || sideScanTheme.activeFocus || channel1Combo.activeFocus || channel2Combo.activeFocus)
             : (sideScanViewCheckButton.hovered                    ||
                isHovered                                          ||
                sideScanTheme.activeFocus                          ||
                channel1Combo.activeFocus                          ||
                channel2Combo.activeFocus)

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
        ColumnLayout { // levels
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

        ColumnLayout {
            RowLayout {
                CText {
                    text: qsTr("Theme:")
                }
                Item {
                    Layout.fillWidth: true
                }
                CCombo {
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
                ColumnLayout {
                    CText {
                        text: qsTr("Channels:")
                    }

                    CText {
                        text: qsTr("Angle, °:")
                    }
                }

                ColumnLayout {
                    RowLayout {
                        id: rowDataset

                        CCombo  {
                            id: channel1Combo

                            property bool suppressTextSignal: false

                            Layout.fillWidth: true
                            Layout.preferredWidth: rowDataset.width / 3
                            visible: true

                            onFocusChanged: {
                                if (Qt.platform.os === 'android') {
                                    sideScanViewSettings.focus = true
                                }
                            }

                            onCurrentTextChanged: {
                                if (suppressTextSignal) {
                                    return
                                }

                                sideScanViewSettings.setChannelNamesToBackend()
                            }

                            Component.onCompleted: {
                                model = dataset.channelsNameList()

                                let index = model.indexOf(core.ch1Name)
                                if (index >= 0) {
                                    channel1Combo.currentIndex = index
                                }
                            }

                            Connections {
                                target: core
                                function onChannelListUpdated() {
                                    let list = dataset.channelsNameList()

                                    channel1Combo.suppressTextSignal = true

                                    channel1Combo.model = []
                                    channel1Combo.model = list

                                    let newIndex = list.indexOf(core.ch1Name)
                                    if (newIndex >= 0) {
                                        channel1Combo.currentIndex = newIndex
                                    }
                                    else {
                                        channel1Combo.currentIndex = 0
                                    }

                                    sideScanViewSettings.setChannelNamesToBackend()

                                    channel1Combo.suppressTextSignal = false
                                }
                            }
                        }

                        CCombo  {
                            id: channel2Combo

                            property bool suppressTextSignal: false

                            Layout.fillWidth: true
                            Layout.preferredWidth: rowDataset.width / 3
                            visible: true

                            onFocusChanged: {
                                if (Qt.platform.os === 'android') {
                                    sideScanViewSettings.focus = true
                                }
                            }

                            onCurrentTextChanged: {
                                if (suppressTextSignal) {
                                    return
                                }

                                sideScanViewSettings.setChannelNamesToBackend()
                            }


                            Component.onCompleted: {
                                model = dataset.channelsNameList()

                                let index = model.indexOf(core.ch2Name)
                                if (index >= 0) {
                                    channel2Combo.currentIndex = index
                                }
                            }

                            Connections {
                                target: core
                                function onChannelListUpdated() {
                                    let list = dataset.channelsNameList()

                                    channel2Combo.suppressTextSignal = true

                                    channel2Combo.model = []
                                    channel2Combo.model = list

                                    let newIndex = list.indexOf(core.ch2Name)

                                    if (newIndex >= 0) {
                                        channel2Combo.currentIndex = newIndex
                                    }
                                    else {
                                        channel2Combo.currentIndex = 0
                                    }

                                    sideScanViewSettings.setChannelNamesToBackend()

                                    channel2Combo.suppressTextSignal = false
                                }
                            }
                        }
                    }

                    RowLayout {
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

            RowLayout {
                CText {
                    text: qsTr("Resolution, pix/m:")
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

            Item {
                Layout.fillHeight: true
            }
        }
    }
}

// ColumnLayout {
//     RowLayout {
//         //visible: core.isSeparateReading

//         CText {
//             text: qsTr("Tile side pixel size:")
//         }
//         Item {
//             Layout.fillWidth: true
//         }
//         SpinBoxCustom {
//             id: sideScanTileSidePixelSizeSpinBox
//             implicitWidth: 200
//             from: 32
//             to: 2048
//             stepSize: 1
//             value: 256
//             editable: false

//             onFocusChanged: {
//                 sideScanViewSettings.focus = true
//             }
//         }
//     }

//     RowLayout {
//         //visible: core.isSeparateReading

//         CText {
//             text: qsTr("Tile height matrix ratio:")
//         }
//         Item {
//             Layout.fillWidth: true
//         }
//         SpinBoxCustom {
//             id: sideScanTileHeightMatrixRatioSpinBox
//             implicitWidth: 200
//             from: 2
//             to: 256
//             stepSize: 1
//             value: 16
//             editable: false

//             onFocusChanged: {
//                 sideScanViewSettings.focus = true
//             }
//         }
//     }

//     RowLayout {
//         CText {
//             text: qsTr("Tile resolution, pix/m:")
//         }
//         Item {
//             Layout.fillWidth: true
//         }
//         SpinBoxCustom {
//             id: sideScanTileResolutionSpinBox
//             implicitWidth: 200
//             from: 1
//             to: 100
//             stepSize: 5
//             value: 10
//             editable: false

//             onFocusChanged: {
//                 sideScanViewSettings.focus = true
//             }
//         }
//     }

//     CButton {
//         text: qsTr("Reinit global mesh")
//         Layout.fillWidth: true
//         Layout.preferredWidth: 200
//         enabled: !core.isMosaicUpdatingInThread && !core.isFileOpening

//         onClicked: {
//             SideScanViewControlMenuController.onGlobalMeshChanged(
//                         sideScanTileSidePixelSizeSpinBox.value, sideScanTileHeightMatrixRatioSpinBox.value, 1 / sideScanTileResolutionSpinBox.value)
//         }

//         onFocusChanged: {
//             sideScanViewSettings.focus = true
//         }
//     }
// }
// CheckButton {
//     id: sideScanUseLinearFilter
//     text: qsTr("Use linear filter")
//     Layout.fillWidth: true
//     Layout.preferredWidth: 200
//     //visible: core.isSeparateReading

//     onClicked: {
//         SideScanViewControlMenuController.onUseFilterChanged(checked)
//     }

//     onFocusChanged: {
//         sideScanViewSettings.focus = true
//     }

//     Component.onCompleted: {
//         SideScanViewControlMenuController.onUseFilterChanged(checked)
//     }

//     Settings {
//         property alias sideScanUseLinearFilter: sideScanUseLinearFilter.checked
//     }
// }
// CheckButton {
//     id: sideScanGridContourVisible
//     text: qsTr("Grid/contour visible")
//     Layout.fillWidth: true
//     Layout.preferredWidth: 200
//     checked: false
//     //visible: core.isSeparateReading

//     onClicked: {
//         SideScanViewControlMenuController.onGridVisibleChanged(checked)
//     }

//     onFocusChanged: {
//         sideScanViewSettings.focus = true
//     }

//     Component.onCompleted: {
//         SideScanViewControlMenuController.onGridVisibleChanged(checked)
//     }

//     Settings {
//         property alias sideScanGridContourVisible: sideScanGridContourVisible.checked
//     }
// }
// CheckButton {
//     id: sideScanMeasLinesVisible
//     text: qsTr("Measuse lines visible")
//     Layout.fillWidth: true
//     Layout.preferredWidth: 200
//     checked: false
//     //visible: core.isSeparateReading

//     onClicked: {
//         SideScanViewControlMenuController.onMeasLineVisibleChanged(checked)
//     }

//     onFocusChanged: {
//         sideScanViewSettings.focus = true
//     }

//     Component.onCompleted: {
//         SideScanViewControlMenuController.onMeasLineVisibleChanged(checked)
//     }

//     Settings {
//         property alias sideScanMeasLinesVisible: sideScanMeasLinesVisible.checked
//     }
// }
// CheckButton {
//     id: sideScanGenerateGridContour
//     text: qsTr("Generate grid/contour")
//     Layout.fillWidth: true
//     Layout.preferredWidth: 200
//     checked: false
//     //visible: core.isSeparateReading

//     onClicked: {
//         SideScanViewControlMenuController.onGenerateGridContourChanged(checked)
//     }

//     onFocusChanged: {
//         sideScanViewSettings.focus = true
//     }

//     Component.onCompleted: {
//         SideScanViewControlMenuController.onGenerateGridContourChanged(checked)
//     }

//     Settings {
//         property alias sideScanGenerateGridContour: sideScanGenerateGridContour.checked
//     }
// }

// CButton {
//     text: qsTr("Clear")
//     Layout.fillWidth: true
//     Layout.preferredWidth: 200
//     //visible: core.isSeparateReading
//     enabled: !core.isMosaicUpdatingInThread && !core.isFileOpening

//     onClicked: {
//         SideScanViewControlMenuController.onClearClicked()
//     }

//     onFocusChanged: {
//         sideScanViewSettings.focus = true
//     }
// }

// Item {
//     Layout.fillHeight: true
// }

// CButton {
//     id: updateMosaicButton
//     text: qsTr("Update")
//     Layout.fillWidth: true
//     Layout.preferredWidth: 200
//     enabled: !core.isMosaicUpdatingInThread && !core.isFileOpening

//     onClicked: {
//         SideScanViewControlMenuController.onUpdateClicked()
//     }

//     onFocusChanged: {
//         sideScanViewSettings.focus = true
//     }
// }
// }

// // levels
// ColumnLayout {
// CText {
//     Layout.fillWidth: true
//     Layout.topMargin: 0
//     Layout.preferredWidth: theme.controlHeight*1.2
//     horizontalAlignment: Text.AlignHCenter
//     text: sideScanLevelsSlider.stopValue
//     small: true
// }

// ChartLevel {
//     Layout.fillHeight: true
//     Layout.fillWidth: true
//     Layout.preferredWidth: theme.controlHeight * 1.2
//     id: sideScanLevelsSlider
//     Layout.alignment: Qt.AlignHCenter

//     onStartValueChanged: {
//        SideScanViewControlMenuController.onLevelChanged(startValue, stopValue);
//     }

//     onStopValueChanged: {
//        SideScanViewControlMenuController.onLevelChanged(startValue, stopValue);
//     }

//     Component.onCompleted: {
//         SideScanViewControlMenuController.onLevelChanged(startValue, stopValue);
//     }

//     Settings {
//         property alias sideScanLevelsStart: sideScanLevelsSlider.startValue
//         property alias sideScanLevelsStop: sideScanLevelsSlider.stopValue
//     }
// }

// CText {
//     Layout.fillWidth: true
//     Layout.preferredWidth: theme.controlHeight * 1.2
//     Layout.bottomMargin: 0
//     horizontalAlignment: Text.AlignHCenter

//     text: sideScanLevelsSlider.startValue
//     small: true
// }
// }
