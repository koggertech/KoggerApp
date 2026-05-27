import QtQuick 2.15
import QtQuick.Controls 2.15
import kqml_types 1.0

Item {
    id: paneItem

    property var paneRect: ({
        leafId: -1,
        x: 0,
        y: 0,
        width: 0,
        height: 0,
        pane: {
            title: "",
            color: "transparent",
            mode: "2D"
        }
    })
    required property var store
    required property Item workspaceItem

    property int leafId: paneRect && paneRect.leafId !== undefined ? paneRect.leafId : -1
    property var paneData: paneRect && paneRect.pane ? paneRect.pane : ({
        title: "",
        color: "transparent",
        mode: "2D"
    })
    property string hoveredEdge: ""
    property bool canAddPane: store.leafCount() < store.maxPaneCount
    property bool isMaximized: store.maximizedLeafId === paneItem.leafId
    property bool hiddenByMaximizedPane: store.maximizedLeafId !== -1 && !isMaximized
    property bool isModeSelecting: store.isLeafModeSelecting(paneItem.leafId)
    property bool paneRotateEnabled: store.paneRotateEnabledByLeafId(paneItem.leafId)
    property bool popupChooserOpen: false
    property int popupSourceLeafId: store.popupSourceLeafIdForHost(paneItem.leafId)
    property var popupCandidates: store.popupCandidateItemsForHost(paneItem.leafId)
    readonly property int centerQuickIconSize: 136
    readonly property color menuPanelColor: AppPalette.bg
    readonly property color menuButtonFillColor: AppPalette.card
    readonly property color menuButtonHoverColor: AppPalette.cardHover
    readonly property color menuButtonPressedColor: AppPalette.bgDeep
    readonly property color menuButtonBorderColor: AppPalette.border
    readonly property color menuButtonHoverBorderColor: AppPalette.borderHover

    x: isMaximized ? 0 : (paneRect && paneRect.x !== undefined ? paneRect.x : 0)
    y: isMaximized ? 0 : (paneRect && paneRect.y !== undefined ? paneRect.y : 0)
    width: isMaximized ? workspaceItem.width : (paneRect && paneRect.width !== undefined ? paneRect.width : 0)
    height: isMaximized ? workspaceItem.height : (paneRect && paneRect.height !== undefined ? paneRect.height : 0)
    visible: !hiddenByMaximizedPane
    z: isMaximized ? ZOrder.maximizedPane : (isModeSelecting ? ZOrder.maximizingPane : ZOrder.workspacePane)

    Behavior on x { enabled: store.edgeResizeMovingSplitId < 0 && !store.layoutTransitionSuspended; NumberAnimation { duration: Anim.paneResizeMs; easing.type: Anim.paneResizeEasing } }
    Behavior on y { enabled: store.edgeResizeMovingSplitId < 0 && !store.layoutTransitionSuspended; NumberAnimation { duration: Anim.paneResizeMs; easing.type: Anim.paneResizeEasing } }
    Behavior on width { enabled: store.edgeResizeMovingSplitId < 0 && !store.layoutTransitionSuspended; NumberAnimation { duration: Anim.paneResizeMs; easing.type: Anim.paneResizeEasing } }
    Behavior on height { enabled: store.edgeResizeMovingSplitId < 0 && !store.layoutTransitionSuspended; NumberAnimation { duration: Anim.paneResizeMs; easing.type: Anim.paneResizeEasing } }

    onVisibleChanged: {
        if (!visible) {
            popupChooserOpen = false
            hoveredEdge = ""
        }
    }

    onPopupChooserOpenChanged: {
        if (!popupChooserOpen && store.hoveredPopupCandidateLeafId !== -1)
            store.hoveredPopupCandidateLeafId = -1
    }

    Connections {
        target: paneItem.store
        ignoreUnknownSignals: true

        function onEditableModeChanged() {
            if (!paneItem.store.editableMode) {
                paneItem.popupChooserOpen = false
                paneItem.hoveredEdge = ""
            }
        }

        function onModePickerLeafIdChanged() {
            if (paneItem.store.modePickerLeafId !== -1) {
                paneItem.popupChooserOpen = false
                paneItem.hoveredEdge = ""
            }
        }

        function onMaximizedLeafIdChanged() {
            if (paneItem.store.maximizedLeafId !== -1) {
                paneItem.popupChooserOpen = false
                paneItem.hoveredEdge = ""
            }
        }

        function onLayoutTransitionSuspendedChanged() {
            if (paneItem.store.layoutTransitionSuspended)
                paneItem.hoveredEdge = ""
        }
    }

    Rectangle {
            anchors.fill: parent
            anchors.margins: 0
            radius: 0
            color: paneItem.isModeSelecting ? "#111827" : "#09111F"
            border.width: store.editableMode ? 1 : 0
            border.color: paneItem.isModeSelecting ? AppPalette.text : paneItem.paneData.color

            PaneContentLoader {
                anchors.fill: parent
                paneData: paneItem.paneData
                leafId: paneItem.leafId
                rotateEnabled: paneItem.paneRotateEnabled
                workspaceRoot: paneItem.workspaceItem
                visible: !paneItem.isModeSelecting
                active: paneItem.visible && !paneItem.isModeSelecting
                z: 3
            }

            Rectangle {
                id: titleBadge
                visible: !paneItem.isModeSelecting
                anchors.left: parent.left
                anchors.bottom: parent.bottom
                anchors.leftMargin: 8
                anchors.bottomMargin: 8
                height: 28
                width: titleRow.implicitWidth + 12
                radius: 8
                color: paneItem.menuButtonFillColor
                border.width: 1
                border.color: paneItem.menuButtonBorderColor

                Row {
                    id: titleRow
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.leftMargin: 6
                    spacing: 0

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: paneItem.paneData.title
                        color: AppPalette.text
                        font.bold: true
                        font.pixelSize: 14
                    }
                }
            }

            Loader {
                id: paneEditorChromeLoader
                anchors.fill: parent
                z: 40
                active: paneItem.visible
                        && !store.layoutTransitionSuspended
                        && (store.editableMode || paneItem.isModeSelecting || paneItem.popupChooserOpen)
                sourceComponent: PaneEditorChrome {
                    paneFrame: paneItem
                }
            }

            TapHandler {
                id: paneTap
                enabled: store.modePickerLeafId === -1 && !store.editableMode
                gesturePolicy: TapHandler.ReleaseWithinBounds

                // Manual double-tap detection — same recipe as KTapArea but
                // here directly inside the TapHandler that already owns the
                // pane's touch stream (otherwise PaneInputBridge MouseArea
                // never sees these taps under TapHandler's grab).
                property real _lastTapMs: 0
                property point _lastTapPos: Qt.point(-10000, -10000)

                onTapped: function(eventPoint) {
                    store.handleLeafTap(paneItem.leafId)

                    var now = Date.now()
                    var pos = eventPoint && eventPoint.position
                              ? eventPoint.position
                              : Qt.point(0, 0)
                    var dx = pos.x - _lastTapPos.x
                    var dy = pos.y - _lastTapPos.y
                    var distSq = dx * dx + dy * dy
                    var maxDist = AppPalette.doubleTapDistancePx

                    if ((now - _lastTapMs) <= 500 && distSq <= maxDist * maxDist) {
                        _lastTapMs = 0
                        _lastTapPos = Qt.point(-10000, -10000)
                        if (store && typeof store.toggleLeafMaximize === "function") {
                            store.activeLeafId = paneItem.leafId
                            store.toggleLeafMaximize(paneItem.leafId)
                        }
                        return
                    }
                    _lastTapMs = now
                    _lastTapPos = pos
                }
            }
        }

    Rectangle {
        id: hoverHighlight
        anchors.fill: parent
        color: "transparent"
        border.width: 3
        border.color: paneItem.paneData.color
        opacity: store.hoveredPopupCandidateLeafId === paneItem.leafId ? 1.0 : 0.0
        z: 52
        visible: opacity > 0

        Behavior on opacity { NumberAnimation { duration: 120 } }
    }

    Rectangle {
        id: flashOverlay
        anchors.fill: parent
        color: paneItem.paneData.color
        opacity: 0
        z: 53
        visible: opacity > 0

        SequentialAnimation {
            id: flashAnim
            NumberAnimation { target: flashOverlay; property: "opacity"; to: 0.5; duration: 80 }
            NumberAnimation { target: flashOverlay; property: "opacity"; to: 0.0; duration: 500; easing.type: Easing.OutCubic }
            ScriptAction { script: { if (paneItem.store.flashingLeafId === paneItem.leafId) paneItem.store.flashingLeafId = -1 } }
        }
    }

    Connections {
        target: paneItem.store
        function onFlashingLeafIdChanged() {
            if (paneItem.store.flashingLeafId === paneItem.leafId)
                flashAnim.restart()
        }
    }
}
