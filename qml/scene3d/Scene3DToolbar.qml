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

    property var view: null
    property var store: null
    property real buttonSize: Math.round(40 * (theme ? theme.resCoeff : 1.0))

    opacity: toolbarFade.value
    Behavior on opacity { NumberAnimation { duration: 300; easing.type: Easing.OutCubic } }
    IdleFade { id: toolbarFade; hovered: toolbarHover.hovered }

    HoverHandler {
        id: toolbarHover
        onHoveredChanged: {
            if (hovered && toolbarRoot.view && toolbarRoot.view.resetScenePointerState) {
                toolbarRoot.view.resetScenePointerState()
            }
        }
    }

    RowLayout {
        id: rowButtons
        spacing: Math.round(6 * AppPalette.scale)
        anchors.horizontalCenter: parent.horizontalCenter

        KCircleIconButton {
            id: resetCamBtn
            width: toolbarRoot.buttonSize
            height: toolbarRoot.buttonSize
            Layout.preferredWidth: toolbarRoot.buttonSize
            Layout.preferredHeight: toolbarRoot.buttonSize
            iconSource: "qrc:/icons/ui/fit-in-view.svg"
            iconTintColor: AppPalette.text
            fillColor: AppPalette.card
            fillHoverColor: AppPalette.cardHover
            borderColor: AppPalette.border
            toolTipText: qsTr("Reset camera")
            onClicked: Scene3dToolBarController.onSetCameraMapViewButtonClicked()
        }

        // Hidden, kept solely for QSettings persistence of the legacy GeoJSON
        // mode flag. UI-wise removed (see qml-scene3d.md).
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

            onToggled: {
                if (!visible && checked) {
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

        KCircleIconButton {
            id: boatTrackBtn
            width: toolbarRoot.buttonSize
            height: toolbarRoot.buttonSize
            Layout.preferredWidth: toolbarRoot.buttonSize
            Layout.preferredHeight: toolbarRoot.buttonSize
            iconSource: "qrc:/icons/ui/route.svg"
            iconTintColor: AppPalette.text
            fillHoverColor: AppPalette.cardHover

            readonly property bool active: toolbarRoot.store ? toolbarRoot.store.boatTrackVisible : false
            fillColor: active ? AppPalette.accentBgStrong : AppPalette.card
            borderColor: active ? AppPalette.accentBorder : AppPalette.border
            borderWidth: active ? 2 : 1

            toolTipText: qsTr("Boat track")
            onClicked: {
                if (toolbarRoot.store)
                    toolbarRoot.store.toggleAppSettingsAtGroup("app.boattrack")
            }
        }

        KCircleIconButton {
            id: bottomTrackBtn
            width: toolbarRoot.buttonSize
            height: toolbarRoot.buttonSize
            Layout.preferredWidth: toolbarRoot.buttonSize
            Layout.preferredHeight: toolbarRoot.buttonSize
            iconSource: "qrc:/icons/ui/double_route.svg"
            iconTintColor: AppPalette.text
            fillHoverColor: AppPalette.cardHover

            readonly property bool active: toolbarRoot.store ? toolbarRoot.store.bottomTrackVisible : false
            fillColor: active ? AppPalette.accentBgStrong : AppPalette.card
            borderColor: active ? AppPalette.accentBorder : AppPalette.border
            borderWidth: active ? 2 : 1

            property bool pulse: core.dataProcessorState === 1
            SequentialAnimation {
                id: pulseBottomTrackAnimation
                running: bottomTrackBtn.pulse
                loops: Animation.Infinite
                NumberAnimation { target: bottomTrackBtn; property: "opacity"; to: 0.2; duration: 500 }
                NumberAnimation { target: bottomTrackBtn; property: "opacity"; to: 1.0; duration: 500 }
            }
            onPulseChanged: { if (!pulse) bottomTrackBtn.opacity = 1.0 }

            toolTipText: qsTr("Bottom track")
            onClicked: {
                if (toolbarRoot.store)
                    toolbarRoot.store.toggleAppSettingsAtGroup("app.bottomtrack")
            }
        }

        KCircleIconButton {
            id: isobathsBtn
            width: toolbarRoot.buttonSize
            height: toolbarRoot.buttonSize
            Layout.preferredWidth: toolbarRoot.buttonSize
            Layout.preferredHeight: toolbarRoot.buttonSize
            iconSource: "qrc:/icons/ui/isobaths.svg"
            iconTintColor: AppPalette.text
            fillHoverColor: AppPalette.cardHover

            readonly property bool active: toolbarRoot.store ? toolbarRoot.store.isobathsVisible : false
            fillColor: active ? AppPalette.accentBgStrong : AppPalette.card
            borderColor: active ? AppPalette.accentBorder : AppPalette.border
            borderWidth: active ? 2 : 1

            property bool pulse: core.dataProcessorState === 2 || core.dataProcessorState === 4
            SequentialAnimation {
                id: pulseIsobathsAnimation
                running: isobathsBtn.pulse
                loops: Animation.Infinite
                NumberAnimation { target: isobathsBtn; property: "opacity"; to: 0.2; duration: 500 }
                NumberAnimation { target: isobathsBtn; property: "opacity"; to: 1.0; duration: 500 }
            }
            onPulseChanged: { if (!pulse) isobathsBtn.opacity = 1.0 }

            toolTipText: qsTr("Isobaths")
            onClicked: {
                if (toolbarRoot.store)
                    toolbarRoot.store.toggleAppSettingsAtGroup("app.isobaths")
            }
        }

        KCircleIconButton {
            id: mosaicBtn
            width: toolbarRoot.buttonSize
            height: toolbarRoot.buttonSize
            Layout.preferredWidth: toolbarRoot.buttonSize
            Layout.preferredHeight: toolbarRoot.buttonSize
            iconSource: "qrc:/icons/ui/side_scan.svg"
            iconTintColor: AppPalette.text
            fillHoverColor: AppPalette.cardHover

            readonly property bool active: toolbarRoot.store ? toolbarRoot.store.mosaicVisible : false
            fillColor: active ? AppPalette.accentBgStrong : AppPalette.card
            borderColor: active ? AppPalette.accentBorder : AppPalette.border
            borderWidth: active ? 2 : 1

            property bool pulse: core.dataProcessorState === 3
            SequentialAnimation {
                id: pulseMosaicAnimation
                running: mosaicBtn.pulse
                loops: Animation.Infinite
                NumberAnimation { target: mosaicBtn; property: "opacity"; to: 0.2; duration: 500 }
                NumberAnimation { target: mosaicBtn; property: "opacity"; to: 1.0; duration: 500 }
            }
            onPulseChanged: { if (!pulse) mosaicBtn.opacity = 1.0 }

            toolTipText: qsTr("Mosaic")
            onClicked: {
                if (toolbarRoot.store)
                    toolbarRoot.store.toggleAppSettingsAtGroup("app.mosaic")
            }
        }
    }
}
