import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtQuick.Dialogs
import QtCore


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
             : (mosaicViewCheckButton.hovered                  ||
                isHovered                                      ||
                mosaicTheme.activeFocus                        ||
                channel1Combo.activeFocus                      ||
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

    ColumnLayout {
        RowLayout {
            spacing: 16

            Rectangle {
                Layout.fillWidth: true
                height: 2
                color: "#808080"
            }

            CText {
                text: qsTr("Mosaic settings")
            }

            Rectangle {
                Layout.fillWidth: true
                height: 2
                color: "#808080"
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
                        Layout.preferredWidth: 200

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
                    CText {
                        text: qsTr("Channels:")
                    }
                    Item {
                        Layout.fillWidth: true
                    }
                    ColumnLayout {
                        id: rowDataset

                        implicitWidth: 200

                        CCombo  {
                            id: channel1Combo
                            Layout.preferredWidth: 200

                            property bool suppressTextSignal: false
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

                            Layout.preferredWidth: 200
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
                }

                RowLayout {
                    CText {
                        text: qsTr("Angle, °:")
                    }
                    Item {
                        Layout.fillWidth: true
                    }
                    ColumnLayout {
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
                Item {
                    Layout.fillHeight: true
                }
                RowLayout {
                    CText {
                        text: qsTr("Res., px/m:")
                    }
                    Item {
                        Layout.fillWidth: true
                    }
                    SpinBoxCustom {
                        id: mosaicResolutionSpinBox
                        implicitWidth: 200
                        from: 5
                        to: 100
                        stepSize: 5
                        value: 10
                        editable: false

                        onFocusChanged: {
                            mosaicViewSettings.focus = true
                        }

                        onValueChanged: {
                            MosaicViewControlMenuController.onSetResolution(value)
                        }
                        Component.onCompleted: {
                            MosaicViewControlMenuController.onSetResolution(value)
                        }

                        Settings {
                            property alias mosaicResolutionSpinBox: mosaicResolutionSpinBox.value
                        }
                    }
                }
            }
        }
    }
}
