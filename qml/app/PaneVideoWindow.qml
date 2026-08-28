import QtQuick 2.15
import kqml_types 1.0

Item {
    id: root

    property var workspaceRoot: null
    property int leafId: -1
    property var paneData: ({
        title: "",
        color: "transparent",
        mode: "Video"
    })
    property bool rotateEnabled: false
    property string videoContentIdOverride: ""
    property int lastRegisteredLeafId: -1

    readonly property var store: workspaceRoot ? workspaceRoot.store : null
    readonly property string contentId: videoContentIdOverride.length
                                        ? videoContentIdOverride
                                        : (paneData && paneData.contentId ? String(paneData.contentId) : "")

    PaneInputBridge {
        anchors.fill: parent
        z: -1
        workspaceRoot: root.workspaceRoot
        leafId: root.leafId
        paneKind: "Video"
        focusOnPointer: !videoSurfaceItem.selectorOpen
        active: root.workspaceRoot !== null
                && root.leafId >= 0
                && (!root.workspaceRoot.store
                    || (!root.workspaceRoot.store.editableMode
                        && root.workspaceRoot.store.modePickerLeafId === -1))
    }

    VideoSurface {
        id: videoSurfaceItem
        anchors.fill: parent
        store: root.store
        contentId: root.contentId
    }

    Item {
        id: hostSurface
        anchors.fill: parent
        clip: true
    }

    function hostMode() {
        return "Video"
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
