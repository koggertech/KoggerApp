import QtQuick 2.15
import QtQuick.Layouts 1.15
import "../scene3d"
import "../controls"

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
