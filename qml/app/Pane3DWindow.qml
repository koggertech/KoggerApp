import QtQuick 2.15
import "../scene3d"

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
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        view: root.scene3dView
        geo: root.scene3dView ? root.scene3dView.geoJsonController : null
        z: 1
    }

    PaneInputBridge {
        anchors.fill: parent
        workspaceRoot: root.workspaceRoot
        leafId: root.leafId
        paneKind: "3D"
        active: root.workspaceRoot !== null
                && root.leafId >= 0
                && (!root.workspaceRoot.store
                    || (!root.workspaceRoot.store.editableMode
                        && root.workspaceRoot.store.modePickerLeafId === -1))
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

    Component.onCompleted: syncHostRegistration()
    onWorkspaceRootChanged: syncHostRegistration()
    onLeafIdChanged: syncHostRegistration()

    Component.onDestruction: {
        if (workspaceRoot && typeof workspaceRoot.unregisterPaneHost === "function" && lastRegisteredLeafId !== -1) {
            workspaceRoot.unregisterPaneHost(lastRegisteredLeafId, hostSurface)
        }
    }
}
