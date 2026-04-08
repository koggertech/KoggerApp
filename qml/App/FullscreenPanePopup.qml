import QtQuick 2.15
import QtQuick.Controls 2.15

BasePanePopup {
    id: root

    required property var store
    property var workspaceRoot: null
    property int hostLeafId: -1
    property int sourceLeafId: -1

    readonly property var paneData: sourceLeafId !== -1 ? store.paneByLeafId(store.layoutTree, sourceLeafId) : null
    readonly property real fixedExpandedWidth: 640
    readonly property real fixedExpandedHeight: 480
    popupVisible: hostLeafId !== -1 && sourceLeafId !== -1 && paneData !== null
    expandedWidth: fixedExpandedWidth
    expandedHeight: fixedExpandedHeight
    popupMargin: store && store.popupMarginPx !== undefined ? store.popupMarginPx : 16
    panelColor: paneData && paneData.color ? paneData.color : "#0B1220"

    property bool syncingFromStore: false

    function syncFromStore() {
        if (!popupVisible)
            return

        suspendSignals = true
        syncingFromStore = true
        expandedWidth = fixedExpandedWidth
        expandedHeight = fixedExpandedHeight
        collapsed = store.popupCollapsedForHost(hostLeafId)
        var p = store.popupPositionForHost(hostLeafId, popupWidth, popupHeight)
        panelX = clampX(p.x)
        panelY = clampY(p.y)
        syncingFromStore = false
        suspendSignals = false
    }

    onPopupVisibleChanged: {
        if (popupVisible) {
            syncFromStore()
            Qt.callLater(syncFromStore)
        }
    }

    Component.onCompleted: {
        syncFromStore()
        Qt.callLater(syncFromStore)
    }

    onHostLeafIdChanged: syncFromStore()
    onSourceLeafIdChanged: syncFromStore()
    onVisibleChanged: {
        if (visible) {
            syncFromStore()
            Qt.callLater(syncFromStore)
        }
    }

    onCollapsedToggled: function(nextCollapsed) {
        if (!popupVisible || syncingFromStore)
            return
        store.setPopupCollapsedForHost(hostLeafId, nextCollapsed)
    }

    onPositionCommitted: function(x, y, w, h) {
        if (!popupVisible || syncingFromStore)
            return
        store.setPopupPositionForHost(hostLeafId, x, y, w, h)
    }

    onPopupDoubleClicked: {
        if (!popupVisible || syncingFromStore)
            return
        if (hostLeafId === -1 || sourceLeafId === -1 || hostLeafId === sourceLeafId)
            return

        // Switch fullscreen focus from the current host pane to the pane shown in popup.
        store.maximizedLeafId = sourceLeafId
        store.activeLeafId = sourceLeafId
    }

    onCloseRequested: {
        if (!popupVisible || syncingFromStore)
            return
        store.setPopupSourceForHost(hostLeafId, -1)
    }

    Connections {
        target: root.store
        ignoreUnknownSignals: true

        function onFullscreenPopupStateByHostChanged() {
            root.syncFromStore()
        }
    }

    PaneContentLoader {
        anchors.fill: parent
        paneData: root.paneData
        leafId: root.sourceLeafId
        rotateEnabled: root.store.paneRotateEnabledByLeafId(root.sourceLeafId)
        workspaceRoot: root.workspaceRoot
        active: root.popupVisible
    }
}
