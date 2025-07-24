import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import Qt.labs.settings 1.1
import QtQuick.Dialogs 1.2


Item  {
    id: toolbarRoot
    anchors.horizontalCenter: parent.horizontalCenter
    anchors.bottom:           parent.bottom
    anchors.bottomMargin:     8

    width: rowButtons.implicitWidth
    height: rowButtons.implicitHeight

    signal updateBottomTrack()

    function updateMosaic() {
        sideScanViewSettings.updateMosaic()
    }

    // opacity
    property bool isFitViewCheckButtonHovered: false
    property bool isBoatTrackCheckButtonHovered: false
    property bool isBottomTrackCheckButtonHovered: false

    property bool toolbarHovered:
        Qt.platform.os === "android" ?
    (   setCameraIsometricView.down
     || boatTrackCheckButton.down
     || bottomTrackCheckButton.down ) :
    (   isBoatTrackCheckButtonHovered
     || isBottomTrackCheckButtonHovered
     || isFitViewCheckButtonHovered )

    property bool menuOpened:
        settings3DSettings.visible
    || isobathsSettings.visible
    || sideScanViewSettings.visible

    opacity: (toolbarHovered || menuOpened) ? 1.0 : 0.5
    Behavior on opacity { NumberAnimation { duration: 120 } }

    ColumnLayout {
        id: column

        // buttons
        RowLayout {
            id: rowButtons
            spacing: 3
            Layout.alignment: Qt.AlignHCenter

            CheckButton {
                id: setCameraIsometricView
                iconSource: "./fit-in-view.svg"
                backColor: theme.controlBackColor
                borderColor: theme.controlBackColor
                checkedBorderColor: theme.controlBorderColor
                checkable: false
                checked: false
                implicitHeight: theme.controlHeight * 1.3
                implicitWidth: theme.controlHeight * 1.3

                hoverEnabled: true
                onHoveredChanged: {
                    toolbarRoot.isFitViewCheckButtonHovered = hovered
                }

                onClicked: {
                    Scene3dToolBarController.onSetCameraMapViewButtonClicked()
                }
            }

            Item {
                id: settings3DWrapper
                width : settings3DCheckButton.implicitWidth
                height: settings3DCheckButton.implicitHeight

                CheckButton {
                    id: settings3DCheckButton
                    iconSource: "qrc:/icons/ui/settings.svg"
                    backColor: theme.controlBackColor
                    borderColor: theme.controlBackColor
                    checkedBorderColor: theme.controlBorderColor
                    checkable: false
                    checked: false
                    implicitHeight: theme.controlHeight * 1.3
                    implicitWidth: theme.controlHeight * 1.3

                    property bool settings3DLongPressTriggered: false

                    MouseArea {
                        id: settings3DTouchArea
                        anchors.fill: parent
                        enabled: Qt.platform.os === "android"

                        onPressed: {
                            if (enabled) {
                                settings3DLongPressTimer.start()
                                settings3DCheckButton.settings3DLongPressTriggered = false
                            }
                        }

                        onReleased: {
                            if (enabled) {
                                settings3DLongPressTimer.stop()
                            }
                        }

                        onCanceled: {
                            if (enabled) {
                                settings3DLongPressTimer.stop()
                            }
                        }
                    }

                    Timer {
                        id: settings3DLongPressTimer
                        interval: 100 // ms
                        repeat: false
                        running : false

                        onTriggered: {
                            settings3DCheckButton.settings3DLongPressTriggered = true;
                        }
                    }
                }

                Settings3DExtraSettings {
                    id: settings3DSettings
                    settings3DCheckButton: settings3DCheckButton
                    anchors.bottom:        settings3DCheckButton.top
                    anchors.horizontalCenter: settings3DCheckButton.horizontalCenter
                    z: 2
                }
            }

            CheckButton {
                id: boatTrackCheckButton
                iconSource: "qrc:/icons/ui/route.svg"
                backColor: theme.controlBackColor
                borderColor: theme.controlBackColor
                checkedBorderColor: theme.controlBorderColor
                checked: true
                implicitHeight: theme.controlHeight * 1.3
                implicitWidth: theme.controlHeight * 1.3

                hoverEnabled: true
                onHoveredChanged: {
                    toolbarRoot.isBoatTrackCheckButtonHovered = hovered
                }

                onCheckedChanged: {
                    BoatTrackControlMenuController.onVisibilityCheckBoxCheckedChanged(checked)
                }

                Component.onCompleted: {
                    BoatTrackControlMenuController.onVisibilityCheckBoxCheckedChanged(checked)
                }

                Settings {
                    property alias boatTrackCheckButton: boatTrackCheckButton.checked
                }
            }

            CheckButton {
                id: bottomTrackCheckButton
                iconSource: "qrc:/icons/ui/double_route.svg"
                backColor: theme.controlBackColor
                borderColor: theme.controlBackColor
                checkedBorderColor: theme.controlBorderColor
                checked: true
                implicitHeight: theme.controlHeight * 1.3
                implicitWidth: theme.controlHeight * 1.3

                hoverEnabled: true

                property bool pulse: core.dataProcessorState === 1

                SequentialAnimation {
                    id: pulseBottomTrackAnimation
                    running: bottomTrackCheckButton.pulse
                    loops: Animation.Infinite
                    NumberAnimation { target: bottomTrackCheckButton; property: "opacity"; to: 0.2; duration: 500 }
                    NumberAnimation { target: bottomTrackCheckButton; property: "opacity"; to: 1.0; duration: 500 }
                }

                onPulseChanged: {
                    if (!pulse) {
                        bottomTrackCheckButton.opacity = 1.0;
                    }
                }

                onHoveredChanged: {
                    toolbarRoot.isBottomTrackCheckButtonHovered = hovered
                }

                onCheckedChanged: {
                    BottomTrackControlMenuController.onVisibilityCheckBoxCheckedChanged(checked)
                    Scene3dToolBarController.onUpdateBottomTrackCheckButtonCheckedChanged(checked)

                    if (checked) {
                        toolbarRoot.updateBottomTrack()
                    }
                }

                Component.onCompleted: {
                    Scene3dToolBarController.onUpdateBottomTrackCheckButtonCheckedChanged(checked)
                    BottomTrackControlMenuController.onVisibilityCheckBoxCheckedChanged(checked)
                }

                Settings {
                    property alias bottomTrackCheckButton: bottomTrackCheckButton.checked
                }
            }

            Item {
                id: isobathsWrapper
                width : isobathsCheckButton.implicitWidth
                height: isobathsCheckButton.implicitHeight

                CheckButton {
                    id: isobathsCheckButton
                    iconSource: "qrc:/icons/ui/isobaths.svg"
                    backColor: theme.controlBackColor
                    borderColor: theme.controlBackColor
                    checkedBorderColor: theme.controlBorderColor
                    checked: true
                    implicitHeight: theme.controlHeight * 1.3
                    implicitWidth: theme.controlHeight * 1.3

                    property bool pulse: core.dataProcessorState === 2

                    SequentialAnimation {
                        id: pulseIsobathsAnimation
                        running: isobathsCheckButton.pulse
                        loops: Animation.Infinite
                        NumberAnimation { target: isobathsCheckButton; property: "opacity"; to: 0.2; duration: 500 }
                        NumberAnimation { target: isobathsCheckButton; property: "opacity"; to: 1.0; duration: 500 }
                    }

                    onPulseChanged: {
                        if (!pulse) {
                            isobathsCheckButton.opacity = 1.0;
                        }
                    }

                    onCheckedChanged: {
                        IsobathsControlMenuController.onIsobathsVisibilityCheckBoxCheckedChanged(checked) // visibility
                        IsobathsControlMenuController.onProcessStateChanged(checked); // calculation state

                        if (checked) {
                            IsobathsControlMenuController.onUpdateIsobathsButtonClicked()
                        }
                    }

                    Component.onCompleted: {
                        IsobathsControlMenuController.onIsobathsVisibilityCheckBoxCheckedChanged(checked)
                        IsobathsControlMenuController.onProcessStateChanged(checked);
                    }

                    property bool isobathsLongPressTriggered: false

                    MouseArea {
                        id: isobathsTouchArea
                        anchors.fill: parent
                        enabled: Qt.platform.os === "android"

                        onPressed: {
                            if (enabled) {
                                isobathsLongPressTimer.start()
                                isobathsCheckButton.isobathsLongPressTriggered = false
                            }
                        }

                        onReleased: {
                            if (enabled) {
                                if (!isobathsCheckButton.isobathsLongPressTriggered) {
                                    isobathsCheckButton.checked = !isobathsCheckButton.checked
                                }
                                isobathsLongPressTimer.stop()
                            }
                        }

                        onCanceled: {
                            if (enabled) {
                                isobathsLongPressTimer.stop()
                            }
                        }
                    }

                    Timer {
                        id: isobathsLongPressTimer
                        interval: 100 // ms
                        repeat: false
                        running : false
                        onTriggered: {
                            isobathsCheckButton.isobathsLongPressTriggered = true;
                        }
                    }

                    Settings {
                        property alias isobathsCheckButton: isobathsCheckButton.checked
                    }
                }

                IsobathsExtraSettings {
                    id: isobathsSettings
                    isobathsCheckButton: isobathsCheckButton

                    anchors.bottom:           isobathsCheckButton.top
                    anchors.horizontalCenter: isobathsCheckButton.horizontalCenter
                    z: 2
                }
            }

            Item {
                id: sideScanViewWrapper
                width : sideScanViewCheckButton.implicitWidth
                height: sideScanViewCheckButton.implicitHeight

                CheckButton { // side scan
                    id: sideScanViewCheckButton
                    iconSource: "qrc:/icons/ui/side_scan.svg"
                    backColor: theme.controlBackColor
                    borderColor: theme.controlBackColor
                    checkedBorderColor: theme.controlBorderColor
                    checked: true
                    implicitHeight: theme.controlHeight * 1.3
                    implicitWidth: theme.controlHeight * 1.3

                    property bool pulse: core.dataProcessorState === 3

                    SequentialAnimation {
                        id: pulseMosaicAnimation
                        running: sideScanViewCheckButton.pulse
                        loops: Animation.Infinite
                        NumberAnimation { target: sideScanViewCheckButton; property: "opacity"; to: 0.2; duration: 500 }
                        NumberAnimation { target: sideScanViewCheckButton; property: "opacity"; to: 1.0; duration: 500 }
                    }

                    onPulseChanged: {
                        if (!pulse) {
                            sideScanViewCheckButton.opacity = 1.0;
                        }
                    }

                    onCheckedChanged: {
                        SideScanViewControlMenuController.onVisibilityChanged(checked)
                    }

                    Component.onCompleted: {
                        SideScanViewControlMenuController.onVisibilityChanged(checked)
                    }

                    property bool sideScanLongPressTriggered: false

                    MouseArea {
                        id: sideScanViewTouchArea
                        anchors.fill: parent
                        enabled: Qt.platform.os === "android"

                        onPressed: {
                            if (enabled) {
                                sideScanViewLongPressTimer.start()
                                sideScanViewCheckButton.sideScanLongPressTriggered = false
                            }
                        }

                        onReleased: {
                            if (enabled) {
                                if (!sideScanViewCheckButton.sideScanLongPressTriggered) {
                                    sideScanViewCheckButton.checked = !sideScanViewCheckButton.checked
                                }
                                sideScanViewLongPressTimer.stop()
                            }
                        }

                        onCanceled: {
                            if (enabled) {
                                sideScanViewLongPressTimer.stop()
                            }
                        }
                    }

                    Timer {
                        id: sideScanViewLongPressTimer
                        interval: 100 // ms
                        repeat: false
                        running: false

                        onTriggered: {
                            sideScanViewCheckButton.sideScanLongPressTriggered = true;
                        }
                    }


                    Settings {
                        property alias sideScanViewCheckButton: sideScanViewCheckButton.checked
                    }
                }

                SideScanExtraSettings {
                    id: sideScanViewSettings
                    sideScanViewCheckButton: sideScanViewCheckButton

                    anchors.bottom:           sideScanViewCheckButton.top
                    anchors.horizontalCenter: sideScanViewCheckButton.horizontalCenter
                    z: 2
                }
            }

            ButtonGroup {
                property bool buttonChangeFlag : false
                id: buttonGroup
                onCheckedButtonChanged: buttonChangeFlag = true
                onClicked: {
                    if (!buttonChangeFlag) {
                        checkedButton = null
                    }

                    buttonChangeFlag = false;
                }
            }
        }
    }
}
