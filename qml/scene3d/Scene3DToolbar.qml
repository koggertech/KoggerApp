import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtQuick.Dialogs
import QtCore
import kqml_types 1.0
import "../controls"
import "../menus"


Item  {
    id: toolbarRoot
    anchors.horizontalCenter: parent.horizontalCenter
    anchors.bottom:           parent.bottom
    // Push above the split-drag hit zone so touches near the bottom border
    // resize the pane instead of poking through to toolbar buttons.
    anchors.bottomMargin:     8 + AppPalette.splitHitSizePx / 2

    width: rowButtons.implicitWidth
    height: rowButtons.implicitHeight

    // Mosaic/Isobaths hotkeys now dispatch through WorkspaceStore.applyMosaicHotkey
    // / applyIsobathsHotkey into the App Settings groups (app.mosaic / app.isobaths).
    // MainWindow.handleLegacyHotkey wires them in — see that file for the chain.
    function resetCameraTop()         { Scene3dToolBarController.onSetCameraMapViewButtonClicked() }
    function toggleBottomTrack()      { if (store) store.bottomTrackVisible = !store.bottomTrackVisible }
    function toggleIsobaths()         { if (store) store.isobathsVisible   = !store.isobathsVisible   }
    function toggleMosaic()           { if (store) store.mosaicVisible     = !store.mosaicVisible     }

    // opacity
    property bool isFitViewCheckButtonHovered: false
    property bool isBoatTrackCheckButtonHovered: false
    property bool isBottomTrackCheckButtonHovered: false
    property var view: null
    property var store: null
    readonly property bool showMosaicQualityLabel: false
    property bool toolbarHovered:
        Qt.platform.os === "android" ?
    (   setCameraIsometricView.down
     || boatTrackCheckButton.down
     || bottomTrackCheckButton.down ) :
    (   isBoatTrackCheckButtonHovered
     || isBottomTrackCheckButtonHovered
     || isFitViewCheckButtonHovered )

    opacity: toolbarHovered ? 1.0 : 0.5
    Behavior on opacity { NumberAnimation { duration: 120 } }

    HoverHandler {
        onHoveredChanged: {
            if (hovered && toolbarRoot.view && toolbarRoot.view.resetScenePointerState) {
                toolbarRoot.view.resetScenePointerState()
            }
        }
    }

    ColumnLayout {
        id: column

        // buttons
        RowLayout {
            id: rowButtons
            spacing: 3
            Layout.alignment: Qt.AlignHCenter

            CheckButton {
                id: setCameraIsometricView
                iconSource: "qrc:/icons/ui/fit-in-view.svg"
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

            CheckButton {
                id: geoJsonToolButton
                iconSource: "qrc:/icons/ui/map_pin_cog.svg"
                backColor: theme.controlBackColor
                borderColor: theme.controlBackColor
                checkedBorderColor: theme.controlBorderColor
                checked: false
                implicitHeight: theme.controlHeight * 1.3
                implicitWidth: theme.controlHeight * 1.3
                visible: false

                CMouseOpacityArea {
                    toolTipText: qsTr("GeoJSON")
                    popupPosition: "topRight"
                }

                onToggled: {
                    if (!visible && checked) {
                        // GeoJSON button is hidden; never allow persisted "true" state to activate mode.
                        checked = false
                        return
                    }
                    Scene3dToolBarController.onGeoJsonModeChanged(checked)
                }

                Component.onCompleted: {
                    if (!visible && checked) {
                        checked = false
                    }
                    Scene3dToolBarController.onGeoJsonModeChanged(checked)
                }

                onVisibleChanged: {
                    if (!visible && checked) {
                        checked = false
                    } else if (!visible) {
                        Scene3dToolBarController.onGeoJsonModeChanged(false)
                    }
                }

                Settings {
                    property alias geoJsonToolButton: geoJsonToolButton.checked
                }
            }

            CheckButton {
                id: boatTrackCheckButton
                iconSource: "qrc:/icons/ui/route.svg"
                backColor: theme.controlBackColor
                borderColor: (toolbarRoot.store && toolbarRoot.store.boatTrackVisible)
                             ? theme.controlBorderColor : theme.controlBackColor
                checkedBorderColor: theme.controlBorderColor
                checkable: false
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

                onClicked: {
                    if (toolbarRoot.store)
                        toolbarRoot.store.toggleAppSettingsAtGroup("app.boattrack")
                }
            }

            CheckButton {
                id: bottomTrackCheckButton
                iconSource: "qrc:/icons/ui/double_route.svg"
                backColor: theme.controlBackColor
                borderColor: (toolbarRoot.store && toolbarRoot.store.bottomTrackVisible)
                             ? theme.controlBorderColor : theme.controlBackColor
                checkedBorderColor: theme.controlBorderColor
                checkable: false
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

                onClicked: {
                    if (toolbarRoot.store)
                        toolbarRoot.store.toggleAppSettingsAtGroup("app.bottomtrack")
                }
            }

            CheckButton {
                id: isobathsCheckButton
                iconSource: "qrc:/icons/ui/isobaths.svg"
                backColor: theme.controlBackColor
                borderColor: (toolbarRoot.store && toolbarRoot.store.isobathsVisible)
                             ? theme.controlBorderColor : theme.controlBackColor
                checkedBorderColor: theme.controlBorderColor
                checkable: false
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

                CMouseOpacityArea {
                    toolTipText: qsTr("Isobaths")
                    popupPosition: "topRight"
                }

                onClicked: {
                    if (toolbarRoot.store)
                        toolbarRoot.store.toggleAppSettingsAtGroup("app.isobaths")
                }
            }

            CheckButton {
                id: mosaicViewCheckButton
                iconSource: "qrc:/icons/ui/side_scan.svg"
                backColor: theme.controlBackColor
                borderColor: (toolbarRoot.store && toolbarRoot.store.mosaicVisible)
                             ? theme.controlBorderColor : theme.controlBackColor
                checkedBorderColor: theme.controlBorderColor
                checkable: false
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

                CMouseOpacityArea {
                    toolTipText: qsTr("Mosaic")
                    popupPosition: "topRight"
                }

                onClicked: {
                    if (toolbarRoot.store)
                        toolbarRoot.store.toggleAppSettingsAtGroup("app.mosaic")
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
