import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import kqml_types 1.0

Item {
    id: root

    required property Item paneFrame

    anchors.fill: parent

    Item {
        id: centerQuickActions
        visible: paneFrame.store.editableMode
                 && paneFrame.store.modePickerLeafId === -1
                 && !paneFrame.isModeSelecting
        readonly property real horizontalPadding: 12
        readonly property real verticalPadding: 12
        readonly property real iconSpacing: 14
        readonly property real singleRowWidth: paneFrame.centerQuickIconSize * 3 + iconSpacing * 2
        readonly property real doubleRowWidth: paneFrame.centerQuickIconSize * 2 + iconSpacing
        readonly property real layoutWidth: Math.max(0, width - horizontalPadding * 2)
        readonly property int quickColumns: layoutWidth >= singleRowWidth ? 3 : (layoutWidth >= doubleRowWidth ? 2 : 1)
        readonly property real topSafeInset: editableTopActions.visible ? (editableTopActions.height + 8) : 0
        readonly property real bottomSafeInset: editableBottomActions.visible ? (editableBottomActions.height + 8) : 0
        width: Math.min(parent.width - 20, singleRowWidth + horizontalPadding * 2)
        height: quickActionsGrid.implicitHeight + verticalPadding * 2
        z: 36
        x: (parent.width - width) / 2
        y: {
            var preferred = (parent.height - height) / 2
            var minY = topSafeInset
            var maxY = parent.height - bottomSafeInset - height
            if (maxY < minY)
                return Math.max(0, preferred)
            return Math.max(minY, Math.min(maxY, preferred))
        }

        GridLayout {
            id: quickActionsGrid
            anchors.centerIn: parent
            columns: centerQuickActions.quickColumns
            columnSpacing: centerQuickActions.iconSpacing
            rowSpacing: centerQuickActions.iconSpacing

            CircleIconButton {
                id: paneModeQuickButton
                width: paneFrame.centerQuickIconSize
                height: paneFrame.centerQuickIconSize
                Layout.preferredWidth: paneFrame.centerQuickIconSize
                Layout.preferredHeight: paneFrame.centerQuickIconSize
                rounded: false
                cornerRadius: 0
                borderWidth: 1
                fillColor: paneFrame.menuButtonFillColor
                fillHoverColor: paneFrame.menuButtonHoverColor
                fillPressedColor: paneFrame.menuButtonPressedColor
                borderColor: paneFrame.menuButtonBorderColor
                borderHoverColor: paneFrame.menuButtonHoverBorderColor
                glyph: paneFrame.paneData.mode === "3D" ? "3D" : "2D"
                glyphPixelSize: Math.round(paneFrame.centerQuickIconSize * 0.34)
                glyphColor: paneFrame.paneData.mode === "3D" ? "#86EFAC" : "#93C5FD"
                z: 33
                onClicked: {
                    paneFrame.popupChooserOpen = false
                    paneFrame.store.setModePickerLeafIds([paneFrame.leafId])
                }
            }

            CircleIconButton {
                id: popupAssignButton
                width: paneFrame.centerQuickIconSize
                height: paneFrame.centerQuickIconSize
                Layout.preferredWidth: paneFrame.centerQuickIconSize
                Layout.preferredHeight: paneFrame.centerQuickIconSize
                rounded: false
                cornerRadius: 0
                borderWidth: 1
                fillColor: paneFrame.menuButtonFillColor
                fillHoverColor: paneFrame.menuButtonHoverColor
                fillPressedColor: paneFrame.menuButtonPressedColor
                borderColor: paneFrame.menuButtonBorderColor
                borderHoverColor: paneFrame.menuButtonHoverBorderColor
                iconSource: "qrc:/icons/ui/anchor.svg"
                iconPixelSize: Math.round(paneFrame.centerQuickIconSize * 0.42)
                glyph: "P"
                glyphPixelSize: Math.round(paneFrame.centerQuickIconSize * 0.38)
                glyphColor: paneFrame.popupSourceLeafId !== -1 ? "#FCA5A5" : "#CBD5E1"
                showGlyphWithIcon: false
                z: 33
                onClicked: paneFrame.popupChooserOpen = !paneFrame.popupChooserOpen
            }

            CircleIconButton {
                id: movePaneButton
                width: paneFrame.centerQuickIconSize
                height: paneFrame.centerQuickIconSize
                Layout.preferredWidth: paneFrame.centerQuickIconSize
                Layout.preferredHeight: paneFrame.centerQuickIconSize
                rounded: false
                cornerRadius: 0
                borderWidth: 1
                fillColor: paneFrame.menuButtonFillColor
                fillHoverColor: paneFrame.menuButtonHoverColor
                fillPressedColor: paneFrame.menuButtonPressedColor
                borderColor: paneFrame.menuButtonBorderColor
                borderHoverColor: paneFrame.menuButtonHoverBorderColor
                Layout.columnSpan: centerQuickActions.quickColumns === 2 ? 2 : 1
                Layout.alignment: Qt.AlignHCenter
                iconSource: "qrc:/icons/ui/arrows-cross.svg"
                iconPixelSize: Math.round(paneFrame.centerQuickIconSize * 0.42)
                glyph: "MOVE"
                glyphPixelSize: Math.round(paneFrame.centerQuickIconSize * 0.22)
                glyphColor: "#FDE68A"
                showGlyphWithIcon: false
                cursorShape: pressed ? Qt.ClosedHandCursor : Qt.OpenHandCursor
                z: 33

                onPressStarted: function(mouseX, mouseY) {
                    paneFrame.popupChooserOpen = false
                    paneFrame.store.activeLeafId = paneFrame.leafId
                    paneFrame.store.beginPaneDrag(paneFrame.leafId)
                    var p = movePaneButton.mapToItem(paneFrame.workspaceItem, mouseX, mouseY)
                    paneFrame.store.dragCursor = Qt.point(p.x, p.y)
                    paneFrame.store.updateDropTargetByCursor()
                }

                onPointerMoved: function(mouseX, mouseY) {
                    if (!movePaneButton.pressed)
                        return
                    var p = movePaneButton.mapToItem(paneFrame.workspaceItem, mouseX, mouseY)
                    paneFrame.store.dragCursor = Qt.point(p.x, p.y)
                    paneFrame.store.updateDropTargetByCursor()
                }

                onPressEnded: paneFrame.store.finishPaneDrag()
                onPressCanceled: paneFrame.store.finishPaneDrag()
            }
        }
    }

    Rectangle {
        visible: paneFrame.isModeSelecting
        anchors.fill: parent
        color: "#020617D9"
        z: 70

        MouseArea {
            anchors.fill: parent
        }

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
                    onClicked: paneFrame.store.applyPaneModeSelection(paneFrame.leafId, "2D")
                }

                KButton {
                    readonly property int existing3DLeaf: paneFrame.store.firstLeafIdByMode(paneFrame.store.layoutTree, "3D")
                    readonly property bool canChoose3D: (existing3DLeaf === -1 || existing3DLeaf === paneFrame.leafId)
                                                         && paneFrame.store.globalPopupMode !== "3D"
                    text: "3D"
                    width: 76
                    height: 40
                    enabled: canChoose3D
                    opacity: enabled ? 1.0 : 0.45
                    onClicked: paneFrame.store.applyPaneModeSelection(paneFrame.leafId, "3D")
                }
            }

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                visible: (paneFrame.store.firstLeafIdByMode(paneFrame.store.layoutTree, "3D") !== -1
                         && paneFrame.store.firstLeafIdByMode(paneFrame.store.layoutTree, "3D") !== paneFrame.leafId)
                         || paneFrame.store.globalPopupMode === "3D"
                text: "3D is already used in another pane"
                color: "#C7D2FE"
                font.pixelSize: 12
            }
        }
    }

    Rectangle {
        visible: paneFrame.store.editableMode && paneFrame.popupChooserOpen && paneFrame.store.modePickerLeafId === -1
        anchors.fill: parent
        color: "#02061799"
        z: 72

        MouseArea {
            anchors.fill: parent
            onClicked: paneFrame.popupChooserOpen = false
        }
    }

    Rectangle {
        id: popupChooserPanel
        visible: paneFrame.store.editableMode && paneFrame.popupChooserOpen && paneFrame.store.modePickerLeafId === -1
        z: 73
        width: Math.max(180, Math.min(parent.width - 24, 270))
        height: Math.min(parent.height - 24, chooserColumn.implicitHeight + 16)
        x: (parent.width - width) / 2
        y: (parent.height - height) / 2
        radius: 8
        color: "#0F172A"
        border.width: 1
        border.color: "#475569"

        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.AllButtons
            onPressed: function(mouse) { mouse.accepted = true }
        }

        Column {
            id: chooserColumn
            anchors.fill: parent
            anchors.margins: 8
            spacing: 8

            Text {
                text: "Fullscreen pop-up"
                color: "#E2E8F0"
                font.pixelSize: 13
                font.bold: true
            }

            KButton {
                text: paneFrame.popupSourceLeafId !== -1 ? "Remove pop-up" : "No pop-up"
                onClicked: {
                    paneFrame.store.setPopupSourceForHost(paneFrame.leafId, -1)
                    paneFrame.popupChooserOpen = false
                }
            }

            Text {
                visible: paneFrame.popupCandidates.length === 0
                text: "No neighboring panes available"
                color: "#94A3B8"
                font.pixelSize: 12
            }

            Repeater {
                model: paneFrame.popupCandidates

                delegate: KButton {
                    required property var modelData
                    text: modelData.title + " (" + modelData.mode + ")"
                    onClicked: {
                        paneFrame.store.setPopupSourceForHost(paneFrame.leafId, modelData.leafId)
                        paneFrame.popupChooserOpen = false
                    }
                }
            }
        }
    }

    Rectangle {
        id: editableTopActions
        visible: paneFrame.store.editableMode && paneFrame.store.modePickerLeafId === -1
        width: topActionsRow.implicitWidth + 10
        height: topActionsRow.implicitHeight + 8
        radius: 8
        color: paneFrame.menuPanelColor
        border.width: 1
        border.color: paneFrame.menuButtonBorderColor
        z: 31
        x: (parent.width - width) / 2
        y: 0

        Row {
            id: topActionsRow
            anchors.centerIn: parent
            spacing: paneFrame.canAddPane ? 6 : 0

            ActionChip {
                visible: paneFrame.canAddPane
                chipWidth: paneFrame.canAddPane ? 40 : 0
                chipHeight: paneFrame.canAddPane ? 32 : 0
                label: "+"
                onClicked: paneFrame.store.createPaneInLeaf(paneFrame.leafId, "top")
            }

            ActionChip {
                chipWidth: 40
                chipHeight: 32
                label: "-"
                opacity: paneFrame.store.leafCount() > 1 ? 1.0 : 0.45
                onClicked: {
                    if (paneFrame.store.leafCount() > 1)
                        paneFrame.store.removePane(paneFrame.leafId)
                }
            }
        }
    }

    Rectangle {
        id: editableBottomActions
        visible: paneFrame.store.editableMode && paneFrame.store.modePickerLeafId === -1
        width: bottomActionsRow.implicitWidth + 10
        height: bottomActionsRow.implicitHeight + 8
        radius: 8
        color: paneFrame.menuPanelColor
        border.width: 1
        border.color: paneFrame.menuButtonBorderColor
        z: 31
        x: (parent.width - width) / 2
        y: parent.height - height

        Row {
            id: bottomActionsRow
            anchors.centerIn: parent
            spacing: paneFrame.canAddPane ? 6 : 0

            ActionChip {
                visible: paneFrame.canAddPane
                chipWidth: paneFrame.canAddPane ? 40 : 0
                chipHeight: paneFrame.canAddPane ? 32 : 0
                label: "+"
                onClicked: paneFrame.store.createPaneInLeaf(paneFrame.leafId, "bottom")
            }

            ActionChip {
                chipWidth: 40
                chipHeight: 32
                label: "-"
                opacity: paneFrame.store.leafCount() > 1 ? 1.0 : 0.45
                onClicked: {
                    if (paneFrame.store.leafCount() > 1)
                        paneFrame.store.removePane(paneFrame.leafId)
                }
            }
        }
    }

    Rectangle {
        id: editableLeftActions
        visible: paneFrame.store.editableMode && paneFrame.store.modePickerLeafId === -1
        width: leftActionsColumn.implicitWidth + 6
        height: leftActionsColumn.implicitHeight + 8
        radius: 8
        color: paneFrame.menuPanelColor
        border.width: 1
        border.color: paneFrame.menuButtonBorderColor
        z: 31
        x: 0
        y: (parent.height - height) / 2

        Column {
            id: leftActionsColumn
            anchors.centerIn: parent
            spacing: paneFrame.canAddPane ? 6 : 0

            ActionChip {
                visible: paneFrame.canAddPane
                chipWidth: paneFrame.canAddPane ? 40 : 0
                chipHeight: paneFrame.canAddPane ? 32 : 0
                label: "+"
                onClicked: paneFrame.store.createPaneInLeaf(paneFrame.leafId, "left")
            }

            ActionChip {
                chipWidth: 40
                chipHeight: 32
                label: "-"
                opacity: paneFrame.store.leafCount() > 1 ? 1.0 : 0.45
                onClicked: {
                    if (paneFrame.store.leafCount() > 1)
                        paneFrame.store.removePane(paneFrame.leafId)
                }
            }
        }
    }

    Rectangle {
        id: editableRightActions
        visible: paneFrame.store.editableMode && paneFrame.store.modePickerLeafId === -1
        width: rightActionsColumn.implicitWidth + 6
        height: rightActionsColumn.implicitHeight + 8
        radius: 8
        color: paneFrame.menuPanelColor
        border.width: 1
        border.color: paneFrame.menuButtonBorderColor
        z: 31
        x: parent.width - width
        y: (parent.height - height) / 2

        Column {
            id: rightActionsColumn
            anchors.centerIn: parent
            spacing: paneFrame.canAddPane ? 6 : 0

            ActionChip {
                visible: paneFrame.canAddPane
                chipWidth: paneFrame.canAddPane ? 40 : 0
                chipHeight: paneFrame.canAddPane ? 32 : 0
                label: "+"
                onClicked: paneFrame.store.createPaneInLeaf(paneFrame.leafId, "right")
            }

            ActionChip {
                chipWidth: 40
                chipHeight: 32
                label: "-"
                opacity: paneFrame.store.leafCount() > 1 ? 1.0 : 0.45
                onClicked: {
                    if (paneFrame.store.leafCount() > 1)
                        paneFrame.store.removePane(paneFrame.leafId)
                }
            }
        }
    }

    Timer {
        id: hidePanelTimer
        interval: 120
        repeat: false
        onTriggered: {
            if (!panelMouse.containsMouse)
                paneFrame.hoveredEdge = ""
        }
    }

    MouseArea {
        id: leftEdgeMouse
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: 16
        enabled: paneFrame.store.editableMode && paneFrame.store.modePickerLeafId === -1 && !paneFrame.popupChooserOpen
        acceptedButtons: Qt.NoButton
        hoverEnabled: paneFrame.store.editableMode && paneFrame.store.modePickerLeafId === -1 && !paneFrame.popupChooserOpen
        preventStealing: true
        cursorShape: paneFrame.store.editableMode && paneFrame.store.modePickerLeafId === -1
                     ? (pressed ? Qt.ClosedHandCursor : Qt.SplitHCursor)
                     : Qt.ArrowCursor
        onEntered: {
            hidePanelTimer.stop()
            paneFrame.hoveredEdge = "left"
        }
        onExited: hidePanelTimer.restart()
    }

    MouseArea {
        id: rightEdgeMouse
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: 16
        enabled: paneFrame.store.editableMode && paneFrame.store.modePickerLeafId === -1 && !paneFrame.popupChooserOpen
        acceptedButtons: Qt.NoButton
        hoverEnabled: paneFrame.store.editableMode && paneFrame.store.modePickerLeafId === -1 && !paneFrame.popupChooserOpen
        preventStealing: true
        cursorShape: paneFrame.store.editableMode && paneFrame.store.modePickerLeafId === -1
                     ? (pressed ? Qt.ClosedHandCursor : Qt.SplitHCursor)
                     : Qt.ArrowCursor
        onEntered: {
            hidePanelTimer.stop()
            paneFrame.hoveredEdge = "right"
        }
        onExited: hidePanelTimer.restart()
    }

    MouseArea {
        id: topEdgeMouse
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: 16
        enabled: paneFrame.store.editableMode && paneFrame.store.modePickerLeafId === -1 && !paneFrame.popupChooserOpen
        acceptedButtons: Qt.NoButton
        hoverEnabled: paneFrame.store.editableMode && paneFrame.store.modePickerLeafId === -1 && !paneFrame.popupChooserOpen
        preventStealing: true
        cursorShape: paneFrame.store.editableMode && paneFrame.store.modePickerLeafId === -1
                     ? (pressed ? Qt.ClosedHandCursor : Qt.SplitVCursor)
                     : Qt.ArrowCursor
        onEntered: {
            hidePanelTimer.stop()
            paneFrame.hoveredEdge = "top"
        }
        onExited: hidePanelTimer.restart()
    }

    MouseArea {
        id: bottomEdgeMouse
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 16
        enabled: paneFrame.store.editableMode && paneFrame.store.modePickerLeafId === -1 && !paneFrame.popupChooserOpen
        acceptedButtons: Qt.NoButton
        hoverEnabled: paneFrame.store.editableMode && paneFrame.store.modePickerLeafId === -1 && !paneFrame.popupChooserOpen
        preventStealing: true
        cursorShape: paneFrame.store.editableMode && paneFrame.store.modePickerLeafId === -1
                     ? (pressed ? Qt.ClosedHandCursor : Qt.SplitVCursor)
                     : Qt.ArrowCursor
        onEntered: {
            hidePanelTimer.stop()
            paneFrame.hoveredEdge = "bottom"
        }
        onExited: hidePanelTimer.restart()
    }

    MouseArea {
        id: paneDragArea
        anchors.fill: parent
        z: 20
        enabled: paneFrame.store.editableMode && paneFrame.store.modePickerLeafId === -1 && !paneFrame.popupChooserOpen
        acceptedButtons: Qt.LeftButton
        hoverEnabled: true
        preventStealing: true
        cursorShape: enabled && pressed ? Qt.ClosedHandCursor : Qt.ArrowCursor

        onPressed: function(mouse) {
            paneFrame.store.activeLeafId = paneFrame.leafId
            paneFrame.store.beginPaneDrag(paneFrame.leafId)
            var p = paneDragArea.mapToItem(paneFrame.workspaceItem, mouse.x, mouse.y)
            paneFrame.store.dragCursor = Qt.point(p.x, p.y)
            paneFrame.store.updateDropTargetByCursor()
        }
        onPositionChanged: function(mouse) {
            if (!pressed)
                return
            var p = paneDragArea.mapToItem(paneFrame.workspaceItem, mouse.x, mouse.y)
            paneFrame.store.dragCursor = Qt.point(p.x, p.y)
            paneFrame.store.updateDropTargetByCursor()
        }
        onReleased: paneFrame.store.finishPaneDrag()
        onCanceled: paneFrame.store.finishPaneDrag()
    }

    Rectangle {
        id: edgePanel
        visible: false
        property bool verticalStrip: paneFrame.hoveredEdge === "left" || paneFrame.hoveredEdge === "right"
        property bool canAddPane: paneFrame.store.leafCount() < paneFrame.store.maxPaneCount
        width: verticalStrip ? edgePanelColumn.implicitWidth + 8 : edgePanelRow.implicitWidth + 10
        height: verticalStrip ? edgePanelColumn.implicitHeight + 8 : edgePanelRow.implicitHeight + 8
        radius: 8
        color: paneFrame.menuPanelColor
        border.width: 1
        border.color: paneFrame.menuButtonBorderColor
        z: 30

        x: {
            if (paneFrame.hoveredEdge === "left")
                return 0
            if (paneFrame.hoveredEdge === "right")
                return parent.width - width
            return (parent.width - width) / 2
        }

        y: {
            if (paneFrame.hoveredEdge === "top")
                return 0
            if (paneFrame.hoveredEdge === "bottom")
                return parent.height - height
            return (parent.height - height) / 2
        }

        Row {
            id: edgePanelRow
            visible: !edgePanel.verticalStrip
            anchors.centerIn: parent
            spacing: edgePanel.canAddPane ? 6 : 0

            ActionChip {
                visible: edgePanel.canAddPane
                chipWidth: edgePanel.canAddPane ? 30 : 0
                chipHeight: edgePanel.canAddPane ? 24 : 0
                label: "+"
                onClicked: paneFrame.store.createPaneInLeaf(paneFrame.leafId, paneFrame.hoveredEdge)
            }

            ActionChip {
                label: "-"
                opacity: paneFrame.store.leafCount() > 1 ? 1.0 : 0.45
                onClicked: {
                    if (paneFrame.store.leafCount() > 1)
                        paneFrame.store.removePane(paneFrame.leafId)
                }
            }
        }

        Column {
            id: edgePanelColumn
            visible: edgePanel.verticalStrip
            anchors.centerIn: parent
            spacing: edgePanel.canAddPane ? 6 : 0

            ActionChip {
                visible: edgePanel.canAddPane
                chipWidth: edgePanel.canAddPane ? 30 : 0
                chipHeight: edgePanel.canAddPane ? 24 : 0
                label: "+"
                onClicked: paneFrame.store.createPaneInLeaf(paneFrame.leafId, paneFrame.hoveredEdge)
            }

            ActionChip {
                label: "-"
                opacity: paneFrame.store.leafCount() > 1 ? 1.0 : 0.45
                onClicked: {
                    if (paneFrame.store.leafCount() > 1)
                        paneFrame.store.removePane(paneFrame.leafId)
                }
            }
        }

        MouseArea {
            id: panelMouse
            anchors.fill: parent
            acceptedButtons: Qt.NoButton
            hoverEnabled: true
            onExited: hidePanelTimer.restart()
        }
    }

    Rectangle {
        anchors.fill: parent
        color: paneFrame.store.dropTargetLeafId === paneFrame.leafId ? "#DC262622" : "transparent"
        border.width: paneFrame.store.dropTargetLeafId === paneFrame.leafId ? 6 : 0
        border.color: "#EF4444"
        z: 40
    }
}
