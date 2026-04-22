import QtQuick 2.15
import QtQuick.Controls 2.15
import kqml_types 1.0

Item {
    id: root

    required property var store
    property bool favoritesEnabled: true
    property bool expanded: false
    property bool showToggleButton: true
    property int revealShiftX: 0
    property real maxExpandedWidth: 620
    property int controlHeight: 34
    property int panelPaddingX: 8
    property int panelPaddingY: 6
    property int triggerButtonWidth: 92
    readonly property color hotkeysLayerColor: AppPalette.bg
    readonly property color hotkeysPopupLayerColor: AppPalette.bg
    readonly property color buttonFillColor: AppPalette.card
    readonly property color buttonHoverColor: AppPalette.cardHover
    readonly property color buttonPressedColor: AppPalette.bgDeep
    readonly property color buttonBorderColor: AppPalette.border
    readonly property color buttonHoverBorderColor: AppPalette.borderHover
    readonly property int previewCardWidth: 84
    readonly property int previewCardHeight: 64
    readonly property int panelHeight: Math.max(controlHeight + panelPaddingY * 2, 48)
    readonly property bool hasFavoriteLayouts: favoritesEnabled
                                              && store
                                              && store.favoriteLayouts
                                              && store.favoriteLayouts.length > 0
    readonly property int favoriteCount: hasFavoriteLayouts ? store.favoriteLayouts.length : 0
    property bool layoutsMenuOpen: false
    property int favoriteItemSpacing: 6
    property int favoriteListMaxHeight: 244
    property bool markerToolActive: false
    property bool connectionsOnline: true
    property bool markerToolVisible: true
    property bool connectionStatusToolVisible: true
    property string highlightedQuickActionKey: ""
    property int highlightPulseToken: 0
    property string inputDeviceLabel: ""
    property color inputDeviceColor: "#2563EB"
    readonly property int inputDeviceStackSpacing: 8

    readonly property bool popupShown: root.expanded && root.layoutsMenuOpen && root.hasFavoriteLayouts
    readonly property int favoriteItemHeight: 76
    readonly property int favoriteListContentHeight: favoriteCount > 0
                                                     ? favoriteCount * favoriteItemHeight + (favoriteCount - 1) * favoriteItemSpacing
                                                     : 0
    readonly property int favoriteListViewportHeight: Math.min(favoriteListMaxHeight, favoriteListContentHeight)
    readonly property bool inputDeviceBadgeVisible: root.showToggleButton && root.inputDeviceLabel !== ""
    readonly property int inputDeviceBadgeWidth: inputDeviceBadgeVisible ? inputDeviceBadge.implicitWidth : 0

    signal settingsTriggered()
    signal connectionsTriggered()
    signal markerPlacementTriggered()
    signal connectionStatusChanged(bool connected)
    signal openFileTriggered()
    signal closeFileTriggered()
    signal updateBottomTrackTriggered()
    signal updateMosaicTriggered()
    signal mode3DTriggered()
    signal mode2DTriggered()
    signal legacyRequested()

    Component {
        id: layoutsPopupContentComponent

        Column {
            id: popupColumn
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: 7
            spacing: 7

            Flickable {
                id: favoritesFlick
                width: parent.width
                height: root.favoriteListViewportHeight
                contentWidth: favoritesList.width
                contentHeight: favoritesList.implicitHeight
                clip: true
                interactive: contentHeight > height
                boundsBehavior: Flickable.StopAtBounds

                ScrollBar.vertical: ScrollBar {
                    policy: ScrollBar.AsNeeded
                }

                Column {
                    id: favoritesList
                    width: favoritesFlick.width
                    spacing: root.favoriteItemSpacing

                    Repeater {
                        model: root.favoriteCount

                        delegate: FavoriteLayoutCard {
                            required property int index
                            readonly property int favoriteEntryIndex: index
                            readonly property var favoriteEntry: (root.store
                                                                 && root.store.favoriteLayouts
                                                                 && favoriteEntryIndex < root.store.favoriteLayouts.length)
                                                             ? root.store.favoriteLayouts[favoriteEntryIndex]
                                                             : null
                            readonly property var snapshotData: favoriteEntry && favoriteEntry.layout ? favoriteEntry.layout : favoriteEntry
                            readonly property var popupLinksData: favoriteEntry && favoriteEntry.popupLinks ? favoriteEntry.popupLinks : []

                            width: favoritesList.width
                            snapshot: snapshotData
                            popupLinks: popupLinksData
                            favoriteIndex: favoriteEntryIndex
                            selected: root.store && root.store.favoriteLayoutIsCurrent
                                      ? root.store.favoriteLayoutIsCurrent(favoriteEntryIndex)
                                      : false
                            showText: false

                            onClicked: {
                                if (root.store && root.store.applyFavoriteLayout)
                                    root.store.applyFavoriteLayout(favoriteEntryIndex)
                                root.layoutsMenuOpen = false
                                root.expanded = false
                            }
                        }
                    }
                }
            }

            Rectangle {
                width: parent.width
                height: root.controlHeight
                radius: 8
                color: settingsMouse.containsMouse ? root.buttonHoverColor : root.buttonFillColor
                border.width: 1
                border.color: settingsMouse.containsMouse ? root.buttonHoverBorderColor : root.buttonBorderColor

                Text {
                    anchors.centerIn: parent
                    text: "\u2699"
                    color: AppPalette.text
                    font.pixelSize: 19
                    font.bold: true
                }

                MouseArea {
                    id: settingsMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        if (root.store && typeof root.store.openAppLayoutSettings === "function")
                            root.store.openAppLayoutSettings()
                        else
                            root.settingsTriggered()
                        root.layoutsMenuOpen = false
                        root.expanded = false
                    }
                }

                KToolTip {
                    text: "Open layout settings"
                    targetItem: parent
                    shown: settingsMouse.containsMouse
                }
            }
        }
    }

    function revealQuickAction(key) {
        highlightedQuickActionKey = typeof key === "string" ? key : ""
        highlightPulseToken += 1
        expanded = true
    }

    function clearQuickActionReveal() {
        highlightedQuickActionKey = ""
    }

    onExpandedChanged: {
        if (!expanded)
            layoutsMenuOpen = false
    }

    onHasFavoriteLayoutsChanged: {
        if (!hasFavoriteLayouts)
            layoutsMenuOpen = false
    }

    readonly property int leadingClusterWidth: root.showToggleButton
                                               ? Math.max(toggleButton.width, inputDeviceBadgeVisible ? inputDeviceBadgeWidth : 0)
                                               : 0
    readonly property int leadingClusterHeight: root.showToggleButton
                                                ? toggleButton.height
                                                  + (inputDeviceBadgeVisible ? inputDeviceStackSpacing + inputDeviceBadge.height : 0)
                                                : 0
    readonly property int panelOffsetX: (root.showToggleButton ? toggleButton.width + 8 : 0) + root.revealShiftX

    width: Math.max(leadingClusterWidth, root.expanded ? panelOffsetX + panel.width : panelOffsetX)
    height: Math.max(leadingClusterHeight, panel.height)

    component LayoutsTriggerButton: Rectangle {
        id: button

        property bool open: false
        signal clicked()

        implicitWidth: root.triggerButtonWidth
        implicitHeight: root.controlHeight
        radius: height / 2
        color: buttonMouse.containsMouse ? root.buttonHoverColor : root.buttonFillColor
        border.width: 1
        border.color: buttonMouse.containsMouse ? root.buttonHoverBorderColor : root.buttonBorderColor

        Canvas {
            id: iconCanvas
            width: 46
            height: 22
            anchors.centerIn: parent
            anchors.horizontalCenterOffset: -6

            onPaint: {
                var ctx = getContext("2d")
                ctx.clearRect(0, 0, width, height)

                var gap = 2
                var leftWidth = Math.floor(width * 0.48)
                var rightWidth = width - leftWidth - gap
                var topLeftHeight = Math.floor((height - gap) * 0.5)
                var bottomLeftHeight = height - topLeftHeight - gap

                function pane(x, y, w, h, fill, stroke) {
                    ctx.fillStyle = fill
                    ctx.strokeStyle = stroke
                    ctx.lineWidth = 1
                    ctx.fillRect(x, y, w, h)
                    if (w > 1 && h > 1)
                        ctx.strokeRect(x + 0.5, y + 0.5, w - 1, h - 1)
                }

                pane(0, 0, leftWidth, topLeftHeight, "#2563EB", "#93C5FD")
                pane(0, topLeftHeight + gap, leftWidth, bottomLeftHeight, "#2563EB", "#93C5FD")
                pane(leftWidth + gap, 0, rightWidth, height, "#16A34A", "#86EFAC")
            }
        }

        DisclosureIndicator {
            anchors.right: parent.right
            anchors.rightMargin: 10
            anchors.verticalCenter: parent.verticalCenter
            width: 10
            height: 10
            expanded: button.open
            indicatorColor: AppPalette.textSecond
        }

        MouseArea {
            id: buttonMouse
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: button.clicked()
        }

        KToolTip {
            text: "Layouts"
            targetItem: button
            shown: buttonMouse.containsMouse
        }
    }

    component PulseSlot: Item {
        id: slot

        property bool highlighted: false
        property int pulseToken: 0
        property color pulseColor: AppPalette.accentBorder
        property color pulseBorderColor: "#BFDBFE"
        property int cornerRadius: 18
        default property alias contentData: contentHolder.data

        Item {
            id: contentHolder
            anchors.fill: parent
        }

        Rectangle {
            id: pulseRing
            anchors.fill: parent
            radius: slot.cornerRadius
            color: "transparent"
            border.width: 2
            border.color: slot.pulseBorderColor
            opacity: 0
            visible: slot.highlighted
        }

        SequentialAnimation {
            id: pulseAnim
            running: false
            NumberAnimation { target: pulseRing; property: "opacity"; to: 0.95; duration: 90; easing.type: Easing.OutQuad }
            NumberAnimation { target: pulseRing; property: "opacity"; to: 0.32; duration: 180; easing.type: Easing.OutCubic }
            NumberAnimation { target: pulseRing; property: "opacity"; to: 0.0; duration: 280; easing.type: Easing.OutCubic }
        }

        onPulseTokenChanged: {
            if (highlighted)
                pulseAnim.restart()
        }

        onHighlightedChanged: {
            if (!highlighted)
                pulseRing.opacity = 0.0
        }
    }

    CircleIconButton {
        id: toggleButton
        anchors.left: parent.left
        anchors.top: parent.top
        visible: root.showToggleButton
        width: visible ? 48 : 0
        height: visible ? 48 : 0
        iconSource: root.expanded ? "qrc:/icons/ui/x.svg"
                                  : "qrc:/icons/ui/menu-2.svg"
        iconPixelSize: root.expanded ? 19 : 20
        fillColor: root.buttonFillColor
        fillHoverColor: root.buttonHoverColor
        fillPressedColor: root.buttonPressedColor
        borderColor: root.buttonBorderColor
        borderHoverColor: root.buttonHoverBorderColor
        toolTipText: root.expanded ? "Collapse hotkeys" : "Open hotkeys"

        onClicked: {
            if (!root.showToggleButton)
                return
            root.expanded = !root.expanded
        }
    }

    Item {
        id: inputDeviceBadge
        anchors.left: parent.left
        anchors.top: toggleButton.bottom
        anchors.topMargin: root.inputDeviceStackSpacing
        visible: root.inputDeviceBadgeVisible
        implicitWidth: Math.max(76, inputDeviceText.implicitWidth + 34)
        implicitHeight: 26
        width: implicitWidth
        height: implicitHeight

        Rectangle {
            anchors.fill: parent
            radius: height / 2
            color: root.buttonFillColor
            border.width: 1
            border.color: root.inputDeviceColor
            opacity: 0.96
        }

        Rectangle {
            id: inputDeviceDot
            anchors.left: parent.left
            anchors.leftMargin: 10
            anchors.verticalCenter: parent.verticalCenter
            width: 8
            height: 8
            radius: 4
            color: root.inputDeviceColor
        }

        Text {
            id: inputDeviceText
            anchors.left: inputDeviceDot.right
            anchors.leftMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            text: root.inputDeviceLabel
            color: AppPalette.text
            font.pixelSize: 12
            font.bold: true
        }
    }

    Rectangle {
        id: panel
        anchors.left: parent.left
        anchors.leftMargin: panelOffsetX
        anchors.top: parent.top
        width: root.expanded ? Math.min(root.maxExpandedWidth, topRow.implicitWidth + root.panelPaddingX * 2) : 0
        height: root.panelHeight
        radius: height / 2
        clip: true
        color: root.hotkeysLayerColor
        border.width: root.expanded ? 1 : 0
        border.color: AppPalette.border
        opacity: root.expanded ? 1 : 0

        Behavior on width {
            NumberAnimation {
                duration: 220
                easing.type: Easing.OutCubic
            }
        }

        Behavior on opacity {
            NumberAnimation {
                duration: 170
                easing.type: Easing.OutQuad
            }
        }

        Row {
            id: topRow
            anchors.left: parent.left
            anchors.leftMargin: root.panelPaddingX
            anchors.verticalCenter: parent.verticalCenter
            spacing: 8
            height: root.controlHeight

            SettingsGearButton {
                width: root.controlHeight
                height: root.controlHeight
                modeTag: "app"
                fillColor: root.buttonFillColor
                fillHoverColor: root.buttonHoverColor
                fillPressedColor: root.buttonPressedColor
                borderColor: root.buttonBorderColor
                borderHoverColor: root.buttonHoverBorderColor
                onClicked: {
                    root.settingsTriggered()
                    root.expanded = false
                }
            }

            CircleIconButton {
                width: root.controlHeight
                height: root.controlHeight
                iconSource: "qrc:/icons/ui/layout-board.svg"
                iconPixelSize: 18
                fillColor: root.buttonFillColor
                fillHoverColor: root.buttonHoverColor
                fillPressedColor: root.buttonPressedColor
                borderColor: root.buttonBorderColor
                borderHoverColor: root.buttonHoverBorderColor
                toolTipText: "Legacy menus"
                onClicked: root.legacyRequested()
            }

            PulseSlot {
                id: layoutsSlot
                highlighted: root.highlightedQuickActionKey === "layouts"
                pulseToken: root.highlightPulseToken
                pulseColor: AppPalette.accentBorder
                pulseBorderColor: AppPalette.accentBar
                width: layoutsButton.visible ? layoutsButton.implicitWidth : 0
                height: layoutsButton.visible ? layoutsButton.implicitHeight : 0
                cornerRadius: 18

                LayoutsTriggerButton {
                    id: layoutsButton
                    anchors.fill: parent
                    visible: root.hasFavoriteLayouts || root.highlightedQuickActionKey === "layouts"
                    open: root.layoutsMenuOpen
                    onClicked: root.layoutsMenuOpen = !root.layoutsMenuOpen
                }
            }

            PulseSlot {
                id: markerSlot
                highlighted: root.highlightedQuickActionKey === "marker"
                pulseToken: root.highlightPulseToken
                pulseColor: AppPalette.accentBorder
                pulseBorderColor: AppPalette.accentBar
                width: markerToolButton.visible ? root.controlHeight : 0
                height: markerToolButton.visible ? root.controlHeight : 0
                cornerRadius: root.controlHeight / 2

                CircleIconButton {
                    id: markerToolButton
                    anchors.fill: parent
                    visible: root.markerToolVisible || root.highlightedQuickActionKey === "marker"
                    iconSource: "qrc:/icons/ui/anchor.svg"
                    iconPixelSize: 18
                    fillColor: root.markerToolActive ? "#1D4ED8" : root.buttonFillColor
                    fillHoverColor: root.markerToolActive ? "#1E40AF" : root.buttonHoverColor
                    fillPressedColor: root.markerToolActive ? AppPalette.accentBg : root.buttonPressedColor
                    borderColor: root.markerToolActive ? AppPalette.accentBar : root.buttonBorderColor
                    borderHoverColor: root.markerToolActive ? AppPalette.accentBorder : root.buttonHoverBorderColor
                    toolTipText: root.markerToolActive ? "Marker tool enabled" : "Place marker"
                    onClicked: {
                        root.markerToolActive = !root.markerToolActive
                        root.markerPlacementTriggered()
                    }
                }
            }

            PulseSlot {
                id: connectionSlot
                highlighted: root.highlightedQuickActionKey === "connections"
                pulseToken: root.highlightPulseToken
                pulseColor: "#86EFAC"
                pulseBorderColor: "#4ADE80"
                width: connectionStatusButton.visible ? root.controlHeight : 0
                height: connectionStatusButton.visible ? root.controlHeight : 0
                cornerRadius: root.controlHeight / 2

            CircleIconButton {
                id: connectionStatusButton
                anchors.fill: parent
                visible: root.connectionStatusToolVisible || root.highlightedQuickActionKey === "connections"
                    iconSource: root.connectionsOnline
                                ? "qrc:/icons/ui/access_point.svg"
                                : "qrc:/icons/ui/access_point_off.svg"
                    iconPixelSize: 18
                    fillColor: root.connectionsOnline ? "#14532D" : "#7F1D1D"
                    fillHoverColor: root.connectionsOnline ? "#166534" : "#991B1B"
                    fillPressedColor: root.connectionsOnline ? "#052E16" : "#7F1D1D"
                    borderColor: root.connectionsOnline ? "#22C55E" : "#EF4444"
                    borderHoverColor: root.connectionsOnline ? "#4ADE80" : "#F87171"
                    toolTipText: "Open connections"
                    onClicked: {
                        root.connectionsTriggered()
                    }
                }

                Rectangle {
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    anchors.rightMargin: 3
                    anchors.bottomMargin: 3
                    width: 8
                    height: 8
                    radius: 4
                    color: root.connectionsOnline ? "#22C55E" : "#EF4444"
                    border.width: 1
                    border.color: AppPalette.bgDeep
                    visible: root.connectionStatusToolVisible || root.highlightedQuickActionKey === "connections"
                }
            }

            CircleIconButton {
                id: mode3DButton
                visible: true
                width: root.controlHeight
                height: root.controlHeight
                iconSource: "qrc:/icons/ui/map.svg"
                iconPixelSize: 18
                fillColor: "#14532D"
                fillHoverColor: "#166534"
                fillPressedColor: "#052E16"
                borderColor: "#22C55E"
                borderHoverColor: "#4ADE80"
                toolTipText: "Set active pane to 3D"
                onClicked: {
                    root.mode3DTriggered()
                }
            }

            CircleIconButton {
                id: mode2DButton
                visible: true
                width: root.controlHeight
                height: root.controlHeight
                iconSource: "qrc:/icons/ui/ripple.svg"
                iconPixelSize: 18
                fillColor: AppPalette.accentBg
                fillHoverColor: "#1D4ED8"
                fillPressedColor: "#172554"
                borderColor: AppPalette.accentBar
                borderHoverColor: AppPalette.accentBorder
                toolTipText: "Set active pane to 2D"
                onClicked: {
                    root.mode2DTriggered()
                }
            }

            CircleIconButton {
                id: openFileButton
                visible: true
                width: root.controlHeight
                height: root.controlHeight
                iconSource: "qrc:/icons/ui/folder-open.svg"
                iconPixelSize: 18
                fillColor: AppPalette.card
                fillHoverColor: AppPalette.cardHover
                fillPressedColor: AppPalette.bgDeep
                borderColor: AppPalette.borderFocus
                borderHoverColor: AppPalette.textMuted
                toolTipText: "Open file"
                onClicked: {
                    root.openFileTriggered()
                }
            }

            CircleIconButton {
                id: closeFileButton
                visible: true
                width: root.controlHeight
                height: root.controlHeight
                iconSource: "qrc:/icons/ui/file_off.svg"
                iconPixelSize: 18
                fillColor: "#7F1D1D"
                fillHoverColor: "#991B1B"
                fillPressedColor: "#450A0A"
                borderColor: "#F87171"
                borderHoverColor: "#FCA5A5"
                toolTipText: "Close file"
                onClicked: {
                    root.closeFileTriggered()
                }
            }

            CircleIconButton {
                id: updateBottomTrackButton
                visible: true
                width: root.controlHeight
                height: root.controlHeight
                iconSource: "qrc:/icons/ui/file-check.svg"
                iconPixelSize: 18
                fillColor: "#0F766E"
                fillHoverColor: "#13847B"
                fillPressedColor: "#134E4A"
                borderColor: "#5EEAD4"
                borderHoverColor: "#99F6E4"
                toolTipText: "Update bottom track"
                onClicked: {
                    root.updateBottomTrackTriggered()
                }
            }

            CircleIconButton {
                id: updateMosaicButton
                visible: true
                width: root.controlHeight
                height: root.controlHeight
                iconSource: "qrc:/icons/ui/layers-selected.svg"
                iconPixelSize: 18
                fillColor: "#4338CA"
                fillHoverColor: "#4F46E5"
                fillPressedColor: "#312E81"
                borderColor: "#A5B4FC"
                borderHoverColor: "#C7D2FE"
                toolTipText: "Update mosaic"
                onClicked: {
                    root.updateMosaicTriggered()
                }
            }
        }
    }

    Rectangle {
        id: layoutsPopup
        readonly property real desiredX: panel.x + topRow.x + layoutsButton.x + (layoutsButton.width - width) / 2

        x: Math.round(desiredX)
        y: panel.y + panel.height + 6
        width: Math.max(root.previewCardWidth + 16, layoutsButton.width + 8)
        height: popupContentLoader.item ? popupContentLoader.item.implicitHeight + 14 : 14
        radius: 12
        color: root.hotkeysPopupLayerColor
        border.width: 1
        border.color: AppPalette.border
        z: panel.z + 20
        visible: opacity > 0.01
        opacity: root.popupShown ? 1 : 0

        Behavior on opacity {
            NumberAnimation {
                duration: 150
                easing.type: Easing.OutQuad
            }
        }

        Loader {
            id: popupContentLoader
            anchors.fill: parent
            active: root.popupShown || layoutsPopup.opacity > 0.01
            sourceComponent: layoutsPopupContentComponent
        }
    }
}
