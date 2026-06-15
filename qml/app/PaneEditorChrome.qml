import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import kqml_types 1.0

Item {
    id: root

    required property Item paneFrame

    readonly property real addInset: paneFrame.edgeSafetyMargin + Math.round(8 * AppPalette.scale)
    readonly property string addHoverSide: editableTopActions.hovered ? "top"
                                           : editableBottomActions.hovered ? "bottom"
                                           : editableLeftActions.hovered ? "left"
                                           : editableRightActions.hovered ? "right" : ""
    readonly property real chooserRowHeight: Math.round(34 * AppPalette.scale)

    anchors.fill: parent

    component ChooserRow: Rectangle {
        id: crow
        property string label: ""
        property color dotColor: "transparent"
        property bool selected: false
        readonly property bool hovered: crowMouse.containsMouse
        signal activated()

        height: root.chooserRowHeight
        radius: Tokens.radiusMd
        color: crowMouse.containsMouse ? paneFrame.menuButtonHoverColor
               : (crow.selected ? AppPalette.accentBg : paneFrame.menuButtonFillColor)
        border.width: 1
        border.color: crowMouse.containsMouse ? paneFrame.menuButtonHoverBorderColor
                                              : paneFrame.menuButtonBorderColor

        Rectangle {
            id: crowDot
            visible: crow.dotColor.a > 0
            anchors.left: parent.left
            anchors.leftMargin: Tokens.spaceMd
            anchors.verticalCenter: parent.verticalCenter
            width: Math.round(10 * AppPalette.scale)
            height: width
            radius: Math.round(3 * AppPalette.scale)
            color: crow.dotColor
        }

        Text {
            anchors.left: crowDot.visible ? crowDot.right : parent.left
            anchors.leftMargin: Tokens.spaceMd
            anchors.right: parent.right
            anchors.rightMargin: Tokens.spaceMd
            anchors.verticalCenter: parent.verticalCenter
            text: crow.label
            color: AppPalette.text
            font.pixelSize: Tokens.fontBase
            elide: Text.ElideRight
            verticalAlignment: Text.AlignVCenter
        }

        MouseArea {
            id: crowMouse
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: crow.activated()
        }
    }

    Item {
        id: centerQuickActions
        visible: paneFrame.store.editableMode
                 && paneFrame.store.modePickerLeafId === -1
                 && !paneFrame.isModeSelecting
        readonly property real horizontalPadding: Math.round(12 * AppPalette.scale)
        readonly property real verticalPadding: Math.round(12 * AppPalette.scale)
        readonly property real iconSpacing: Math.round(10 * AppPalette.scale)
        readonly property real quadRowWidth: paneFrame.centerQuickIconSize * 4 + iconSpacing * 3
        readonly property real doubleRowWidth: paneFrame.centerQuickIconSize * 2 + iconSpacing
        readonly property real sideReserve: root.addInset + paneFrame.edgeAddButtonSize + Math.round(12 * AppPalette.scale)
        readonly property real layoutWidth: Math.max(0, width - horizontalPadding * 2)
        readonly property int quickColumns: layoutWidth >= quadRowWidth ? 4 : (layoutWidth >= doubleRowWidth ? 2 : 1)
        readonly property real topSafeInset: editableTopActions.visible ? (editableTopActions.height + 8) : 0
        readonly property real bottomSafeInset: editableBottomActions.visible ? (editableBottomActions.height + 8) : 0
        width: Math.min(parent.width - 2 * sideReserve, quadRowWidth + horizontalPadding * 2)
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

            KCircleIconButton {
                id: paneModeQuickButton
                width: paneFrame.centerQuickIconSize
                height: paneFrame.centerQuickIconSize
                Layout.preferredWidth: paneFrame.centerQuickIconSize
                Layout.preferredHeight: paneFrame.centerQuickIconSize
                borderWidth: 1
                fillColor: paneFrame.menuButtonFillColor
                fillHoverColor: paneFrame.menuButtonHoverColor
                fillPressedColor: paneFrame.menuButtonPressedColor
                borderColor: paneFrame.menuButtonBorderColor
                borderHoverColor: paneFrame.menuButtonHoverBorderColor
                glyph: paneFrame.paneData.mode === "3D" ? "3D" : "2D"
                glyphPixelSize: Math.round(paneFrame.centerQuickIconSize * 0.34)
                glyphColor: AppPalette.accentBorder
                z: 33
                onClicked: {
                    paneFrame.popupChooserOpen = false
                    paneFrame.store.setModePickerLeafIds([paneFrame.leafId])
                }
            }

            KCircleIconButton {
                id: popupAssignButton
                width: paneFrame.centerQuickIconSize
                height: paneFrame.centerQuickIconSize
                Layout.preferredWidth: paneFrame.centerQuickIconSize
                Layout.preferredHeight: paneFrame.centerQuickIconSize
                borderWidth: 1
                fillColor: paneFrame.menuButtonFillColor
                fillHoverColor: paneFrame.menuButtonHoverColor
                fillPressedColor: paneFrame.menuButtonPressedColor
                borderColor: paneFrame.menuButtonBorderColor
                borderHoverColor: paneFrame.menuButtonHoverBorderColor
                iconSource: "qrc:/icons/ui/anchor.svg"
                iconTintColor: paneFrame.popupSourceLeafId !== -1 ? AppPalette.dangerText : AppPalette.textSecond
                iconPixelSize: Math.round(paneFrame.centerQuickIconSize * 0.42)
                toolTipText: qsTr("Pop-up link")
                z: 33
                onClicked: paneFrame.popupChooserOpen = !paneFrame.popupChooserOpen
            }

            KCircleIconButton {
                id: movePaneButton
                width: paneFrame.centerQuickIconSize
                height: paneFrame.centerQuickIconSize
                Layout.preferredWidth: paneFrame.centerQuickIconSize
                Layout.preferredHeight: paneFrame.centerQuickIconSize
                borderWidth: 1
                fillColor: paneFrame.menuButtonFillColor
                fillHoverColor: paneFrame.menuButtonHoverColor
                fillPressedColor: paneFrame.menuButtonPressedColor
                borderColor: paneFrame.menuButtonBorderColor
                borderHoverColor: paneFrame.menuButtonHoverBorderColor
                iconSource: "qrc:/icons/ui/arrows-cross.svg"
                iconTintColor: AppPalette.textSecond
                iconPixelSize: Math.round(paneFrame.centerQuickIconSize * 0.42)
                toolTipText: qsTr("Move pane")
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

            KCircleIconButton {
                id: deletePaneButton
                width: paneFrame.centerQuickIconSize
                height: paneFrame.centerQuickIconSize
                Layout.preferredWidth: paneFrame.centerQuickIconSize
                Layout.preferredHeight: paneFrame.centerQuickIconSize
                borderWidth: 1
                fillColor: paneFrame.menuButtonFillColor
                fillHoverColor: paneFrame.menuButtonHoverColor
                fillPressedColor: paneFrame.menuButtonPressedColor
                borderColor: paneFrame.menuButtonBorderColor
                borderHoverColor: paneFrame.menuButtonHoverBorderColor
                iconSource: "qrc:/icons/ui/x.svg"
                iconTintColor: AppPalette.dangerText
                iconPixelSize: Math.round(paneFrame.centerQuickIconSize * 0.42)
                enabled: paneFrame.store.leafCount() > 1
                opacity: enabled ? 1.0 : 0.45
                toolTipText: qsTr("Remove pane")
                z: 33
                onClicked: {
                    if (paneFrame.store.leafCount() > 1)
                        paneFrame.store.removePane(paneFrame.leafId)
                }
            }
        }
    }

    Rectangle {
        id: deleteHoverOverlay
        anchors.fill: parent
        color: AppPalette.dangerText
        opacity: (paneFrame.store.editableMode && deletePaneButton.hovered) ? 0.18 : 0.0
        visible: opacity > 0.001
        z: 25
        Behavior on opacity { NumberAnimation { duration: 120; easing.type: Easing.OutCubic } }
    }

    Rectangle {
        id: addPreviewOverlay
        visible: root.addHoverSide !== ""
        z: 27
        color: AppPalette.accentBorder
        opacity: root.addHoverSide !== "" ? 0.22 : 0.0
        border.width: Math.max(2, Math.round(2 * AppPalette.scale))
        border.color: AppPalette.accentBorder
        x: root.addHoverSide === "right" ? parent.width / 2 : 0
        y: root.addHoverSide === "bottom" ? parent.height / 2 : 0
        width: (root.addHoverSide === "left" || root.addHoverSide === "right") ? parent.width / 2 : parent.width
        height: (root.addHoverSide === "top" || root.addHoverSide === "bottom") ? parent.height / 2 : parent.height
        Behavior on opacity { NumberAnimation { duration: 120; easing.type: Easing.OutCubic } }
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
            spacing: Math.round(12 * AppPalette.scale)

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Choose pane type")
                color: AppPalette.text
                font.pixelSize: Math.round(18 * AppPalette.scale)
                font.bold: true
            }

            Row {
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: Math.round(10 * AppPalette.scale)

                KButton {
                    text: qsTr("2D")
                    width: Math.max(Math.round(76 * AppPalette.scale), implicitWidth)
                    height: Math.max(Math.round(40 * AppPalette.scale), implicitHeight)
                    onClicked: paneFrame.store.applyPaneModeSelection(paneFrame.leafId, "2D")
                }

                KButton {
                    readonly property int existing3DLeaf: paneFrame.store.firstLeafIdByMode(paneFrame.store.layoutTree, "3D")
                    readonly property bool canChoose3D: (existing3DLeaf === -1 || existing3DLeaf === paneFrame.leafId)
                                                         && paneFrame.store.globalPopupMode !== "3D"
                    text: qsTr("3D")
                    width: Math.max(Math.round(76 * AppPalette.scale), implicitWidth)
                    height: Math.max(Math.round(40 * AppPalette.scale), implicitHeight)
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
                text: qsTr("3D is already used in another pane")
                color: "#C7D2FE"
                font.pixelSize: Math.round(12 * AppPalette.scale)
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
        readonly property real maxPanelHeight: parent.height - Tokens.spaceXl * 2
        readonly property real chromeV: Tokens.spaceMd * 2 + Tokens.spaceSm
        width: Math.max(Math.round(200 * AppPalette.scale),
                        Math.min(parent.width - Tokens.spaceXl * 2, Math.round(264 * AppPalette.scale)))
        height: Math.min(maxPanelHeight, chooserHeader.implicitHeight + chooserBody.height + chromeV)
        x: (parent.width - width) / 2
        y: (parent.height - height) / 2
        radius: Tokens.radiusLg
        color: AppPalette.bg
        border.width: 1
        border.color: AppPalette.borderHover

        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.AllButtons
            onPressed: function(mouse) { mouse.accepted = true }
        }

        Column {
            anchors.fill: parent
            anchors.margins: Tokens.spaceMd
            spacing: Tokens.spaceSm

            Text {
                id: chooserHeader
                width: parent.width
                text: qsTr("Fullscreen pop-up")
                color: AppPalette.text
                font.pixelSize: Tokens.fontMd
                font.bold: true
                wrapMode: Text.WordWrap
            }

            Flickable {
                id: chooserBody
                width: parent.width
                height: Math.max(0, Math.min(chooserList.implicitHeight,
                                             popupChooserPanel.maxPanelHeight
                                             - chooserHeader.implicitHeight - popupChooserPanel.chromeV))
                contentWidth: width
                contentHeight: chooserList.implicitHeight
                clip: true
                boundsBehavior: Flickable.StopAtBounds
                interactive: contentHeight > height + 1
                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                Column {
                    id: chooserList
                    width: chooserBody.width
                    spacing: Tokens.spaceSm

                    ChooserRow {
                        width: parent.width
                        label: paneFrame.popupSourceLeafId !== -1 ? qsTr("Remove pop-up") : qsTr("No pop-up")
                        onActivated: {
                            paneFrame.store.setPopupSourceForHost(paneFrame.leafId, -1)
                            paneFrame.popupChooserOpen = false
                        }
                    }

                    Text {
                        visible: paneFrame.popupCandidates.length === 0
                        width: parent.width
                        text: qsTr("No neighboring panes available")
                        color: AppPalette.textMuted
                        font.pixelSize: Tokens.fontSm
                        wrapMode: Text.WordWrap
                    }

                    Repeater {
                        model: paneFrame.popupCandidates

                        delegate: ChooserRow {
                            required property var modelData
                            width: parent.width
                            label: qsTr("Pane %1").arg(modelData.paneId) + " (" + modelData.mode + ")"
                            dotColor: modelData.color
                            selected: paneFrame.popupSourceLeafId === modelData.leafId
                            onActivated: {
                                paneFrame.store.setPopupSourceForHost(paneFrame.leafId, modelData.leafId)
                                paneFrame.store.flashingLeafId = modelData.leafId
                                paneFrame.popupChooserOpen = false
                            }
                            onHoveredChanged: {
                                if (hovered)
                                    paneFrame.store.hoveredPopupCandidateLeafId = modelData.leafId
                                else if (paneFrame.store.hoveredPopupCandidateLeafId === modelData.leafId)
                                    paneFrame.store.hoveredPopupCandidateLeafId = -1
                            }
                        }
                    }
                }
            }
        }
    }

    KCircleIconButton {
        id: editableTopActions
        visible: paneFrame.store.editableMode && paneFrame.store.modePickerLeafId === -1 && paneFrame.canAddPane
        width: paneFrame.edgeAddButtonSize
        height: paneFrame.edgeAddButtonSize
        borderWidth: 1
        fillColor: paneFrame.menuButtonFillColor
        fillHoverColor: paneFrame.menuButtonHoverColor
        fillPressedColor: paneFrame.menuButtonPressedColor
        borderColor: paneFrame.menuButtonBorderColor
        borderHoverColor: paneFrame.menuButtonHoverBorderColor
        iconSource: "qrc:/icons/ui/plus.svg"
        iconTintColor: AppPalette.text
        iconPixelSize: Math.round(paneFrame.edgeAddButtonSize * 0.46)
        toolTipText: qsTr("Add pane")
        z: 31
        x: (parent.width - width) / 2
        y: root.addInset
        onClicked: paneFrame.store.createPaneInLeaf(paneFrame.leafId, "top")
    }

    KCircleIconButton {
        id: editableBottomActions
        visible: paneFrame.store.editableMode && paneFrame.store.modePickerLeafId === -1 && paneFrame.canAddPane
        width: paneFrame.edgeAddButtonSize
        height: paneFrame.edgeAddButtonSize
        borderWidth: 1
        fillColor: paneFrame.menuButtonFillColor
        fillHoverColor: paneFrame.menuButtonHoverColor
        fillPressedColor: paneFrame.menuButtonPressedColor
        borderColor: paneFrame.menuButtonBorderColor
        borderHoverColor: paneFrame.menuButtonHoverBorderColor
        iconSource: "qrc:/icons/ui/plus.svg"
        iconTintColor: AppPalette.text
        iconPixelSize: Math.round(paneFrame.edgeAddButtonSize * 0.46)
        toolTipText: qsTr("Add pane")
        z: 31
        x: (parent.width - width) / 2
        y: parent.height - height - root.addInset
        onClicked: paneFrame.store.createPaneInLeaf(paneFrame.leafId, "bottom")
    }

    KCircleIconButton {
        id: editableLeftActions
        visible: paneFrame.store.editableMode && paneFrame.store.modePickerLeafId === -1 && paneFrame.canAddPane
        width: paneFrame.edgeAddButtonSize
        height: paneFrame.edgeAddButtonSize
        borderWidth: 1
        fillColor: paneFrame.menuButtonFillColor
        fillHoverColor: paneFrame.menuButtonHoverColor
        fillPressedColor: paneFrame.menuButtonPressedColor
        borderColor: paneFrame.menuButtonBorderColor
        borderHoverColor: paneFrame.menuButtonHoverBorderColor
        iconSource: "qrc:/icons/ui/plus.svg"
        iconTintColor: AppPalette.text
        iconPixelSize: Math.round(paneFrame.edgeAddButtonSize * 0.46)
        toolTipText: qsTr("Add pane")
        z: 31
        x: root.addInset
        y: (parent.height - height) / 2
        onClicked: paneFrame.store.createPaneInLeaf(paneFrame.leafId, "left")
    }

    KCircleIconButton {
        id: editableRightActions
        visible: paneFrame.store.editableMode && paneFrame.store.modePickerLeafId === -1 && paneFrame.canAddPane
        width: paneFrame.edgeAddButtonSize
        height: paneFrame.edgeAddButtonSize
        borderWidth: 1
        fillColor: paneFrame.menuButtonFillColor
        fillHoverColor: paneFrame.menuButtonHoverColor
        fillPressedColor: paneFrame.menuButtonPressedColor
        borderColor: paneFrame.menuButtonBorderColor
        borderHoverColor: paneFrame.menuButtonHoverBorderColor
        iconSource: "qrc:/icons/ui/plus.svg"
        iconTintColor: AppPalette.text
        iconPixelSize: Math.round(paneFrame.edgeAddButtonSize * 0.46)
        toolTipText: qsTr("Add pane")
        z: 31
        x: parent.width - width - root.addInset
        y: (parent.height - height) / 2
        onClicked: paneFrame.store.createPaneInLeaf(paneFrame.leafId, "right")
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

}
