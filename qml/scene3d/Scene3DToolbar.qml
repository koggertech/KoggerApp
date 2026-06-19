import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtQuick.Dialogs
import QtCore
import kqml_types 1.0
import controls
import menus


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

    // Width cap for a layer theme-swatch strip (opens RIGHT from the control at
    // ctlX within the row): space from THAT control to the pane edge minus the
    // right toolbar, but never below ~2 swatches (may then slightly overlap).
    function themeStripMaxWidthFor(ctlX) {
        var paneW = (parent && parent.width > 0) ? parent.width : width
        var rightReserve = Math.round(56 * AppPalette.scale)
        var stripLeft = x + rowButtons.x + ctlX
        var available = paneW - rightReserve - stripLeft
        var minForTwo = buttonSize * 3 + Math.round(30 * AppPalette.scale)
        return Math.max(minForTwo, available)
    }

    opacity: toolbarFade.value
    Behavior on opacity { NumberAnimation { duration: 300; easing.type: Easing.OutCubic } }
    IdleFade {
        id: toolbarFade
        hovered: toolbarHover.hovered
                 || boatTrackCtl.menuHovered || bottomTrackCtl.menuHovered
                 || isobathsCtl.menuHovered || mosaicCtl.menuHovered
    }

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

        Scene3DLayerControl {
            id: boatTrackCtl
            buttonSize: toolbarRoot.buttonSize
            Layout.preferredWidth: toolbarRoot.buttonSize
            Layout.preferredHeight: toolbarRoot.buttonSize
            iconSource: "qrc:/icons/ui/route.svg"
            toolTipText: qsTr("Boat track")
            active: toolbarRoot.store ? toolbarRoot.store.boatTrackVisible : false
            onToggleRequested: if (toolbarRoot.store) toolbarRoot.store.boatTrackVisible = !toolbarRoot.store.boatTrackVisible
            onSettingsRequested: if (toolbarRoot.store) toolbarRoot.store.toggleAppSettingsAtGroup("app.boattrack")
            onMenuOpenChanged: if (menuOpen) { bottomTrackCtl.menuOpen = false; isobathsCtl.menuOpen = false; mosaicCtl.menuOpen = false }
        }

        Scene3DLayerControl {
            id: bottomTrackCtl
            buttonSize: toolbarRoot.buttonSize
            Layout.preferredWidth: toolbarRoot.buttonSize
            Layout.preferredHeight: toolbarRoot.buttonSize
            iconSource: "qrc:/icons/ui/double_route.svg"
            toolTipText: qsTr("Bottom track")
            active: toolbarRoot.store ? toolbarRoot.store.bottomTrackVisible : false
            pulse: core.dataProcessorState === 1
            onToggleRequested: if (toolbarRoot.store) toolbarRoot.store.bottomTrackVisible = !toolbarRoot.store.bottomTrackVisible
            onSettingsRequested: if (toolbarRoot.store) toolbarRoot.store.toggleAppSettingsAtGroup("app.bottomtrack")
            onMenuOpenChanged: if (menuOpen) { boatTrackCtl.menuOpen = false; isobathsCtl.menuOpen = false; mosaicCtl.menuOpen = false }
        }

        Scene3DLayerControl {
            id: isobathsCtl
            buttonSize: toolbarRoot.buttonSize
            Layout.preferredWidth: toolbarRoot.buttonSize
            Layout.preferredHeight: toolbarRoot.buttonSize
            iconSource: "qrc:/icons/ui/isobaths.svg"
            toolTipText: qsTr("Isobaths")
            active: toolbarRoot.store ? toolbarRoot.store.isobathsVisible : false
            pulse: core.dataProcessorState === 2 || core.dataProcessorState === 4
            hasTheme: true
            themeCount: 11
            currentThemeId: toolbarRoot.store ? toolbarRoot.store.isobathsThemeIndex : 0
            themeNames: [qsTr("Midnight"), qsTr("Default"), qsTr("Blue"), qsTr("Sepia"), qsTr("Sepia New"), qsTr("WRGBD"), qsTr("WhiteBlack"), qsTr("Standard"), qsTr("DeepBlue"), qsTr("Ice"), qsTr("Green")]
            themeStopsFn: function(id) { return IsobathsViewControlMenuController.themeStops(id) }
            themeStripMaxWidth: toolbarRoot.themeStripMaxWidthFor(isobathsCtl.x)
            onThemePicked: function(index) { if (toolbarRoot.store) toolbarRoot.store.isobathsThemeIndex = index }
            onToggleRequested: if (toolbarRoot.store) toolbarRoot.store.isobathsVisible = !toolbarRoot.store.isobathsVisible
            onSettingsRequested: if (toolbarRoot.store) toolbarRoot.store.toggleAppSettingsAtGroup("app.isobaths")
            onMenuOpenChanged: if (menuOpen) { boatTrackCtl.menuOpen = false; bottomTrackCtl.menuOpen = false; mosaicCtl.menuOpen = false }
        }

        Scene3DLayerControl {
            id: mosaicCtl
            buttonSize: toolbarRoot.buttonSize
            Layout.preferredWidth: toolbarRoot.buttonSize
            Layout.preferredHeight: toolbarRoot.buttonSize
            iconSource: "qrc:/icons/ui/side_scan.svg"
            toolTipText: qsTr("Mosaic")
            active: toolbarRoot.store ? toolbarRoot.store.mosaicVisible : false
            pulse: core.dataProcessorState === 3
            hasTheme: true
            themeCount: 10
            currentThemeId: toolbarRoot.store ? toolbarRoot.store.mosaicThemeIndex : 0
            themeNames: [qsTr("Blue"), qsTr("Sepia"), qsTr("Sepia New"), qsTr("WRGBD"), qsTr("WhiteBlack"), qsTr("BlackWhite"), qsTr("DeepBlue"), qsTr("Ice"), qsTr("Green"), qsTr("Midnight")]
            themeStopsFn: function(id) { return MosaicViewControlMenuController.themeStops(id) }
            themeStripMaxWidth: toolbarRoot.themeStripMaxWidthFor(mosaicCtl.x)
            onThemePicked: function(index) { if (toolbarRoot.store) toolbarRoot.store.mosaicThemeIndex = index }
            onToggleRequested: if (toolbarRoot.store) toolbarRoot.store.mosaicVisible = !toolbarRoot.store.mosaicVisible
            onSettingsRequested: if (toolbarRoot.store) toolbarRoot.store.toggleAppSettingsAtGroup("app.mosaic")
            onMenuOpenChanged: if (menuOpen) { boatTrackCtl.menuOpen = false; bottomTrackCtl.menuOpen = false; isobathsCtl.menuOpen = false }
        }
    }
}
