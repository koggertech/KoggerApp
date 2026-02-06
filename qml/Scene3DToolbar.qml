import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtQuick.Dialogs
import QtCore


Item  {
    id: toolbarRoot
    anchors.horizontalCenter: parent.horizontalCenter
    anchors.bottom:           parent.bottom
    anchors.bottomMargin:     8

    width: rowButtons.implicitWidth
    height: rowButtons.implicitHeight

    signal updateBottomTrack()

    signal mosaicLAngleOffsetChanged(int val)
    signal mosaicRAngleOffsetChanged(int val)

    function updateMosaic() {
        mosaicViewSettings.updateMosaic()
    }

    Component.onCompleted: {
        mosaicViewSettings.mosaicLAngleOffsetChanged.connect(mosaicLAngleOffsetChanged)
        mosaicViewSettings.mosaicRAngleOffsetChanged.connect(mosaicRAngleOffsetChanged)
    }

    // opacity
    property bool isFitViewCheckButtonHovered: false
    property bool isBoatTrackCheckButtonHovered: false
    property bool isBottomTrackCheckButtonHovered: false
    property alias mosaicEnabled: mosaicViewCheckButton.checked
    property alias showMosaicQualityLabel: settings3DSettings.showQualityLabel

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
    || locationSettings.visible
    || isobathsSettings.visible
    || mosaicViewSettings.visible

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

                CMouseOpacityArea {
                    toolTipText: qsTr("Reset camera")
                    popupPosition: "topRight"
                }

                hoverEnabled: true
                onHoveredChanged: {
                    toolbarRoot.isFitViewCheckButtonHovered = hovered
                }

                onClicked: {
                    Scene3dToolBarController.onSetCameraMapViewButtonClicked()
                }
            }

            Item {
                //visible: false
                id:     locationWrapper
                width : locationCheckButton.implicitWidth
                height: locationCheckButton.implicitHeight

                CheckButton {
                    id: locationCheckButton
                    iconSource: "qrc:/icons/ui/location.svg"
                    backColor:          theme.controlBackColor
                    borderColor:        theme.controlBackColor
                    checkedBorderColor: theme.controlBorderColor
                    checked:            false
                    implicitHeight:     theme.controlHeight * 1.3
                    implicitWidth:      theme.controlHeight * 1.3

                    onCheckedChanged: {
                        Scene3dToolBarController.onTrackLastDataCheckButtonCheckedChanged(checked)
                    }

                    Component.onCompleted: {
                        Scene3dToolBarController.onTrackLastDataCheckButtonCheckedChanged(checked)
                    }

                    property bool locationLongPressTriggered: false

                    MouseArea {
                        id: locationTouchArea
                        anchors.fill: parent
                        enabled: Qt.platform.os === "android"

                        onPressed: {
                            if (enabled) {
                                locationLongPressTimer.start()
                                locationCheckButton.locationLongPressTriggered = false
                            }
                        }

                        onReleased: {
                            if (enabled) {
                                if (!locationCheckButton.locationLongPressTriggered) {
                                    locationCheckButton.checked = !locationCheckButton.checked
                                }
                                locationLongPressTimer.stop()
                            }
                        }

                        onCanceled: {
                            if (enabled) {
                                locationLongPressTimer.stop()
                            }
                        }
                    }

                    Timer {
                        id: locationLongPressTimer
                        interval: 100 // ms
                        repeat: false
                        running : false
                        onTriggered: {
                            locationCheckButton.locationLongPressTriggered = true;
                        }
                    }

                    Settings {
                        property alias locationCheckButton: locationCheckButton.checked
                    }
                }

                LocationExtraSettings {
                    id: locationSettings
                    locationCheckButton:      locationCheckButton

                    anchors.bottom:           locationCheckButton.top
                    anchors.horizontalCenter: locationCheckButton.horizontalCenter
                    z: 2
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

                CMouseOpacityArea {
                    toolTipText: qsTr("Boat track")
                    popupPosition: "topRight"
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
                checked: false
                implicitHeight: theme.controlHeight * 1.3
                implicitWidth: theme.controlHeight * 1.3

                hoverEnabled: true

                property bool pulse: core.dataProcessorState === 1

                CMouseOpacityArea {
                    toolTipText: qsTr("Bottom track")
                    popupPosition: "topRight"
                }

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
                    Scene3dToolBarController.onUpdateBottomTrackCheckButtonCheckedChanged(checked) // calc state
                    BottomTrackControlMenuController.onVisibilityCheckBoxCheckedChanged(checked)

                    if (checked) {
                        toolbarRoot.updateBottomTrack()
                    }
                }

                Component.onCompleted: {
                    Scene3dToolBarController.onUpdateBottomTrackCheckButtonCheckedChanged(checked)
                    BottomTrackControlMenuController.onVisibilityCheckBoxCheckedChanged(checked)
                }

                Settings {
                    //property alias bottomTrackCheckButton: bottomTrackCheckButton.checked
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
                    checked: false
                    implicitHeight: theme.controlHeight * 1.3
                    implicitWidth: theme.controlHeight * 1.3

                    property bool pulse: core.dataProcessorState === 2 || core.dataProcessorState === 4

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
                        if (checked) {
                            toolbarRoot.updateBottomTrack()
                        }

                        IsobathsViewControlMenuController.onProcessStateChanged(checked); // calc state/calc
                        IsobathsViewControlMenuController.onIsobathsVisibilityCheckBoxCheckedChanged(checked)
                    }

                    Component.onCompleted: {
                        IsobathsViewControlMenuController.onIsobathsVisibilityCheckBoxCheckedChanged(checked)
                        IsobathsViewControlMenuController.onProcessStateChanged(checked);
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
                        //property alias isobathsCheckButton: isobathsCheckButton.checked
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
                id: mosaicViewWrapper
                width : mosaicViewCheckButton.implicitWidth
                height: mosaicViewCheckButton.implicitHeight

                CheckButton { // side scan
                    id: mosaicViewCheckButton
                    iconSource: "qrc:/icons/ui/side_scan.svg"
                    backColor: theme.controlBackColor
                    borderColor: theme.controlBackColor
                    checkedBorderColor: theme.controlBorderColor
                    checked: false
                    implicitHeight: theme.controlHeight * 1.3
                    implicitWidth: theme.controlHeight * 1.3

                    property bool pulse: core.dataProcessorState === 3

                    SequentialAnimation {
                        id: pulseMosaicAnimation
                        running: mosaicViewCheckButton.pulse
                        loops: Animation.Infinite
                        NumberAnimation { target: mosaicViewCheckButton; property: "opacity"; to: 0.2; duration: 500 }
                        NumberAnimation { target: mosaicViewCheckButton; property: "opacity"; to: 1.0; duration: 500 }
                    }

                    onPulseChanged: {
                        if (!pulse) {
                            mosaicViewCheckButton.opacity = 1.0;
                        }
                    }

                    onCheckedChanged: {
                        if (checked) {
                            toolbarRoot.updateBottomTrack()
                        }

                        MosaicViewControlMenuController.onUpdateStateChanged(checked) // calc state/calc
                        MosaicViewControlMenuController.onVisibilityChanged(checked)
                    }

                    Component.onCompleted: {
                        MosaicViewControlMenuController.onVisibilityChanged(checked)
                        MosaicViewControlMenuController.onUpdateStateChanged(checked)
                    }

                    property bool mosaicLongPressTriggered: false

                    MouseArea {
                        id: mosaicViewTouchArea
                        anchors.fill: parent
                        enabled: Qt.platform.os === "android"

                        onPressed: {
                            if (enabled) {
                                mosaicViewLongPressTimer.start()
                                mosaicViewCheckButton.mosaicLongPressTriggered = false
                            }
                        }

                        onReleased: {
                            if (enabled) {
                                if (!mosaicViewCheckButton.mosaicLongPressTriggered) {
                                    mosaicViewCheckButton.checked = !mosaicViewCheckButton.checked
                                }
                                mosaicViewLongPressTimer.stop()
                            }
                        }

                        onCanceled: {
                            if (enabled) {
                                mosaicViewLongPressTimer.stop()
                            }
                        }
                    }

                    Timer {
                        id: mosaicViewLongPressTimer
                        interval: 100 // ms
                        repeat: false
                        running: false

                        onTriggered: {
                            mosaicViewCheckButton.mosaicLongPressTriggered = true;
                        }
                    }


                    Settings {
                        //property alias mosaicViewCheckButton: mosaicViewCheckButton.checked
                    }
                }

                MosaicExtraSettings {
                    id: mosaicViewSettings
                    mosaicViewCheckButton: mosaicViewCheckButton

                    anchors.bottom:           mosaicViewCheckButton.top
                    anchors.horizontalCenter: mosaicViewCheckButton.horizontalCenter
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
