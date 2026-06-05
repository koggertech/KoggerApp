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
    readonly property real _s: 1.5 * (theme ? theme.resCoeff : 1.0)
    readonly property int _windowW: store ? store.windowWidth : 1440
    property real maxExpandedWidth: Math.max(240,
                                             Math.min(620 * root._s,
                                                      _windowW - 32 * root._s))
    property int controlHeight: Math.round(36 * root._s) - 2
    property int panelPaddingX: Math.round(8 * root._s)
    property int panelPaddingY: Math.round(6 * root._s)
    property int triggerButtonWidth: Math.round(92 * root._s)
    readonly property color hotkeysLayerColor: AppPalette.bg
    readonly property color hotkeysPopupLayerColor: AppPalette.bg
    readonly property color buttonFillColor: AppPalette.card
    readonly property color buttonHoverColor: AppPalette.cardHover
    readonly property color buttonPressedColor: AppPalette.bgDeep
    readonly property color buttonBorderColor: AppPalette.border
    readonly property color buttonHoverBorderColor: AppPalette.borderHover
    readonly property int panelHeight: Math.max(controlHeight + panelPaddingY * 2, Math.round(48 * root._s))
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
    property bool bottomTrackEditorEnabled: true
    readonly property bool _btEditRevealOverride: _revealActiveKey === "bottomTrack"
    readonly property bool showBtEdit: bottomTrackEditorEnabled || _btEditRevealOverride
    readonly property int btTool: (typeof core !== "undefined" && core) ? core.bottomTrackEditTool : 0
    property int favoriteItemSpacing: Math.round(6 * root._s)
    property int favoriteListMaxHeight: Math.round(244 * root._s)
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
    readonly property int inputDeviceStackSpacing: Math.round(8 * root._s)

    readonly property int favoriteItemHeight: Math.round(62 * root._s)
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
    // Index into `devices` (= deviceManagerWrapper.devs) — devSN can collide.
    signal deviceTriggered(int devIndex)

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
            anchors.margins: Math.round(7 * root._s)
            spacing: Math.round(7 * root._s)

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
                    id: favScrollBar
                    policy: ScrollBar.AsNeeded
                    width: Math.round(6 * root._s)
                }

                Column {
                    id: favoritesList
                    width: favoritesFlick.width
                           - (favoritesFlick.interactive ? favScrollBar.width + Math.round(4 * root._s) : 0)
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
                            height: root.favoriteItemHeight
                            contentMargin: Math.round(6 * root._s)
                            previewWidth:  Math.round(60 * root._s)
                            previewHeight: Math.round(46 * root._s)
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

            Item {
                width: parent.width
                height: root.controlHeight

                SettingsGearButton {
                    anchors.centerIn: parent
                    width: root.controlHeight
                    height: root.controlHeight
                    modeTag: "app"
                    toolTipText: qsTr("Open layout settings")
                    onClicked: {
                        if (root.store && typeof root.store.openAppLayoutSettings === "function")
                            root.store.openAppLayoutSettings()
                        else
                            root.settingsTriggered()
                        root.layoutsMenuOpen = false
                        root.expanded = false
                    }
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
    readonly property int panelOffsetX: (root.showToggleButton ? toggleButton.width + Math.round(8 * root._s) : 0) + root.revealShiftX

    width: Math.max(leadingClusterWidth,
                    root.expanded ? panelOffsetX + panel.width
                                  : panelOffsetX + (collapsedDeviceRow.visible ? collapsedDeviceRow.width : 0))
    height: Math.max(leadingClusterHeight, panel.height, layoutsCombo.y + backing.height, btEditCombo.y + btEditCombo.height)

    component LayoutsTriggerButton: Rectangle {
        id: button

        property bool open: false
        property bool dropped: false       // dropdown open/animating → merge into container
        property bool highlighted: false   // "look here" reveal hint (layouts hotkey)
        property int flashToken: 0
        signal clicked()

        readonly property var currentEntry: root.store && root.store.favoriteLayoutEntryFromCurrent
                                            ? root.store.favoriteLayoutEntryFromCurrent()
                                            : null
        readonly property int iconH: root.controlHeight - Math.round(12 * root._s)
        readonly property int iconW: Math.round(button.iconH * 21 / 16)

        implicitWidth: root.triggerButtonWidth
        implicitHeight: root.controlHeight
        radius: height / 2
        color: button.dropped ? "transparent" : (buttonMouse.containsMouse ? root.buttonHoverColor : root.buttonFillColor)
        border.width: button.dropped ? 0 : 1
        border.color: buttonMouse.containsMouse ? root.buttonHoverBorderColor : root.buttonBorderColor

        // Hover whiteness — same feel as KCircleIconButton (the pencil trigger).
        Rectangle {
            anchors.fill: parent
            radius: parent.radius
            color: "#FFFFFF"
            opacity: buttonMouse.containsMouse ? 0.12 : 0.0
            visible: opacity > 0.001
            Behavior on opacity { NumberAnimation { duration: 110; easing.type: Easing.OutCubic } }
        }

        LayoutSnapshotPreview {
            id: iconPreview
            width: button.iconW
            height: button.iconH
            anchors.centerIn: parent
            anchors.horizontalCenterOffset: -Math.round(6 * root._s)
            layoutSnapshot: button.currentEntry ? button.currentEntry.layout : null
            popupLinks: button.currentEntry && button.currentEntry.popupLinks
                        ? button.currentEntry.popupLinks : []
        }

        DisclosureIndicator {
            anchors.right: parent.right
            anchors.rightMargin: Math.round(10 * root._s)
            anchors.verticalCenter: parent.verticalCenter
            width: Math.round(10 * root._s)
            height: Math.round(10 * root._s)
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

        Rectangle {
            id: revealPulse
            anchors.fill: parent
            radius: button.radius
            color: "transparent"
            border.width: Math.max(2, Math.round(2 * root._s))
            border.color: AppPalette.accentBorder
            opacity: 0
            visible: button.highlighted
            z: 10
        }

        SequentialAnimation {
            id: revealPulseAnim
            running: false
            NumberAnimation { target: revealPulse; property: "opacity"; to: 0.95; duration: 90;  easing.type: Easing.OutCubic }
            NumberAnimation { target: revealPulse; property: "opacity"; to: 0.30; duration: 180; easing.type: Easing.OutCubic }
            NumberAnimation { target: revealPulse; property: "opacity"; to: 0.0;  duration: 280; easing.type: Easing.OutCubic }
        }

        onFlashTokenChanged: if (button.highlighted) revealPulseAnim.restart()
        onHighlightedChanged: if (!button.highlighted) revealPulse.opacity = 0.0
        Component.onCompleted: if (button.highlighted) revealPulseAnim.restart()
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
            NumberAnimation { target: pulseRing; property: "opacity"; to: 0.95; duration: 90; easing.type: Easing.OutCubic }
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

    Component {
        id: deviceShortcutDelegate

        KCircleIconButton {
            required property var modelData
            required property int index
            readonly property color _fill:   root.linkFillColor(modelData)
            readonly property color _border: root.linkBorderColor(modelData)

            visible: modelData ? (modelData.devType !== 0) : false
            width: visible ? root.controlHeight : 0
            height: root.controlHeight
            iconSource: root.iconForDevice(modelData)
            iconTintColor: AppPalette.text
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
                root.deviceTriggered(index)
                root.expanded = false
            }

            // Set sonar frequency (transducer kHz) — small number, bottom-right.
            Text {
                visible: modelData && modelData.isTransducerSupport && modelData.transFreq > 0
                anchors.bottom: parent.bottom
                anchors.right: parent.right
                anchors.bottomMargin: Math.round(2 * root._s)
                anchors.rightMargin: Math.round(4 * root._s)
                text: modelData ? modelData.transFreq : ""
                color: AppPalette.text
                font.pixelSize: Math.round(9 * root._s)
                font.bold: true
                style: Text.Outline
                styleColor: "#000000B0"
            }
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
        iconTintColor: AppPalette.text
        iconPixelSize: Math.round((root.expanded ? 19 : 20) * root._s)
        fillColor: root.buttonFillColor
        fillHoverColor: root.buttonHoverColor
        fillPressedColor: root.buttonPressedColor
        borderColor: root.buttonBorderColor
        borderHoverColor: root.buttonHoverBorderColor
        toolTipText: root.expanded ? qsTr("Collapse hotkeys") : qsTr("Open hotkeys")

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
        implicitWidth: Math.max(Math.round(76 * root._s),
                                inputDeviceText.implicitWidth + Math.round(34 * root._s))
        implicitHeight: Math.round(26 * root._s)
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
            anchors.leftMargin: Math.round(12 * root._s)
            anchors.verticalCenter: parent.verticalCenter
            width: Math.round(8 * root._s)
            height: width
            radius: width / 2
            color: root.inputDeviceColor
        }

        Text {
            id: inputDeviceText
            anchors.left: inputDeviceDot.right
            anchors.leftMargin: Math.round(8 * root._s)
            anchors.verticalCenter: parent.verticalCenter
            text: root.inputDeviceLabel
            color: AppPalette.text
            font.pixelSize: Math.round(12 * root._s)
            font.bold: true
        }
    }

    Row {
        id: collapsedDeviceRow
        anchors.left: parent.left
        anchors.leftMargin: root.panelOffsetX
        anchors.verticalCenter: toggleButton.verticalCenter
        spacing: Math.round(8 * root._s)
        visible: root.showToggleButton && !root.expanded && root.connectionStatusToolVisible
        opacity: visible ? 1 : 0

        Behavior on opacity {
            NumberAnimation { duration: 170; easing.type: Easing.OutCubic }
        }

        Repeater {
            model: root.connectionStatusToolVisible ? root.devices : 0
            delegate: deviceShortcutDelegate
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
                easing.type: Easing.OutCubic
            }
        }

        Row {
            id: topRow
            anchors.left: parent.left
            anchors.leftMargin: root.panelPaddingX
            anchors.verticalCenter: parent.verticalCenter
            spacing: Math.round(8 * root._s)
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
                delegate: deviceShortcutDelegate
            }

            Item {
                id: btEditSlot
                visible: root.showBtEdit
                width: visible ? root.controlHeight : 0
                height: root.controlHeight
            }

            Item {
                id: layoutsSlot
                visible: root.hasFavoriteLayouts
                width: visible ? root.triggerButtonWidth : 0
                height: root.controlHeight
            }

            KCircleIconButton {
                visible: Qt.platform.os !== "android" && Qt.platform.os !== "ios"
                width: root.controlHeight
                height: root.controlHeight
                iconSource: "qrc:/icons/ui/external-link.svg"
                iconTintColor: AppPalette.text
                toolTipText: root.secondWindowOpen
                             ? qsTr("Close second window")
                             : qsTr("Open second window")
                fillColor:        root.secondWindowOpen ? AppPalette.accentBgStrong : root.buttonFillColor
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

    Item {
        id: layoutsCombo
        readonly property int comboW: root.triggerButtonWidth
        readonly property int gap: Math.round(6 * root._s)
        readonly property int sidePad: Math.round(3 * root._s)
        readonly property int dropBodyH: root.favoriteListViewportHeight
                                         + root.controlHeight + Math.round(22 * root._s)
        readonly property bool dropped: root.layoutsMenuOpen
                                        || backing.height > root.controlHeight + layoutsCombo.sidePad * 2 + 1
        visible: root.hasFavoriteLayouts && panel.opacity > 0.01
        opacity: panel.opacity         // fade in/out together with the pill
        x: panel.x + topRow.x + layoutsSlot.x - layoutsCombo.sidePad
        y: panel.y + topRow.y - layoutsCombo.sidePad
        width: comboW + layoutsCombo.sidePad * 2
        z: panel.z + 1                 // above the toolbar; the button is the head
        height: backing.height

        Rectangle {
            id: backing
            width: parent.width
            y: 0
            height: layoutsCombo.sidePad + root.controlHeight
                    + (root.layoutsMenuOpen ? layoutsCombo.gap + layoutsCombo.dropBodyH : layoutsCombo.sidePad)
            radius: btEditCombo.width / 2   // не больше скругления задника bottom-track
            color: layoutsCombo.dropped ? root.hotkeysLayerColor : "transparent"
            border.width: layoutsCombo.dropped ? 1 : 0
            border.color: AppPalette.border
            clip: true

            Behavior on height {
                NumberAnimation { duration: 200; easing.type: Easing.OutCubic }
            }

            MouseArea {
                anchors.fill: parent
                hoverEnabled: true
                acceptedButtons: Qt.AllButtons
                onPressed: function(mouse) { mouse.accepted = true }
                onClicked: function(mouse) { mouse.accepted = true }
                onWheel: function(wheel) { wheel.accepted = true }
            }

            Loader {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.topMargin: layoutsCombo.sidePad + root.controlHeight + layoutsCombo.gap   // below the head
                sourceComponent: layoutsPopupContentComponent
            }
        }

        LayoutsTriggerButton {
            id: layoutsTrigger
            anchors.top: parent.top
            anchors.topMargin: layoutsCombo.sidePad
            anchors.horizontalCenter: parent.horizontalCenter
            width: layoutsCombo.comboW
            height: root.controlHeight
            open: root.layoutsMenuOpen
            dropped: layoutsCombo.dropped
            highlighted: root.highlightedQuickActionKey === "layouts"
            flashToken: root.highlightPulseToken
            onClicked: root.layoutsMenuOpen = !root.layoutsMenuOpen
        }
    }

    Item {
        id: btEditCombo
        readonly property int sidePad: Math.round(3 * root._s)
        visible: root.showBtEdit && panel.opacity > 0.01
        opacity: panel.opacity
        x: panel.x + topRow.x + btEditSlot.x - btEditCombo.sidePad
        y: panel.y + topRow.y - btEditCombo.sidePad
        width: root.controlHeight + btEditCombo.sidePad * 2
        height: root.controlHeight + btEditCombo.sidePad * 2
        z: panel.z + 1

        KCircleIconButton {
            id: btEditTrigger
            anchors.centerIn: parent
            width: root.controlHeight
            height: root.controlHeight
            readonly property bool _open: root.store && root.store.bottomTrackEditorOpen
            readonly property bool _accent: root.btTool !== 0 || _open
            iconSource: _open ? "qrc:/icons/ui/x.svg" : "qrc:/icons/ui/pencil.svg"
            iconTintColor: AppPalette.text
            toolTipText: _open ? qsTr("Close bottom track editing")
                               : qsTr("Bottom track editing")
            fillColor:        _accent ? AppPalette.accentBgStrong : root.buttonFillColor
            fillHoverColor:   _accent ? AppPalette.accentBorder : root.buttonHoverColor
            fillPressedColor: root.buttonPressedColor
            borderColor:      _accent ? AppPalette.accentBorder : root.buttonBorderColor
            borderHoverColor: _accent ? AppPalette.accentBorder : root.buttonHoverBorderColor
            highlighted: root.highlightedQuickActionKey === "bottomTrack"
            flashToken: root.highlightPulseToken
            onClicked: {
                if (!root.store) return
                var willOpen = !root.store.bottomTrackEditorOpen
                root.store.bottomTrackEditorOpen = willOpen
                if (willOpen)
                    root.expanded = false
            }
        }
    }

}

