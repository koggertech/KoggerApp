import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import Qt.labs.settings 1.1
import QtQuick.Dialogs 1.2


// side-scan extra settings
MenuFrame {
    id: mosaicViewSettings

    property CheckButton mosaicViewCheckButton

    function updateMosaic() {
        updateMosaicButton.clicked();
    }

    function setChannelNamesToBackend() {
        //plotDatasetChannelFromStrings(channel1Combo.currentText, channel2Combo.currentText)
        core.setMosaicChannels(channel1Combo.currentText, channel2Combo.currentText);
        //plotCursorChanged(indx, cursorFrom(), cursorTo())
    }

    visible: Qt.platform.os === "android"
             ? (mosaicViewCheckButton.mosaicLongPressTriggered || mosaicTheme.activeFocus || channel1Combo.activeFocus || channel2Combo.activeFocus)
             : (mosaicViewCheckButton.hovered                    ||
                isHovered                                          ||
                mosaicTheme.activeFocus                          ||
                channel1Combo.activeFocus                          ||
                channel2Combo.activeFocus)

    z: mosaicViewSettings.visible
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
            mosaicViewCheckButton.mosaicLongPressTriggered = false
        }
    }

    RowLayout {
        ColumnLayout { // levels
            CText {
                Layout.fillWidth: true
                Layout.topMargin: 0
                Layout.preferredWidth: theme.controlHeight*1.2
                horizontalAlignment: Text.AlignHCenter
                text: mosaicLevelsSlider.stopValue
                small: true
            }
            ChartLevel {
                Layout.fillHeight: true
                Layout.fillWidth: true
                Layout.preferredWidth: theme.controlHeight * 1.2
                id: mosaicLevelsSlider
                Layout.alignment: Qt.AlignHCenter

                onStartValueChanged: {
                   MosaicViewControlMenuController.onLevelChanged(startValue, stopValue);
                }

                onStopValueChanged: {
                   MosaicViewControlMenuController.onLevelChanged(startValue, stopValue);
                }

                Component.onCompleted: {
                    MosaicViewControlMenuController.onLevelChanged(startValue, stopValue);
                }

                Settings {
                    property alias mosaicLevelsStart: mosaicLevelsSlider.startValue
                    property alias mosaicLevelsStop: mosaicLevelsSlider.stopValue
                }
            }
            CText {
                Layout.fillWidth: true
                Layout.preferredWidth: theme.controlHeight * 1.2
                Layout.bottomMargin: 0
                horizontalAlignment: Text.AlignHCenter

                text: mosaicLevelsSlider.startValue
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
                    id: mosaicTheme
                    Layout.preferredWidth: 300
                    model: [qsTr("Blue"), qsTr("Sepia"), qsTr("WRGBD"), qsTr("WhiteBlack"), qsTr("BlackWhite")]
                    currentIndex: 0
                    onCurrentIndexChanged: {
                        MosaicViewControlMenuController.onThemeChanged(currentIndex)
                    }

                    onFocusChanged: {
                        if (Qt.platform.os === 'android') {
                            mosaicViewSettings.focus = true
                        }
                    }

                    Component.onCompleted: {
                        MosaicViewControlMenuController.onThemeChanged(currentIndex)
                    }

                    Settings {
                        property alias mosaicTheme: mosaicTheme.currentIndex
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
                                    mosaicViewSettings.focus = true
                                }
                            }

                            onCurrentTextChanged: {
                                if (suppressTextSignal) {
                                    return
                                }

                                mosaicViewSettings.setChannelNamesToBackend()
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

                                    mosaicViewSettings.setChannelNamesToBackend()

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
                                    mosaicViewSettings.focus = true
                                }
                            }

                            onCurrentTextChanged: {
                                if (suppressTextSignal) {
                                    return
                                }

                                mosaicViewSettings.setChannelNamesToBackend()
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

                                    mosaicViewSettings.setChannelNamesToBackend()

                                    channel2Combo.suppressTextSignal = false
                                }
                            }
                        }
                    }

                    RowLayout {
                        SpinBoxCustom  {
                            id: mosaicLAngleOffset
                            implicitWidth: 200
                            from: -90
                            to: 90
                            stepSize: 1
                            value: 0
                            editable: false

                            onValueChanged: {
                                MosaicViewControlMenuController.onSetLAngleOffset(value)
                            }

                            onFocusChanged: {
                                mosaicViewSettings.focus = true
                            }

                            Component.onCompleted: {
                                MosaicViewControlMenuController.onSetLAngleOffset(value)
                            }

                            Settings {
                                property alias mosaicLAngleOffset: mosaicLAngleOffset.value
                            }
                        }

                        SpinBoxCustom  {
                            id: mosaicRAngleOffset
                            implicitWidth: 200
                            from: -90
                            to: 90
                            stepSize: 1
                            value: 0
                            editable: false

                            onValueChanged: {
                                MosaicViewControlMenuController.onSetRAngleOffset(value)
                            }

                            onFocusChanged: {
                                mosaicViewSettings.focus = true
                            }

                            Component.onCompleted: {
                                MosaicViewControlMenuController.onSetRAngleOffset(value)
                            }

                            Settings {
                                property alias mosaicRAngleOffset: mosaicRAngleOffset.value
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
                    id: mosaicTileResolutionSpinBox
                    implicitWidth: 200
                    from: 1
                    to: 100
                    stepSize: 5
                    value: 10
                    editable: false

                    onFocusChanged: {
                        mosaicViewSettings.focus = true
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
//             id: mosaicTileSidePixelSizeSpinBox
//             implicitWidth: 200
//             from: 32
//             to: 2048
//             stepSize: 1
//             value: 256
//             editable: false

//             onFocusChanged: {
//                 mosaicViewSettings.focus = true
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
//             id: mosaicTileHeightMatrixRatioSpinBox
//             implicitWidth: 200
//             from: 2
//             to: 256
//             stepSize: 1
//             value: 16
//             editable: false

//             onFocusChanged: {
//                 mosaicViewSettings.focus = true
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
//             id: mosaicTileResolutionSpinBox
//             implicitWidth: 200
//             from: 1
//             to: 100
//             stepSize: 5
//             value: 10
//             editable: false

//             onFocusChanged: {
//                 mosaicViewSettings.focus = true
//             }
//         }
//     }

//     CButton {
//         text: qsTr("Reinit global mesh")
//         Layout.fillWidth: true
//         Layout.preferredWidth: 200
//         enabled: !core.isMosaicUpdatingInThread && !core.isFileOpening

//         onClicked: {
//             MosaicViewControlMenuController.onGlobalMeshChanged(
//                         mosaicTileSidePixelSizeSpinBox.value, mosaicTileHeightMatrixRatioSpinBox.value, 1 / mosaicTileResolutionSpinBox.value)
//         }

//         onFocusChanged: {
//             mosaicViewSettings.focus = true
//         }
//     }
// }
// CheckButton {
//     id: mosaicUseLinearFilter
//     text: qsTr("Use linear filter")
//     Layout.fillWidth: true
//     Layout.preferredWidth: 200
//     //visible: core.isSeparateReading

//     onClicked: {
//         MosaicViewControlMenuController.onUseFilterChanged(checked)
//     }

//     onFocusChanged: {
//         mosaicViewSettings.focus = true
//     }

//     Component.onCompleted: {
//         MosaicViewControlMenuController.onUseFilterChanged(checked)
//     }

//     Settings {
//         property alias mosaicUseLinearFilter: mosaicUseLinearFilter.checked
//     }
// }
// CheckButton {
//     id: mosaicGridContourVisible
//     text: qsTr("Grid/contour visible")
//     Layout.fillWidth: true
//     Layout.preferredWidth: 200
//     checked: false
//     //visible: core.isSeparateReading

//     onClicked: {
//         MosaicViewControlMenuController.onGridVisibleChanged(checked)
//     }

//     onFocusChanged: {
//         mosaicViewSettings.focus = true
//     }

//     Component.onCompleted: {
//         MosaicViewControlMenuController.onGridVisibleChanged(checked)
//     }

//     Settings {
//         property alias mosaicGridContourVisible: mosaicGridContourVisible.checked
//     }
// }
// CheckButton {
//     id: mosaicMeasLinesVisible
//     text: qsTr("Measuse lines visible")
//     Layout.fillWidth: true
//     Layout.preferredWidth: 200
//     checked: false
//     //visible: core.isSeparateReading

//     onClicked: {
//         MosaicViewControlMenuController.onMeasLineVisibleChanged(checked)
//     }

//     onFocusChanged: {
//         mosaicViewSettings.focus = true
//     }

//     Component.onCompleted: {
//         MosaicViewControlMenuController.onMeasLineVisibleChanged(checked)
//     }

//     Settings {
//         property alias mosaicMeasLinesVisible: mosaicMeasLinesVisible.checked
//     }
// }
// CheckButton {
//     id: mosaicGenerateGridContour
//     text: qsTr("Generate grid/contour")
//     Layout.fillWidth: true
//     Layout.preferredWidth: 200
//     checked: false
//     //visible: core.isSeparateReading

//     onClicked: {
//         MosaicViewControlMenuController.onGenerateGridContourChanged(checked)
//     }

//     onFocusChanged: {
//         mosaicViewSettings.focus = true
//     }

//     Component.onCompleted: {
//         MosaicViewControlMenuController.onGenerateGridContourChanged(checked)
//     }

//     Settings {
//         property alias mosaicGenerateGridContour: mosaicGenerateGridContour.checked
//     }
// }

// CButton {
//     text: qsTr("Clear")
//     Layout.fillWidth: true
//     Layout.preferredWidth: 200
//     //visible: core.isSeparateReading
//     enabled: !core.isMosaicUpdatingInThread && !core.isFileOpening

//     onClicked: {
//         MosaicViewControlMenuController.onClearClicked()
//     }

//     onFocusChanged: {
//         mosaicViewSettings.focus = true
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
//         MosaicViewControlMenuController.onUpdateClicked()
//     }

//     onFocusChanged: {
//         mosaicViewSettings.focus = true
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
//     text: mosaicLevelsSlider.stopValue
//     small: true
// }

// ChartLevel {
//     Layout.fillHeight: true
//     Layout.fillWidth: true
//     Layout.preferredWidth: theme.controlHeight * 1.2
//     id: mosaicLevelsSlider
//     Layout.alignment: Qt.AlignHCenter

//     onStartValueChanged: {
//        MosaicViewControlMenuController.onLevelChanged(startValue, stopValue);
//     }

//     onStopValueChanged: {
//        MosaicViewControlMenuController.onLevelChanged(startValue, stopValue);
//     }

//     Component.onCompleted: {
//         MosaicViewControlMenuController.onLevelChanged(startValue, stopValue);
//     }

//     Settings {
//         property alias mosaicLevelsStart: mosaicLevelsSlider.startValue
//         property alias mosaicLevelsStop: mosaicLevelsSlider.stopValue
//     }
// }

// CText {
//     Layout.fillWidth: true
//     Layout.preferredWidth: theme.controlHeight * 1.2
//     Layout.bottomMargin: 0
//     horizontalAlignment: Text.AlignHCenter

//     text: mosaicLevelsSlider.startValue
//     small: true
// }
// }
