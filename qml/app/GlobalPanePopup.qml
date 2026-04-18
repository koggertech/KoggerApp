import QtQuick 2.15
import QtQuick.Controls 2.15
import kqml_types 1.0

BasePanePopup {
    id: root

    required property var store
    property var workspaceRoot: null

    popupVisible: store.globalPopupEnabled
    fullscreenMode: store.globalPopupFullscreen
    dragEnabled: !store.globalPopupFullscreen
    popupMargin: store && store.popupMarginPx !== undefined ? store.popupMarginPx : 16

    property bool syncingFromStore: false
    readonly property bool modeSelecting: store.globalPopupModePickerOpen || store.globalPopupMode === ""
    readonly property var paneData: ({
        title: "Global pop-up",
        color: "#0F172A",
        mode: store.globalPopupMode === "3D" ? "3D" : "2D"
    })

    function syncFromStore() {
        if (!popupVisible)
            return

        suspendSignals = true
        syncingFromStore = true
        collapsed = store.globalPopupCollapsed()
        var p = store.globalPopupPosition(popupWidth, popupHeight)
        panelX = clampX(p.x)
        panelY = clampY(p.y)
        syncingFromStore = false
        suspendSignals = false
    }

    function restoreSizeFromStore() {
        var sz = store.globalPopupExpandedSize(0, 0)
        if (sz.width > 80 && sz.height > 80) {
            expandedWidth  = sz.width
            expandedHeight = sz.height
        }
    }

    onPopupVisibleChanged: {
        if (popupVisible) {
            syncFromStore()
            Qt.callLater(syncFromStore)
        } else {
            store.globalPopupFullscreen = false
        }
    }

    onSizeCommitted: function(w, h) {
        if (!popupVisible || syncingFromStore)
            return
        store.setGlobalPopupExpandedSize(w, h)
    }

    Component.onCompleted: {
        restoreSizeFromStore()
        syncFromStore()
        Qt.callLater(syncFromStore)
    }

    onVisibleChanged: {
        if (visible) {
            syncFromStore()
            Qt.callLater(syncFromStore)
        }
    }

    onCollapsedToggled: function(nextCollapsed) {
        if (!popupVisible || syncingFromStore)
            return
        store.setGlobalPopupCollapsed(nextCollapsed)
    }

    onPositionCommitted: function(x, y, w, h) {
        if (!popupVisible || syncingFromStore)
            return
        store.setGlobalPopupPosition(x, y, w, h)
    }

    onCloseRequested: {
        store.globalPopupFullscreen = false
        store.globalPopupEnabled = false
    }

    Connections {
        target: root.store
        ignoreUnknownSignals: true

        function onGlobalPopupStateChanged() {
            root.syncFromStore()
        }

        function onGlobalPopupEnabledChanged() {
            if (root.store.globalPopupEnabled)
                root.syncFromStore()
        }
    }

    PaneContentLoader {
        anchors.fill: parent
        active: root.popupVisible
        visible: !root.modeSelecting
        paneData: root.paneData
        leafId: root.store.globalPopupLeafId
        rotateEnabled: false
        workspaceRoot: root.workspaceRoot
    }

    Rectangle {
        visible: root.modeSelecting
        anchors.fill: parent
        color: "#020617D9"

        Column {
            anchors.centerIn: parent
            spacing: 12

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "Choose pane type"
                color: "#E2E8F0"
                font.pixelSize: 18
                font.bold: true
            }

            Row {
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: 10

                KButton {
                    text: "2D"
                    width: 76
                    height: 40
                    onClicked: root.store.setGlobalPopupMode("2D")
                }

                KButton {
                    readonly property bool canChoose3D: root.store.globalPopupCanChoose3D()
                    text: "3D"
                    width: 76
                    height: 40
                    enabled: canChoose3D
                    opacity: enabled ? 1.0 : 0.45
                    onClicked: root.store.setGlobalPopupMode("3D")
                }
            }

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                visible: !root.store.globalPopupCanChoose3D()
                text: "3D is already used in another pane"
                color: "#C7D2FE"
                font.pixelSize: 12
            }
        }
    }
}
