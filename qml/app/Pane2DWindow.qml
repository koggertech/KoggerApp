import QtQuick 2.15

Item {
    id: root

    property var workspaceRoot: null
    property int leafId: -1
    property var paneData: ({
        title: "",
        color: "transparent",
        mode: "2D"
    })
    property bool rotateEnabled: false
    property int lastRegisteredLeafId: -1

    Item {
        id: hostSurface
        anchors.fill: parent
        clip: true
    }

    PaneInputBridge {
        anchors.fill: parent
        z: -1
        workspaceRoot: root.workspaceRoot
        leafId: root.leafId
        paneKind: "2D"
        active: root.workspaceRoot !== null
                && root.leafId >= 0
                && (!root.workspaceRoot.store
                    || (!root.workspaceRoot.store.editableMode
                        && root.workspaceRoot.store.modePickerLeafId === -1))
    }

    function hostMode() {
        return "2D"
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
