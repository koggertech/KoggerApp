import QtQuick 2.15
import QtQuick.Layouts 1.15
import WaterFall 1.0
import scene3d
import scene2d
import controls

Item {
    id: root

    property var workspaceRoot: null
    property int leafId: -1
    property var paneData: ({
        title: "",
        color: "transparent",
        mode: "3D"
    })
    property bool rotateEnabled: false
    property int lastRegisteredLeafId: -1

    readonly property var scene3dView: workspaceRoot ? workspaceRoot.scene3dViewItem : null

    Item {
        id: hostSurface
        anchors.fill: parent
        clip: true
    }

    Item {
        id: toolbarLayer
        anchors.fill: parent
        clip: true
        z: 1

        MouseArea {
            anchors.fill: parent
            enabled: scene3dToolbar.anyLayerMenuOpen
            acceptedButtons: Qt.LeftButton
            onPressed: function(mouse) {
                scene3dToolbar.closeLayerMenus()
                mouse.accepted = true
            }
        }

        Scene3DToolbar {
            id: scene3dToolbar
            view: root.scene3dView
            store: root.workspaceRoot ? root.workspaceRoot.store : null
        }

        Scene3DRightToolbar {
            id: rightToolbar
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            view: root.scene3dView
            geo: root.scene3dView ? root.scene3dView.geoJsonController : null
            store: root.workspaceRoot ? root.workspaceRoot.store : null
        }

        // Surface-quality label: right of the dataset toolbar, left of the scale bar.
        Item {
            id: surfaceQualityBadge
            readonly property var v: root.scene3dView
            readonly property var store: root.workspaceRoot ? root.workspaceRoot.store : null
            readonly property int currentZoom: v ? v.dataZoom : -1
            readonly property bool mosaicOn: store ? store.mosaicVisible : false
            readonly property bool surfaceOn: v ? v.updateSurface : false
            // height matrix is shared by surface mesh and mosaic; picture = mosaic
            readonly property bool heightMatrixOn: surfaceOn || mosaicOn
            readonly property int mosaicCmPerPix: currentZoom > 0 ? Math.pow(2, currentZoom - 1) : 0
            readonly property int surfaceCmPerCell: mosaicCmPerPix > 0 ? Math.round(mosaicCmPerPix * 256 / 8) : 0

            visible: v !== null && v.visible && v.cameraPerspective && currentZoom > 0
                     && store && store.showSurfaceQuality && heightMatrixOn
                     && root.workspaceRoot.active3DPane === root

            anchors.left: scene3dToolbar.right
            anchors.leftMargin: Math.round(12 * theme.resCoeff)
            anchors.verticalCenter: scene3dToolbar.verticalCenter

            width: qualityRect.width
            height: qualityRect.height

            Rectangle {
                id: qualityRect
                color: "#00000080"
                radius: Math.round(4 * theme.resCoeff)
                width: qualityText.implicitWidth + Math.round(16 * theme.resCoeff)
                height: qualityText.implicitHeight + Math.round(8 * theme.resCoeff)

                Text {
                    id: qualityText
                    anchors.centerIn: parent
                    text: {
                        var parts = []
                        if (surfaceQualityBadge.heightMatrixOn)
                            parts.push(qsTr("Surface: ") + surfaceQualityBadge.surfaceCmPerCell + qsTr(" cm/cell"))
                        if (surfaceQualityBadge.mosaicOn)
                            parts.push(qsTr("Mosaic: ") + surfaceQualityBadge.mosaicCmPerPix + qsTr(" cm/pix"))
                        return parts.join("\n")
                    }
                    color: "#ffffff"
                    font: theme.textFont
                    horizontalAlignment: Text.AlignLeft
                }
            }
        }
    }

    EchogramContactPopup {
        id: contact3dPopup
        z: 3
        visible: false

        onVisibleChanged: {
            if (!visible) {
                if (accepted) {
                    contacts.setContact(indx, inputFieldText)
                    accepted = false
                }
                info = ""
                inputFieldText = ""
            }
        }
        onDeleteButtonClicked: contacts.deleteContact(indx)
        onCopyButtonClicked: contacts.update()
        onSetActiveButtonClicked: contacts.setActiveContact(indx)
        onInputAccepted: { contact3dPopup.visible = false; contacts.update() }
        onSetButtonClicked: { contact3dPopup.visible = false; contacts.update() }

        Connections {
            target: typeof contacts !== "undefined" ? contacts : null
            ignoreUnknownSignals: true
            function onContactChanged() {
                var show = contacts.contactVisible
                           && root.workspaceRoot && root.workspaceRoot.active3DPane === root
                if (show) {
                    contact3dPopup.indx = contacts.contactIndx
                    contact3dPopup.info = contacts.contactInfo
                    contact3dPopup.inputFieldText = contacts.contactInfo
                    contact3dPopup.lat = contacts.contactLat
                    contact3dPopup.lon = contacts.contactLon
                    contact3dPopup.depth = contacts.contactDepth
                    contact3dPopup.x = contacts.contactPositionX
                    contact3dPopup.y = contacts.contactPositionY
                }
                contact3dPopup.visible = show
            }
        }
    }

    Item {
        id: syncLoupeOverlay
        readonly property var renderer: root.scene3dView
        readonly property var loupeSrc: root.workspaceRoot ? root.workspaceRoot.loupeSourcePlot : null
        property int previewEpochIndex: (loupeSrc && renderer) ? loupeSrc.getPreferredLoupeEpochIndex(renderer.syncLoupeEpochIndex) : -1

        visible: renderer !== null
                 && renderer.visible
                 && root.workspaceRoot && root.workspaceRoot.active3DPane === root
                 && (renderer.syncLoupeOverlayVisible || (renderer.syncLoupeZoomAdjusting && previewEpochIndex >= 0))

        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.rightMargin: Math.round(12 * theme.resCoeff)
        anchors.bottomMargin: Math.round(12 * theme.resCoeff)
        z: 2

        property real sizeMultiplier: (renderer && renderer.syncLoupeSize === 2) ? 1.5 : ((renderer && renderer.syncLoupeSize === 3) ? 2.25 : 1.0)
        property int baseSide: Math.round(180 * theme.resCoeff * sizeMultiplier)
        property int maxSide: renderer ? Math.max(64, Math.min(renderer.width, renderer.height) - 2 * anchors.rightMargin) : 64
        property int side: Math.max(64, Math.min(baseSide, maxSide))
        property int sourceDepthReferencePx: 0

        width: side
        height: side

        function refreshLoupePlot() {
            const previewEpoch = previewEpochIndex
            if (!loupeSrc || !renderer || !visible || previewEpoch < 0)
                return

            const zoomMultiplier = 1.0 + Math.max(0, Math.min(renderer.syncLoupeZoom, 300)) * 0.01
            const previewSourceBaseSize = Math.max(8, Math.floor(side))
            const previewSourceSize = Math.max(4, Math.floor(previewSourceBaseSize / zoomMultiplier))
            const ch1Name = loupeSrc.plotDatasetChannelName()
            const ch2Name = loupeSrc.plotDatasetChannel2Name()
            let mainDepthPxCandidate = loupeSrc.horizontal ? Math.floor(loupeSrc.height) : Math.floor(loupeSrc.width)
            if (mainDepthPxCandidate > 0)
                sourceDepthReferencePx = mainDepthPxCandidate
            if (sourceDepthReferencePx <= 0)
                sourceDepthReferencePx = Math.max(1, Math.floor(syncLoupePlot3D.height))

            const from2D = loupeSrc.cursorFrom()
            const to2D = loupeSrc.cursorTo()
            const has2DRange = isFinite(from2D) && isFinite(to2D) && Math.abs(to2D - from2D) > 0.0001
            const cursorFrom = has2DRange ? from2D : renderer.syncLoupeDepthFrom
            const cursorTo = has2DRange ? to2D : renderer.syncLoupeDepthTo
            const centerDepth = loupeSrc.getLoupeDepthForEpoch(previewEpoch)

            syncLoupePlot3D.horizontal = loupeSrc.horizontal
            syncLoupePlot3D.plotDatasetChannelFromStrings(ch1Name, ch2Name)
            syncLoupePlot3D.plotEchogramTheme(loupeSrc.getThemeId())
            syncLoupePlot3D.plotEchogramSetLevels(loupeSrc.getLowEchogramLevel(), loupeSrc.getHighEchogramLevel())
            syncLoupePlot3D.plotEchogramCompensation(loupeSrc.getEchogramCompensation())
            syncLoupePlot3D.plotBottomTrackVisible(loupeSrc.getBottomTrackVisible())
            syncLoupePlot3D.plotBottomTrackTheme(loupeSrc.getBottomTrackThemeId())
            syncLoupePlot3D.plotRangefinderVisible(loupeSrc.getRangefinderVisible())
            syncLoupePlot3D.plotRangefinderTheme(loupeSrc.getRangefinderThemeId())

            syncLoupePlot3D.setCursorFromTo(cursorFrom, cursorTo)
            syncLoupePlot3D.setTimelinePositionByEpochCentered(previewEpoch)
            syncLoupePlot3D.setZoomPreviewSourceSize(previewSourceSize)
            syncLoupePlot3D.setZoomPreviewReferenceDepthPixels(sourceDepthReferencePx)
            syncLoupePlot3D.setZoomPreviewFlipY(renderer.syncLoupeFlipY)
            syncLoupePlot3D.setZoomPreviewSourceByEpochDepth(previewEpoch, centerDepth)
            syncLoupePlot3D.update()
        }

        onVisibleChanged: {
            if (visible) {
                if (typeof core !== "undefined" && core)
                    core.registerSyncLoupePlot(syncLoupePlot3D)
                refreshLoupePlot()
            }
        }
        onWidthChanged: if (visible) refreshLoupePlot()

        Connections {
            target: syncLoupeOverlay.renderer
            function onSyncLoupeStateChanged() { syncLoupeOverlay.refreshLoupePlot() }
        }
        Connections {
            target: syncLoupeOverlay.loupeSrc
            function onTimelinePositionChanged() { syncLoupeOverlay.refreshLoupePlot() }
            function onEchogramThemeChanged(themeId) { syncLoupeOverlay.refreshLoupePlot() }
        }

        Rectangle {
            id: syncLoupeFrame
            anchors.fill: parent
            color: "black"
            border.color: "#545E84"
            border.width: Math.max(1, Math.round(2 * theme.resCoeff))
            radius: Math.max(1, Math.round(2 * theme.resCoeff))
            clip: true

            WaterFall {
                id: syncLoupePlot3D
                objectName: "syncLoupe3DPlot"
                anchors.fill: parent
                anchors.margins: syncLoupeFrame.border.width
                horizontal: true
                enabled: false

                Component.onCompleted: {
                    setZoomPreviewMode(true)
                    plotAttitudeVisible(false)
                    plotTemperatureVisible(false)
                    plotDopplerBeamVisible(false, 0)
                    plotDopplerInstrumentVisible(false)
                    plotGNSSVisible(false, 0)
                    plotAcousticAngleVisible(false)
                    plotVelocityVisible(false)
                    plotAngleVisibility(false)
                    plotGridVerticalNumber(0)
                    plotGridFillWidth(false)
                    plotGridInvert(false)
                    plotDistanceAutoRange(-1)
                    plotEchogramCompensation(0)
                }
            }
        }
    }

    readonly property bool hasTransientUi: contextMenu3D.visible
                                         || (root.scene3dView !== null
                                             && (root.scene3dView.rulerDrawing || root.scene3dView.rulerEnabled))
                                         || rightToolbar.geometryOpen

    function closeTransientUi() {
        // Context menu wins alone — ephemeral right-click popup.
        if (contextMenu3D.visible) {
            contextMenu3D.visible = false
            return true
        }
        // Tools/panels — ruler reset + geometry collapse together. The
        // map-layer panel lived here too, but moved to AppSettings — no
        // local transient state for it any more.
        var handled = false
        if (root.scene3dView && (root.scene3dView.rulerDrawing || root.scene3dView.rulerEnabled)) {
            if (root.scene3dView.rulerDrawing
                    && typeof root.scene3dView.rulerCancelDrawing === "function") {
                root.scene3dView.rulerCancelDrawing()
            }
            root.scene3dView.rulerEnabled = false
            handled = true
        }
        if (rightToolbar.geometryOpen) {
            rightToolbar.geometryOpen = false
            handled = true
        }
        return handled
    }

    RowLayout {
        id: contextMenu3D
        visible: false
        spacing: 1
        z: 2

        function position(mx, my) {
            var oy = root.height - (my + implicitHeight)
            if (oy < 0) my = my + oy
            if (my < 0) my = 0
            var ox = root.width - (mx + implicitWidth)
            if (ox < 0) mx = mx + ox
            if (mx < 0) mx = 0
            x = mx; y = my
            visible = true
        }

        // Ruler: finish
        CheckButton {
            checkable: false; iconSource: "qrc:/icons/ui/file-check.svg"
            backColor: theme.controlBackColor; borderColor: theme.controlBackColor
            checkedBorderColor: theme.controlBorderColor
            implicitHeight: theme.controlHeight * 1.3; implicitWidth: theme.controlHeight * 1.3
            visible: root.scene3dView ? (root.scene3dView.rulerEnabled && root.scene3dView.rulerDrawing) : false
            CMouseOpacityArea { toolTipText: qsTr("Finish ruler"); popupPosition: "topRight" }
            onClicked: { if (root.scene3dView) root.scene3dView.rulerFinishDrawing(); contextMenu3D.visible = false }
        }
        // Ruler: cancel
        CheckButton {
            checkable: false; iconSource: "qrc:/icons/ui/x.svg"
            backColor: theme.controlBackColor; borderColor: theme.controlBackColor
            checkedBorderColor: theme.controlBorderColor
            implicitHeight: theme.controlHeight * 1.3; implicitWidth: theme.controlHeight * 1.3
            visible: root.scene3dView ? (root.scene3dView.rulerEnabled || root.scene3dView.rulerSelected) : false
            CMouseOpacityArea { toolTipText: qsTr("Cancel ruler"); popupPosition: "topRight" }
            onClicked: {
                if (root.scene3dView && root.scene3dView.rulerDrawing) root.scene3dView.rulerCancelDrawing()
                contextMenu3D.visible = false
            }
        }
        // Ruler: delete selected
        CheckButton {
            checkable: false; iconSource: "qrc:/icons/ui/timeline_event_x.svg"
            backColor: theme.controlBackColor; borderColor: theme.controlBackColor
            checkedBorderColor: theme.controlBorderColor
            implicitHeight: theme.controlHeight * 1.3; implicitWidth: theme.controlHeight * 1.3
            visible: root.scene3dView ? (!root.scene3dView.rulerDrawing && root.scene3dView.rulerSelected) : false
            CMouseOpacityArea { toolTipText: qsTr("Delete ruler"); popupPosition: "topRight" }
            onClicked: { if (root.scene3dView) root.scene3dView.rulerDeleteSelected(); contextMenu3D.visible = false }
        }
    }

    PaneInputBridge {
        id: paneInputBridge
        anchors.fill: parent
        workspaceRoot: root.workspaceRoot
        leafId: root.leafId
        paneKind: "3D"
        active: root.workspaceRoot !== null
                && root.leafId >= 0
                && (!root.workspaceRoot.store
                    || (!root.workspaceRoot.store.editableMode
                        && root.workspaceRoot.store.modePickerLeafId === -1))

        onScene3dRightReleased: function(x, y, wasDrag) {
            var v = root.scene3dView
            // Ruler / GeoJSON keep their own context menu.
            if (v && (v.rulerEnabled || v.rulerSelected || v.geoJsonEnabled)) {
                contextMenu3D.position(x, y)
                return
            }
            // Box-selection finished → apply active tool to selected vertices.
            // unified 2=up→MaxDistProc(2), 3=down→MinDistProc(3), 4=delete→ClearDistProc(1)
            var tool = (typeof core !== "undefined" && core) ? core.bottomTrackEditTool : 0
            if (wasDrag && v && (tool === 2 || tool === 3 || tool === 4))
                v.bottomTrackActionEvent(tool === 4 ? 1 : tool)
        }
    }

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton
        propagateComposedEvents: true
        onPressed: function(mouse) {
            if (contextMenu3D.visible) contextMenu3D.visible = false
            mouse.accepted = false
        }
    }

    function hostMode() {
        return "3D"
    }

    function syncHostRegistration() {
        if (workspaceRoot && typeof workspaceRoot.unregisterPaneHost === "function" && lastRegisteredLeafId !== -1 && lastRegisteredLeafId !== leafId) {
            workspaceRoot.unregisterPaneHost(lastRegisteredLeafId, hostSurface)
            lastRegisteredLeafId = -1
        }

        if (!workspaceRoot || typeof workspaceRoot.registerPaneHost !== "function" || leafId < 0) {
            return
        }

        workspaceRoot.registerPaneHost(leafId, hostSurface, hostMode())
        lastRegisteredLeafId = leafId
    }

    function syncActive3DPane() {
        if (!workspaceRoot) return
        if (typeof workspaceRoot.active3DPane === "undefined") return
        workspaceRoot.active3DPane = root
    }

    Component.onCompleted: { syncHostRegistration(); syncActive3DPane() }
    onWorkspaceRootChanged: { syncHostRegistration(); syncActive3DPane() }
    onLeafIdChanged: syncHostRegistration()

    Component.onDestruction: {
        if (workspaceRoot && typeof workspaceRoot.unregisterPaneHost === "function" && lastRegisteredLeafId !== -1) {
            workspaceRoot.unregisterPaneHost(lastRegisteredLeafId, hostSurface)
        }
        if (workspaceRoot && workspaceRoot.active3DPane === root)
            workspaceRoot.active3DPane = null
    }
}
