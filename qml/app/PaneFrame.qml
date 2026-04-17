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
    readonly property color menuPanelColor: "#0F172A"
    readonly property color menuButtonFillColor: "#1E293B"
    readonly property color menuButtonHoverColor: "#172133"
    readonly property color menuButtonPressedColor: "#0B1220"
    readonly property color menuButtonBorderColor: "#334155"
    readonly property color menuButtonHoverBorderColor: "#475569"

    x: isMaximized ? 0 : (paneRect && paneRect.x !== undefined ? paneRect.x : 0)
    y: isMaximized ? 0 : (paneRect && paneRect.y !== undefined ? paneRect.y : 0)
    width: isMaximized ? workspaceItem.width : (paneRect && paneRect.width !== undefined ? paneRect.width : 0)
    height: isMaximized ? workspaceItem.height : (paneRect && paneRect.height !== undefined ? paneRect.height : 0)
    visible: !hiddenByMaximizedPane
    z: isMaximized ? 140 : (isModeSelecting ? 130 : 0)

    Behavior on x { enabled: store.edgeResizeMovingSplitId < 0 && !store.layoutTransitionSuspended; NumberAnimation { duration: 220; easing.type: Easing.InOutCubic } }
    Behavior on y { enabled: store.edgeResizeMovingSplitId < 0 && !store.layoutTransitionSuspended; NumberAnimation { duration: 220; easing.type: Easing.InOutCubic } }
    Behavior on width { enabled: store.edgeResizeMovingSplitId < 0 && !store.layoutTransitionSuspended; NumberAnimation { duration: 220; easing.type: Easing.InOutCubic } }
    Behavior on height { enabled: store.edgeResizeMovingSplitId < 0 && !store.layoutTransitionSuspended; NumberAnimation { duration: 220; easing.type: Easing.InOutCubic } }

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
            border.color: paneItem.isModeSelecting ? "#E2E8F0" : paneItem.paneData.color

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

            SettingsGearButton {
                id: paneModeSettingsButton
                visible: !paneItem.isModeSelecting
                modeTag: paneItem.paneData.mode
                fillColor: paneItem.menuButtonFillColor
                fillHoverColor: paneItem.menuButtonHoverColor
                fillPressedColor: paneItem.menuButtonPressedColor
                borderColor: paneItem.menuButtonBorderColor
                borderHoverColor: paneItem.menuButtonHoverBorderColor
                anchors.top: parent.top
                anchors.right: parent.right
                anchors.topMargin: 8
                anchors.rightMargin: 8
                z: 32
                onClicked: store.openModeSettingsForLeaf(paneItem.leafId)
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
                        color: "#E2E8F0"
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
                enabled: store.modePickerLeafId === -1 && !store.editableMode
                onTapped: store.handleLeafTap(paneItem.leafId)
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
