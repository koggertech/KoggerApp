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
    // Cap by window width so panel never overflows on narrow screens.
    readonly property int _windowW: store ? store.windowWidth : 1440
    property real maxExpandedWidth: Math.max(240,
                                             Math.min(620 * AppPalette.scale,
                                                      _windowW - 32 * AppPalette.scale))
    property int controlHeight: Tokens.controlHLg - 2
    property int panelPaddingX: Tokens.spaceMd
    property int panelPaddingY: Tokens.spaceSm
    property int triggerButtonWidth: Math.round(92 * AppPalette.scale)
    readonly property color hotkeysLayerColor: AppPalette.bg
    readonly property color hotkeysPopupLayerColor: AppPalette.bg
    readonly property color buttonFillColor: AppPalette.card
    readonly property color buttonHoverColor: AppPalette.cardHover
    readonly property color buttonPressedColor: AppPalette.bgDeep
    readonly property color buttonBorderColor: AppPalette.border
    readonly property color buttonHoverBorderColor: AppPalette.borderHover
    readonly property int previewCardWidth: Math.round(84 * AppPalette.scale)
    readonly property int previewCardHeight: Math.round(64 * AppPalette.scale)
    readonly property int panelHeight: Math.max(controlHeight + panelPaddingY * 2, Tokens.controlHXl)
    // While the "layouts" reveal sequence is active we keep showing the icons
    // even if the user just disabled them — so they're visible during the
    // whole open → pulse → close cycle instead of disappearing instantly.
    readonly property bool _favoritesRevealOverride: _revealActiveKey === "layouts"
    readonly property bool hasFavoriteLayouts: (favoritesEnabled || _favoritesRevealOverride)
                                              && store
                                              && store.favoriteLayouts
                                              && store.favoriteLayouts.length > 0
    readonly property int favoriteCount: hasFavoriteLayouts ? store.favoriteLayouts.length : 0
    property bool layoutsMenuOpen: false
    property int favoriteItemSpacing: Tokens.spaceSm
    property int favoriteListMaxHeight: Math.round(244 * AppPalette.scale)
    property bool connectionsOnline: true
    property bool connectionStatusToolVisible: true
    property string highlightedQuickActionKey: ""
    property int highlightPulseToken: 0
    // Tracks the active reveal for the whole sequence — set when reveal fires,
    // cleared only when the panel actually collapses. Used to keep the
    // about-to-disappear icons visible until the very end of the close.
    property string _revealActiveKey: ""
    property string inputDeviceLabel: ""
    property color inputDeviceColor: "#2563EB"
    readonly property int inputDeviceStackSpacing: Tokens.spaceMd

    readonly property bool popupShown: root.expanded && root.layoutsMenuOpen && root.hasFavoriteLayouts
    readonly property int favoriteItemHeight: Math.round(76 * AppPalette.scale)
    readonly property int favoriteListContentHeight: favoriteCount > 0
                                                     ? favoriteCount * favoriteItemHeight + (favoriteCount - 1) * favoriteItemSpacing
                                                     : 0
    readonly property int favoriteListViewportHeight: Math.min(favoriteListMaxHeight, favoriteListContentHeight)
    readonly property bool inputDeviceBadgeVisible:
        root.showToggleButton
        && root.inputDeviceLabel !== ""
        && (typeof manualTesting !== "undefined" && manualTesting === true)
    readonly property int inputDeviceBadgeWidth: inputDeviceBadgeVisible ? inputDeviceBadge.implicitWidth : 0

    signal settingsTriggered()
    signal connectionsTriggered()
    signal connectionStatusChanged(bool connected)
    signal openFileTriggered()
    signal closeFileTriggered()
    signal updateBottomTrackTriggered()
    signal updateMosaicTriggered()
    signal mode3DTriggered()
    signal mode2DTriggered()
    signal legacyRequested()
    signal secondWindowToggleRequested()
    property bool secondWindowOpen: false

    // Devices bound from MainWindow (deviceManagerWrapper.devs).
    // Status colors mirror ConnectionViewer link-row palette.
    property var devices: []
    signal deviceTriggered(int devSN)

    function iconForDevice(d) {
        if (!d) return "qrc:/icons/ui/device-unknown.svg"
        if (d.isSonar)                  return "qrc:/icons/ui/device-transducer.svg"
        if (d.isDoppler)                return "qrc:/icons/ui/device-doppler.svg"
        if (d.isUSBLBeacon || d.isUSBL) return "qrc:/icons/ui/device-usbl.svg"
        if (d.isRecorder)               return "qrc:/icons/ui/device-recorder.svg"
        return "qrc:/icons/ui/device-unknown.svg"
    }

    function linkFillColor(d) {
        if (!d) return buttonFillColor
        if (d.linkConnected)    return d.linkReceivesData ? "#0D2D1A" : "#2D2200"
        if (d.linkNotAvailable) return "#2D0D0D"
        return buttonFillColor
    }

    function linkBorderColor(d) {
        if (!d) return buttonBorderColor
        if (d.linkConnected)    return d.linkReceivesData ? "#10B981" : "#F59E0B"
        if (d.linkNotAvailable) return "#EF4444"
        return buttonBorderColor
    }

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
                    text: qsTr("Open layout settings")
                    targetItem: parent
                    shown: settingsMouse.containsMouse
                }
            }
        }
    }

    // Stage 1: open the panel and make the about-to-flash icons visible
    // (via _revealActiveKey override). No pulse yet — see pulseRevealedAction.
    function revealQuickAction(key) {
        var k = typeof key === "string" ? key : ""
        _revealActiveKey = k
        highlightedQuickActionKey = ""
        expanded = true
    }

    // Stage 2: bump flashToken so icons (already rendered) start pulsing.
    function pulseRevealedAction() {
        if (_revealActiveKey === "")
            return
        highlightedQuickActionKey = _revealActiveKey
        highlightPulseToken += 1
    }

    function clearQuickActionReveal() {
        highlightedQuickActionKey = ""
        // Clear the override too so disabled icons VANISH at this point
        // (while the panel is still open). Panel closure follows a moment
        // later via the close timer in MainWindow.
        _revealActiveKey = ""
    }

    onExpandedChanged: {
        if (!expanded) {
            layoutsMenuOpen = false
            _revealActiveKey = ""
        }
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
            text: qsTr("Layouts")
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

    KCircleIconButton {
        id: toggleButton
        anchors.left: parent.left
        anchors.top: parent.top
        visible: root.showToggleButton
        // Match panel height so toggle and expanded panel are flush vertically.
        width: visible ? root.panelHeight : 0
        height: visible ? root.panelHeight : 0
        iconSource: root.expanded ? "qrc:/icons/ui/x.svg"
                                  : "qrc:/icons/ui/menu-2.svg"
        iconPixelSize: Math.round((root.expanded ? 19 : 20) * AppPalette.scale)
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
        implicitWidth: Math.max(Math.round(76 * AppPalette.scale),
                                inputDeviceText.implicitWidth + Math.round(34 * AppPalette.scale))
        implicitHeight: Math.round(26 * AppPalette.scale)
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
            anchors.leftMargin: Tokens.spaceLg
            anchors.verticalCenter: parent.verticalCenter
            width: Math.round(8 * AppPalette.scale)
            height: width
            radius: width / 2
            color: root.inputDeviceColor
        }

        Text {
            id: inputDeviceText
            anchors.left: inputDeviceDot.right
            anchors.leftMargin: Tokens.spaceMd
            anchors.verticalCenter: parent.verticalCenter
            text: root.inputDeviceLabel
            color: AppPalette.text
            font.pixelSize: Tokens.fontSm
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

            Repeater {
                readonly property bool _devicesRevealOverride: root._revealActiveKey === "connections"
                model: (root.connectionStatusToolVisible || _devicesRevealOverride) ? root.devices : 0
                delegate: KCircleIconButton {
                    required property var modelData
                    readonly property color _fill:   root.linkFillColor(modelData)
                    readonly property color _border: root.linkBorderColor(modelData)

                    visible: modelData ? (modelData.devType !== 0) : false
                    width: visible ? root.controlHeight : 0
                    height: root.controlHeight
                    iconSource: root.iconForDevice(modelData)
                    toolTipText: modelData
                                 ? (modelData.devName + " " + modelData.fwVersion + " [" + modelData.devSN + "]")
                                 : ""
                    fillColor:        _fill
                    fillHoverColor:   _fill
                    fillPressedColor: root.buttonPressedColor
                    borderColor:      _border
                    borderHoverColor: _border

                    highlighted: root.highlightedQuickActionKey === "connections"
                    flashToken: root.highlightPulseToken

                    onClicked: {
                        if (!modelData) return
                        root.deviceTriggered(modelData.devSN)
                        root.expanded = false
                    }
                }
            }

            // ── Favorite layout pictograms ─────────────────────────────────
            // One thumbnail per favorite. Click applies it. Skipped when the
            // user disabled the group in Menu settings (favoritesEnabled).
            Repeater {
                model: root.hasFavoriteLayouts ? root.favoriteCount : 0
                delegate: FavoriteLayoutCard {
                    required property int index
                    readonly property var favoriteEntry: (root.store
                                                          && root.store.favoriteLayouts
                                                          && index < root.store.favoriteLayouts.length)
                                                         ? root.store.favoriteLayouts[index]
                                                         : null
                    readonly property var snapshotData: favoriteEntry && favoriteEntry.layout
                                                       ? favoriteEntry.layout
                                                       : favoriteEntry
                    readonly property var popupLinksData: favoriteEntry && favoriteEntry.popupLinks
                                                         ? favoriteEntry.popupLinks
                                                         : []

                    showText: false
                    // Wider pill button without stretching the preview itself:
                    // small aspect bump (1.25×) + generous horizontal padding.
                    previewHeight: root.controlHeight - Math.round(10 * AppPalette.scale)
                    previewWidth:  Math.round(previewHeight * 1.25)
                    contentMargin: Math.round(10 * AppPalette.scale)
                    width: previewWidth + 2 * contentMargin
                    height: root.controlHeight
                    radius: height / 2

                    snapshot: snapshotData
                    popupLinks: popupLinksData
                    favoriteIndex: index
                    selected: root.store && root.store.favoriteLayoutIsCurrent
                              ? root.store.favoriteLayoutIsCurrent(index)
                              : false

                    highlighted: root.highlightedQuickActionKey === "layouts"
                    flashToken: root.highlightPulseToken

                    onClicked: {
                        if (root.store && root.store.applyFavoriteLayout)
                            root.store.applyFavoriteLayout(index)
                        root.expanded = false
                    }
                }
            }

            KCircleIconButton {
                visible: Qt.platform.os !== "android" && Qt.platform.os !== "ios"
                width: root.controlHeight
                height: root.controlHeight
                iconSource: "qrc:/icons/ui/external-link.svg"
                toolTipText: root.secondWindowOpen
                             ? qsTr("Close second window")
                             : qsTr("Open second window")
                fillColor:        root.secondWindowOpen ? AppPalette.accentBg     : root.buttonFillColor
                fillHoverColor:   root.secondWindowOpen ? AppPalette.accentBorder : root.buttonHoverColor
                fillPressedColor: root.buttonPressedColor
                borderColor:      root.secondWindowOpen ? AppPalette.accentBorder : root.buttonBorderColor
                borderHoverColor: root.secondWindowOpen ? AppPalette.accentBorder : root.buttonHoverBorderColor
                onClicked: {
                    root.secondWindowToggleRequested()
                    root.expanded = false
                }
            }

        }
    }

}

