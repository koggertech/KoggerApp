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

    Scene3DToolbar {
        view: root.scene3dView
        z: 1

        onUpdateBottomTrack: {
            if (root.workspaceRoot)
                root.workspaceRoot.updateBottomTrackForAllPlots()
        }
    }

    Scene3DRightToolbar {
        id: rightToolbar
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        view: root.scene3dView
        geo: root.scene3dView ? root.scene3dView.geoJsonController : null
        z: 1
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
        // BottomTrack actions (when ruler/geojson not active)
        CheckButton {
            checkable: false; iconSource: "qrc:/icons/ui/arrow_bar_to_down.svg"
            backColor: theme.controlBackColor; borderColor: theme.controlBackColor
            checkedBorderColor: theme.controlBorderColor
            implicitHeight: theme.controlHeight * 1.3; implicitWidth: theme.controlHeight * 1.3
            visible: root.scene3dView ? !(root.scene3dView.rulerEnabled || root.scene3dView.rulerSelected || root.scene3dView.geoJsonEnabled) : true
            CMouseOpacityArea { toolTipText: qsTr("Set as min dist"); popupPosition: "topRight" }
            onClicked: { if (root.scene3dView) root.scene3dView.bottomTrackActionEvent(3); contextMenu3D.visible = false }
        }
        CheckButton {
            checkable: false; iconSource: "qrc:/icons/ui/arrow_bar_to_up.svg"
            backColor: theme.controlBackColor; borderColor: theme.controlBackColor
            checkedBorderColor: theme.controlBorderColor
            implicitHeight: theme.controlHeight * 1.3; implicitWidth: theme.controlHeight * 1.3
            visible: root.scene3dView ? !(root.scene3dView.rulerEnabled || root.scene3dView.rulerSelected || root.scene3dView.geoJsonEnabled) : true
            CMouseOpacityArea { toolTipText: qsTr("Set as max dist"); popupPosition: "topRight" }
            onClicked: { if (root.scene3dView) root.scene3dView.bottomTrackActionEvent(2); contextMenu3D.visible = false }
        }
        CheckButton {
            checkable: false; iconSource: "qrc:/icons/ui/eraser.svg"
            backColor: theme.controlBackColor; borderColor: theme.controlBackColor
            checkedBorderColor: theme.controlBorderColor
            implicitHeight: theme.controlHeight * 1.3; implicitWidth: theme.controlHeight * 1.3
            visible: root.scene3dView ? !(root.scene3dView.rulerEnabled || root.scene3dView.rulerSelected || root.scene3dView.geoJsonEnabled) : true
            CMouseOpacityArea { toolTipText: qsTr("Clear dist processing"); popupPosition: "topRight" }
            onClicked: { if (root.scene3dView) root.scene3dView.bottomTrackActionEvent(1); contextMenu3D.visible = false }
        }
        CheckButton {
            checkable: false; iconSource: "qrc:/icons/ui/x.svg"
            backColor: theme.controlBackColor; borderColor: theme.controlBackColor
            checkedBorderColor: theme.controlBorderColor
            implicitHeight: theme.controlHeight * 1.3; implicitWidth: theme.controlHeight * 1.3
            visible: root.scene3dView ? !(root.scene3dView.rulerEnabled || root.scene3dView.rulerSelected || root.scene3dView.geoJsonEnabled) : true
            CMouseOpacityArea { toolTipText: qsTr("Deselect"); popupPosition: "topRight" }
            onClicked: { if (root.scene3dView) root.scene3dView.bottomTrackActionEvent(0); contextMenu3D.visible = false }
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

        onScene3dRightReleased: function(x, y) {
            contextMenu3D.position(x, y)
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
