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
    property int panelPaddingX: Math.round(3 * root._s)
    property int triggerButtonWidth: Math.round(92 * root._s)
    readonly property int toggleButtonSize: root.controlHeight
    readonly property color hotkeysLayerColor: AppPalette.bg
    readonly property color hotkeysPopupLayerColor: AppPalette.bg
    readonly property color buttonFillColor: AppPalette.card
    readonly property color buttonHoverColor: AppPalette.cardHover
    readonly property color buttonPressedColor: AppPalette.bgDeep
    readonly property color buttonBorderColor: AppPalette.border
    readonly property color buttonHoverBorderColor: AppPalette.borderHover
    readonly property int panelHeight: controlHeight + panelPaddingX * 2
    readonly property int _arrowSlotW: showToggleButton ? toggleButtonSize + Math.round(8 * root._s) : 0
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
    property bool profilesEnabled: true
    readonly property bool _profilesRevealOverride: _revealActiveKey === "profiles"
    readonly property bool showProfiles: profilesEnabled || _profilesRevealOverride
    property bool extraInfoEnabled: true
    readonly property bool _extraInfoRevealOverride: _revealActiveKey === "extraInfo"
    readonly property bool showExtraInfo: extraInfoEnabled || _extraInfoRevealOverride
    property bool autopilotButtonEnabled: true
    readonly property bool _autopilotRevealOverride: _revealActiveKey === "autopilot"
    readonly property bool showAutopilot: autopilotButtonEnabled || _autopilotRevealOverride
    property bool consoleButtonEnabled: true
    readonly property bool _consoleRevealOverride: _revealActiveKey === "console"
    readonly property bool showConsole: consoleButtonEnabled || _consoleRevealOverride
    property int favoriteItemSpacing: Math.round(6 * root._s)
    property int favoriteListMaxHeight: Math.round(244 * root._s)
    property bool connectionsOnline: true
    property bool connectionStatusToolVisible: true
    property bool layoutEditing: false
    property string highlightedQuickActionKey: ""
    property int highlightPulseToken: 0
    readonly property string draggingKey: store ? store.quickActionDraggingKey : ""
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
    signal loggingIndicatorTriggered()
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
    property bool secondWindowButtonEnabled: true
    readonly property bool _secondWindowAvailable: Qt.platform.os !== "android" && Qt.platform.os !== "ios"
    readonly property bool _secondWindowRevealOverride: _revealActiveKey === "secondWindow"
    readonly property bool _showSecondWindow: _secondWindowAvailable && (secondWindowButtonEnabled || _secondWindowRevealOverride)

    // Devices bound from MainWindow (deviceManagerWrapper.devs).
    // Status colors mirror ConnectionViewer link-row palette.
    property var devices: []
    // Index into `devices` (= deviceManagerWrapper.devs) — devSN can collide.
    signal deviceTriggered(int devIndex)

    readonly property bool _hasConnectedDevice: {
        var ds = root.effectiveDevices
        if (!ds) return false
        for (var i = 0; i < ds.length; ++i)
            if (ds[i] && ds[i].devType !== 0) return true
        return false
    }
    property var _favSlot: null
    property var _btSlot: null

    readonly property bool _loggingActive: typeof core !== "undefined" && core && (core.loggingKlf || core.loggingCsv)
    property bool loggingButtonEnabled: true
    readonly property bool _loggingRevealOverride: _revealActiveKey === "logging"
    readonly property bool _loggingBadgeVisible: (loggingButtonEnabled && (_loggingActive || layoutEditing)) || _loggingRevealOverride

    readonly property bool _manualTesting: typeof manualTesting !== "undefined" && manualTesting === true

    QtObject {
        id: fakeDevice
        property int devType: 1
        property string devName: "Echosounder"
        property string fwVersion: "0.0"
        property int devSN: 0
        property bool isSonar: true
        property bool isDoppler: false
        property bool isUSBL: false
        property bool isUSBLBeacon: false
        property bool isRecorder: false
        property bool isTransducerSupport: true
        property int transFreq: 700
        property bool linkConnected: true
        property bool linkReceivesData: true
        property bool linkNotAvailable: false
    }

    readonly property var effectiveDevices: (devices && devices.length > 0)
                                            ? devices
                                            : (_manualTesting ? [fakeDevice] : [])

    QtObject {
        id: placeholderDevice
        property int devType: 1
        property string devName: qsTr("Device")
        property string fwVersion: ""
        property int devSN: 0
        property bool isSonar: true
        property bool isDoppler: false
        property bool isUSBL: false
        property bool isUSBLBeacon: false
        property bool isRecorder: false
        property bool isTransducerSupport: true
        property int transFreq: 700
        property bool linkConnected: true
        property bool linkReceivesData: true
        property bool linkNotAvailable: false
    }

    // While editing the layout: show a single device slot (first real device, or
    // a neutral placeholder if none). Otherwise the full device list.
    readonly property var _connSlotDevices: layoutEditing
        ? (effectiveDevices.length > 0 ? [effectiveDevices[0]] : [placeholderDevice])
        : effectiveDevices

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
        if (d.linkConnected)    return d.linkReceivesData ? AppPalette.linkOkBg : AppPalette.linkIdleBg
        if (d.linkNotAvailable) return AppPalette.linkDownBg
        return buttonFillColor
    }

    function linkBorderColor(d) {
        if (!d) return buttonBorderColor
        if (d.linkConnected)    return d.linkReceivesData ? AppPalette.linkOkBorder : AppPalette.linkIdleBorder
        if (d.linkNotAvailable) return AppPalette.linkDownBorder
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
    readonly property int panelOffsetX: (root.showToggleButton ? root.toggleButtonSize + Math.round(8 * root._s) : 0) + root.revealShiftX

    width: Math.max(leadingClusterWidth,
                    root.expanded
                    ? panelOffsetX + panel.width
                    : root.panelPaddingX + 2 * root.toggleButtonSize + Math.round(8 * root._s)
                      + (collapsedDeviceRow.visible ? Math.round(8 * root._s) + collapsedDeviceRow.width : 0))
    height: Math.max(leadingClusterHeight, panel.height, layoutsCombo.y + backing.height, btEditCombo.y + btEditCombo.height)

    component LayoutsTriggerButton: Rectangle {
        id: button

        property bool open: false
        property bool dropped: false       // dropdown open/animating → merge into container
        property bool highlighted: false   // "look here" reveal hint (layouts hotkey)
        property int flashToken: 0
        property bool highlightHold: false
        signal clicked()

        readonly property var currentEntry: root.store && root.store.favoriteLayoutEntryFromCurrent
                                            ? root.store.favoriteLayoutEntryFromCurrent()
                                            : null
        readonly property int iconH: root.controlHeight - Math.round(12 * root._s)
        readonly property int iconW: Math.round(button.iconH * 21 / 16)

        implicitWidth: root.triggerButtonWidth
        implicitHeight: root.controlHeight
        radius: height / 2
        color: button.dropped ? "transparent"
               : button.highlightHold ? AppPalette.accentBgStrong
               : (buttonMouse.containsMouse ? root.buttonHoverColor : root.buttonFillColor)
        border.width: button.dropped ? 0 : 1
        border.color: button.highlightHold ? AppPalette.accentBorder
                      : (buttonMouse.containsMouse ? root.buttonHoverBorderColor : root.buttonBorderColor)

        Behavior on color { ColorAnimation { duration: 120; easing.type: Easing.OutCubic } }

        // Hover whiteness — same feel as KCircleIconButton (the pencil trigger).
        Rectangle {
            anchors.fill: parent
            radius: parent.radius
            color: "#FFFFFF"
            opacity: buttonMouse.containsMouse ? 0.12 : 0.0
            visible: opacity > 0.001
            Behavior on opacity { NumberAnimation { duration: 110; easing.type: Easing.OutCubic } }
        }

        Rectangle {
            anchors.fill: parent
            radius: parent.radius
            color: AppPalette.accentBgStrong
            opacity: revealPulse.opacity
            visible: button.highlighted
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
            NumberAnimation { target: pulseRing; property: "opacity"; to: 0.95; duration: 90;  easing.type: Easing.OutCubic }
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

    component LoggingBadge: Item {
        id: logBadge
        visible: root._loggingBadgeVisible
        width: visible ? root.controlHeight : 0
        height: root.controlHeight

        readonly property bool _placeholder: !root._loggingActive
        readonly property bool _klf: _placeholder || (typeof core !== "undefined" && core && core.loggingKlf)
        readonly property bool _csv: !_placeholder && (typeof core !== "undefined" && core && core.loggingCsv)
        readonly property real _hoverScale: badgeMa.pressed ? 0.97 : (badgeMa.containsMouse ? 1.035 : 1.0)

        onVisibleChanged: if (!visible && pill.opened) pill.close()

        Rectangle {
            anchors.fill: parent
            radius: width / 2
            scale: logBadge._hoverScale
            color: badgeMa.containsMouse ? root.buttonHoverColor : root.buttonFillColor
            border.width: 1
            border.color: badgeMa.containsMouse ? Qt.lighter("#EF4444", 1.15) : "#EF4444"
            Behavior on color { ColorAnimation { duration: 110; easing.type: Easing.OutCubic } }
            Behavior on border.color { ColorAnimation { duration: 110; easing.type: Easing.OutCubic } }
            Behavior on scale { NumberAnimation { duration: 120; easing.type: Easing.OutCubic } }
        }

        Column {
            anchors.centerIn: parent
            scale: logBadge._hoverScale
            spacing: Math.round(1 * root._s)
            Behavior on scale { NumberAnimation { duration: 120; easing.type: Easing.OutCubic } }

            Rectangle {
                anchors.horizontalCenter: parent.horizontalCenter
                width: Math.round(8 * root._s)
                height: width
                radius: width / 2
                color: "#EF4444"

                SequentialAnimation on opacity {
                    running: logBadge.visible
                    loops: Animation.Infinite
                    NumberAnimation { to: 0.3; duration: 650; easing.type: Easing.InOutQuad }
                    NumberAnimation { to: 1.0; duration: 650; easing.type: Easing.InOutQuad }
                }
            }

            Column {
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: 0

                Text {
                    visible: logBadge._klf
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: "KLF"
                    color: "#EF4444"
                    font.pixelSize: Math.round(9 * root._s)
                    font.bold: true
                    lineHeight: 0.82
                    lineHeightMode: Text.ProportionalHeight
                }
                Text {
                    visible: logBadge._csv
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: "CSV"
                    color: "#EF4444"
                    font.pixelSize: Math.round(9 * root._s)
                    font.bold: true
                    lineHeight: 0.82
                    lineHeightMode: Text.ProportionalHeight
                }
            }
        }

        MouseArea {
            id: badgeMa
            anchors.fill: parent
            enabled: logBadge.visible
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: pill.opened ? pill.close() : pill.open()
        }

        KToolTip { text: qsTr("Recording"); shown: badgeMa.containsMouse && !pill.opened }

        readonly property bool _highlighted: root.highlightedQuickActionKey === "logging"

        Rectangle {
            id: logPulse
            anchors.fill: parent
            radius: width / 2
            color: "transparent"
            border.width: Math.max(2, Math.round(2 * root._s))
            border.color: AppPalette.accentBorder
            opacity: 0
            visible: logBadge._highlighted
            z: 10
        }

        SequentialAnimation {
            id: logPulseAnim
            NumberAnimation { target: logPulse; property: "opacity"; to: 0.95; duration: 90;  easing.type: Easing.OutCubic }
            NumberAnimation { target: logPulse; property: "opacity"; to: 0.30; duration: 180; easing.type: Easing.OutCubic }
            NumberAnimation { target: logPulse; property: "opacity"; to: 0.0;  duration: 280; easing.type: Easing.OutCubic }
        }

        Connections {
            target: root
            function onHighlightPulseTokenChanged() { if (logBadge._highlighted) logPulseAnim.restart() }
        }

        Timer {
            running: pill.visible
            interval: 1000
            repeat: true
            onTriggered: pill.refresh()
        }

        Popup {
            id: pill
            readonly property int pad: Math.round(4 * root._s)
            x: -pad
            y: -pad
            width: logBadge.width + 2 * pad
            padding: pad
            closePolicy: Popup.CloseOnPressOutside | Popup.CloseOnEscape

            property int sizeB: 0
            property int secs: 0
            function refresh() {
                if (typeof core === "undefined" || !core) return
                pill.sizeB = core.activeLogSizeBytes()
                pill.secs  = core.activeLogDurationSecs()
            }
            function fmtSize(b) {
                if (b < 1024) return b + " B"
                var kb = b / 1024
                if (kb < 1024) return (kb < 10 ? kb.toFixed(1) : Math.round(kb)) + " KB"
                var mb = kb / 1024
                if (mb < 1024) return (mb < 10 ? mb.toFixed(1) : Math.round(mb)) + " MB"
                var gb = mb / 1024
                return (gb < 10 ? gb.toFixed(1) : Math.round(gb)) + " GB"
            }
            function fmtTime(s) {
                var h = Math.floor(s / 3600), m = Math.floor((s % 3600) / 60), ss = s % 60
                function p(n) { return (n < 10 ? "0" : "") + n }
                return h > 0 ? (h + ":" + p(m) + ":" + p(ss)) : (p(m) + ":" + p(ss))
            }
            onOpened: refresh()

            background: Rectangle {
                color: AppPalette.bg
                radius: width / 2
                border.width: 1
                border.color: "#EF4444"
            }

            contentItem: Column {
                spacing: Math.round(5 * root._s)

                Rectangle {
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: logBadge.width; height: width
                    radius: width / 2
                    color: root.buttonFillColor
                    border.width: 1
                    border.color: "#EF4444"

                    Column {
                        anchors.centerIn: parent
                        spacing: Math.round(1 * root._s)

                        Rectangle {
                            anchors.horizontalCenter: parent.horizontalCenter
                            width: Math.round(8 * root._s); height: width
                            radius: width / 2
                            color: "#EF4444"
                            SequentialAnimation on opacity {
                                running: pill.visible
                                loops: Animation.Infinite
                                NumberAnimation { to: 0.3; duration: 650; easing.type: Easing.InOutQuad }
                                NumberAnimation { to: 1.0; duration: 650; easing.type: Easing.InOutQuad }
                            }
                        }
                        Column {
                            anchors.horizontalCenter: parent.horizontalCenter
                            spacing: 0
                            Text {
                                visible: logBadge._klf
                                anchors.horizontalCenter: parent.horizontalCenter
                                text: "KLF"; color: "#EF4444"
                                font.pixelSize: Math.round(9 * root._s); font.bold: true
                                lineHeight: 0.82; lineHeightMode: Text.ProportionalHeight
                            }
                            Text {
                                visible: logBadge._csv
                                anchors.horizontalCenter: parent.horizontalCenter
                                text: "CSV"; color: "#EF4444"
                                font.pixelSize: Math.round(9 * root._s); font.bold: true
                                lineHeight: 0.82; lineHeightMode: Text.ProportionalHeight
                            }
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: pill.close()
                    }
                }

                Rectangle {
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: logBadge.width; height: width
                    radius: width / 2
                    color: stopMa.containsMouse ? "#991B1B" : "#7F1D1D"
                    border.width: 1
                    border.color: "#EF4444"
                    Rectangle {
                        anchors.centerIn: parent
                        width: Math.round(logBadge.width * 0.3); height: width
                        radius: Math.round(2 * root._s)
                        color: "#FFFFFF"
                    }
                    MouseArea {
                        id: stopMa
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            if (root.store) root.store.setRecording(false)
                            pill.close()
                        }
                    }
                    KToolTip { text: qsTr("Stop recording"); shown: stopMa.containsMouse }
                }

                Rectangle {
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: logBadge.width; height: width
                    radius: width / 2
                    color: gearMa.containsMouse ? AppPalette.cardHover : AppPalette.card
                    border.width: 1
                    border.color: AppPalette.border
                    Image {
                        anchors.centerIn: parent
                        width: Math.round(logBadge.width * 0.5); height: width
                        source: "qrc:/icons/ui/settings.svg"
                        fillMode: Image.PreserveAspectFit
                        opacity: 0.85
                    }
                    MouseArea {
                        id: gearMa
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            root.loggingIndicatorTriggered()
                            pill.close()
                            root.expanded = false
                        }
                    }
                    KToolTip { text: qsTr("Recording settings"); shown: gearMa.containsMouse }
                }

                Column {
                    anchors.horizontalCenter: parent.horizontalCenter
                    spacing: 0
                    bottomPadding: Math.round(3 * root._s)
                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: pill.fmtSize(pill.sizeB)
                        color: AppPalette.text
                        font.pixelSize: Math.round(9 * root._s)
                        font.bold: true
                    }
                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: pill.fmtTime(pill.secs)
                        color: AppPalette.textMuted
                        font.pixelSize: Math.round(9 * root._s)
                    }
                }
            }
        }
    }

    Component {
        id: deviceShortcutDelegate

        Item {
            id: devBadge
            required property var modelData
            required property int index
            readonly property color _fill:   root.linkFillColor(modelData)
            readonly property color _border: root.linkBorderColor(modelData)
            readonly property bool _transducer: !!(modelData && modelData.isTransducerSupport)
            readonly property string _tip: modelData
                         ? (modelData.devName + " " + modelData.fwVersion + " [" + modelData.devSN + "]")
                         : ""

            visible: modelData ? (modelData.devType !== 0) : false
            width: visible ? root.controlHeight : 0
            height: root.controlHeight

            KCircleIconButton {
                anchors.fill: parent
                iconSource: root.iconForDevice(devBadge.modelData)
                iconTintColor: AppPalette.text
                toolTipText: devBadge._tip
                fillColor:        devBadge._fill
                fillHoverColor:   devBadge._fill
                fillPressedColor: root.buttonPressedColor
                borderColor:      devBadge._border
                borderHoverColor: devBadge._border

                highlighted: root.highlightedQuickActionKey === "connections"
                flashToken: root.highlightPulseToken
                highlightHold: root.draggingKey === "connections"

                onClicked: devPill.opened ? devPill.close() : devPill.open()

                Text {
                    visible: !!(devBadge.modelData && devBadge.modelData.isTransducerSupport && devBadge.modelData.transFreq > 0)
                    anchors.bottom: parent.bottom
                    anchors.right: parent.right
                    anchors.bottomMargin: Math.round(2 * root._s)
                    anchors.rightMargin: Math.round(4 * root._s)
                    text: (devBadge.modelData && devBadge.modelData.transFreq > 0) ? String(devBadge.modelData.transFreq) : ""
                    color: AppPalette.text
                    font.pixelSize: Math.round(9 * root._s)
                    font.bold: true
                    style: Text.Outline
                    styleColor: "#000000B0"
                }
            }

            Popup {
                id: devPill
                readonly property int pad: Math.round(4 * root._s)
                x: -pad
                y: -pad
                width: devBadge.width + 2 * pad
                padding: pad
                closePolicy: Popup.CloseOnPressOutside | Popup.CloseOnEscape

                background: Rectangle {
                    color: AppPalette.bg
                    radius: width / 2
                    border.width: 1
                    border.color: AppPalette.border
                }

                contentItem: Column {
                    spacing: Math.round(5 * root._s)

                    KCircleIconButton {
                        width: devBadge.width; height: width
                        iconSource: root.iconForDevice(devBadge.modelData)
                        iconTintColor: AppPalette.text
                        toolTipText: devBadge._tip
                        fillColor:        devBadge._fill
                        fillHoverColor:   devBadge._fill
                        borderColor:      devBadge._border
                        borderHoverColor: devBadge._border
                        onClicked: devPill.close()

                        Text {
                            visible: !!(devBadge.modelData && devBadge.modelData.isTransducerSupport && devBadge.modelData.transFreq > 0)
                            anchors.bottom: parent.bottom
                            anchors.right: parent.right
                            anchors.bottomMargin: Math.round(2 * root._s)
                            anchors.rightMargin: Math.round(4 * root._s)
                            text: (devBadge.modelData && devBadge.modelData.transFreq > 0) ? String(devBadge.modelData.transFreq) : ""
                            color: AppPalette.text
                            font.pixelSize: Math.round(9 * root._s)
                            font.bold: true
                            style: Text.Outline
                            styleColor: "#000000B0"
                        }
                    }

                    KCircleIconButton {
                        visible: devBadge._transducer
                        width: devBadge.width; height: width
                        glyph: "700"
                        glyphPixelSize: Math.round(11 * root._s)
                        readonly property bool _active: devBadge.modelData && devBadge.modelData.transFreq === 700
                        fillColor:      _active ? AppPalette.accentBgStrong : AppPalette.card
                        fillHoverColor: AppPalette.cardHover
                        borderColor:    _active ? AppPalette.accentBorder : AppPalette.border
                        toolTipText: qsTr("Set 700 kHz")
                        onClicked: {
                            if (devBadge.modelData) devBadge.modelData.transFreq = 700
                            devPill.close()
                        }
                    }

                    KCircleIconButton {
                        visible: devBadge._transducer
                        width: devBadge.width; height: width
                        glyph: "450"
                        glyphPixelSize: Math.round(11 * root._s)
                        readonly property bool _active: devBadge.modelData && devBadge.modelData.transFreq === 450
                        fillColor:      _active ? AppPalette.accentBgStrong : AppPalette.card
                        fillHoverColor: AppPalette.cardHover
                        borderColor:    _active ? AppPalette.accentBorder : AppPalette.border
                        toolTipText: qsTr("Set 450 kHz")
                        onClicked: {
                            if (devBadge.modelData) devBadge.modelData.transFreq = 450
                            devPill.close()
                        }
                    }

                    KCircleIconButton {
                        width: devBadge.width; height: width
                        iconSource: "qrc:/icons/ui/settings.svg"
                        iconTintColor: AppPalette.text
                        fillColor: AppPalette.card
                        fillHoverColor: AppPalette.cardHover
                        borderColor: AppPalette.border
                        toolTipText: qsTr("Device settings")
                        onClicked: {
                            root.deviceTriggered(devBadge.index)
                            devPill.close()
                            root.expanded = false
                        }
                    }
                }
            }
        }
    }

    Component {
        id: qaConnectionsComp
        Row {
            spacing: Math.round(8 * root._s)
            height: root.controlHeight
            Repeater {
                model: root._connSlotDevices
                delegate: deviceShortcutDelegate
            }
        }
    }

    Component {
        id: qaLoggingComp
        LoggingBadge {}
    }

    Component {
        id: qaSecondWindowComp
        KCircleIconButton {
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
            highlighted: root.highlightedQuickActionKey === "secondWindow"
            flashToken: root.highlightPulseToken
            onClicked: {
                root.secondWindowToggleRequested()
                root.expanded = false
            }
        }
    }

    Component {
        id: qaFavoritesComp
        Item {
            id: favSlotItem
            width: root.triggerButtonWidth
            height: root.controlHeight
            Component.onCompleted: root._favSlot = favSlotItem
            Component.onDestruction: if (root._favSlot === favSlotItem) root._favSlot = null
        }
    }

    Component {
        id: qaBottomTrackComp
        Item {
            id: btSlotItem
            width: root.controlHeight
            height: root.controlHeight
            Component.onCompleted: root._btSlot = btSlotItem
            Component.onDestruction: if (root._btSlot === btSlotItem) root._btSlot = null
        }
    }

    Component {
        id: qaExtraInfoComp
        KCircleIconButton {
            id: extraInfoBtn
            readonly property bool _open: root.store && root.store.extraInfoVisible
            width: root.controlHeight
            height: root.controlHeight
            iconSource: "qrc:/icons/ui/list-details.svg"
            iconTintColor: AppPalette.text
            toolTipText: _open ? qsTr("Hide extra info") : qsTr("Extra info panel")
            fillColor:        _open ? AppPalette.accentBgStrong : root.buttonFillColor
            fillHoverColor:   _open ? AppPalette.accentBorder : root.buttonHoverColor
            fillPressedColor: root.buttonPressedColor
            borderColor:      _open ? AppPalette.accentBorder : root.buttonBorderColor
            borderHoverColor: _open ? AppPalette.accentBorder : root.buttonHoverBorderColor
            highlighted: root.highlightedQuickActionKey === "extraInfo"
            flashToken: root.highlightPulseToken
            highlightHold: root.draggingKey === "extraInfo"
            onClicked: if (root.store) root.store.extraInfoVisible = !root.store.extraInfoVisible

            KCloseBadge { visible: extraInfoBtn._open }
        }
    }

    Component {
        id: qaAutopilotComp
        KCircleIconButton {
            id: autopilotBtn
            readonly property bool _open: root.store && root.store.autopilotEnabled
            width: root.controlHeight
            height: root.controlHeight
            iconSource: "qrc:/icons/ui/autopilot.svg"
            iconTintColor: AppPalette.text
            toolTipText: _open ? qsTr("Hide autopilot panel") : qsTr("Autopilot panel")
            fillColor:        _open ? AppPalette.accentBgStrong : root.buttonFillColor
            fillHoverColor:   _open ? AppPalette.accentBorder : root.buttonHoverColor
            fillPressedColor: root.buttonPressedColor
            borderColor:      _open ? AppPalette.accentBorder : root.buttonBorderColor
            borderHoverColor: _open ? AppPalette.accentBorder : root.buttonHoverBorderColor
            highlighted: root.highlightedQuickActionKey === "autopilot"
            flashToken: root.highlightPulseToken
            highlightHold: root.draggingKey === "autopilot"
            onClicked: if (root.store) root.store.autopilotEnabled = !root.store.autopilotEnabled

            KCloseBadge { visible: autopilotBtn._open }
        }
    }

    Component {
        id: qaConsoleComp
        KCircleIconButton {
            id: consoleBtn
            readonly property bool _open: typeof theme !== "undefined" && theme && theme.consoleVisible
            width: root.controlHeight
            height: root.controlHeight
            iconSource: "qrc:/icons/ui/terminal.svg"
            iconTintColor: AppPalette.text
            toolTipText: _open ? qsTr("Hide console") : qsTr("Console")
            fillColor:        _open ? AppPalette.accentBgStrong : root.buttonFillColor
            fillHoverColor:   _open ? AppPalette.accentBorder : root.buttonHoverColor
            fillPressedColor: root.buttonPressedColor
            borderColor:      _open ? AppPalette.accentBorder : root.buttonBorderColor
            borderHoverColor: _open ? AppPalette.accentBorder : root.buttonHoverBorderColor
            highlighted: root.highlightedQuickActionKey === "console"
            flashToken: root.highlightPulseToken
            highlightHold: root.draggingKey === "console"
            onClicked: if (typeof theme !== "undefined" && theme) theme.consoleVisible = !theme.consoleVisible

            KCloseBadge { visible: consoleBtn._open }
        }
    }

    Component {
        id: qaProfilesComp
        KCircleIconButton {
            id: profilesBtn
            readonly property bool _open: root.store && root.store.profilesPopupOpen
            width: root.controlHeight
            height: root.controlHeight
            iconSource: "qrc:/icons/ui/file_settings.svg"
            iconTintColor: AppPalette.text
            toolTipText: _open ? qsTr("Close profiles") : qsTr("Settings profiles")
            fillColor:        _open ? AppPalette.accentBgStrong : root.buttonFillColor
            fillHoverColor:   _open ? AppPalette.accentBorder : root.buttonHoverColor
            fillPressedColor: root.buttonPressedColor
            borderColor:      _open ? AppPalette.accentBorder : root.buttonBorderColor
            borderHoverColor: _open ? AppPalette.accentBorder : root.buttonHoverBorderColor
            highlighted: root.highlightedQuickActionKey === "profiles"
            flashToken: root.highlightPulseToken
            highlightHold: root.draggingKey === "profiles"
            onClicked: if (root.store) root.store.profilesPopupOpen = !root.store.profilesPopupOpen

            KCloseBadge { visible: profilesBtn._open }
        }
    }

    Connections {
        target: core
        function onActiveTransientUiChanged(who) {
            if (who !== root && root.showToggleButton) {
                root.expanded = false
                root.layoutsMenuOpen = false
            }
        }
    }

    // "K" — opens app settings.
    KCircleIconButton {
        id: toggleButton
        anchors.left: parent.left
        anchors.leftMargin: root.panelPaddingX  // equal top/left inset: matches the centered y-offset (== panelPaddingX)
        visible: root.showToggleButton
        // Icon-sized (matches the row's icon buttons), vertically centered in the panel band.
        width: visible ? root.toggleButtonSize : 0
        height: visible ? root.toggleButtonSize : 0
        y: Math.round((root.panelHeight - height) / 2)
        iconSource: "qrc:/icons/app/kogger_app.png"
        iconTintColor: AppPalette.accentBar
        iconPixelSize: Math.round(root.toggleButtonSize * 0.7)
        fillColor: root.buttonFillColor
        fillHoverColor: root.buttonHoverColor
        fillPressedColor: root.buttonPressedColor
        borderColor: root.buttonBorderColor
        borderHoverColor: root.buttonHoverBorderColor
        toolTipText: qsTr("Settings")

        onClicked: {
            if (!root.showToggleButton)
                return
            if (typeof core !== "undefined" && core) core.requestDismissTransientUi()
            root.settingsTriggered()
        }
    }

    KCircleIconButton {
        id: expandButton
        anchors.left: toggleButton.right
        anchors.leftMargin: Math.round(8 * root._s)
        y: toggleButton.y
        z: 2   // above the panel background so it sits inside the wrap when expanded
        visible: root.showToggleButton
        width: visible ? root.toggleButtonSize : 0
        height: visible ? root.toggleButtonSize : 0
        iconSource: "qrc:/icons/ui/chevron-right.svg"
        iconTintColor: AppPalette.text
        iconPixelSize: Math.round(root.toggleButtonSize * 0.55)
        iconRotation: root.expanded ? 0 : 90   // collapsed → down, expanded → toward the menu (right)
        fillColor: root.buttonFillColor
        fillHoverColor: root.buttonHoverColor
        fillPressedColor: root.buttonPressedColor
        borderColor: root.buttonBorderColor
        borderHoverColor: root.buttonHoverBorderColor
        toolTipText: root.expanded ? qsTr("Collapse hotkeys") : qsTr("Open hotkeys")

        onClicked: {
            if (!root.showToggleButton)
                return
            if (root.expanded) {
                root.expanded = false
            } else {
                if (typeof core !== "undefined" && core) core.setActiveTransientUi(root)
                root.expanded = true
            }
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
        anchors.leftMargin: root.panelPaddingX + 2 * root.toggleButtonSize + 2 * Math.round(8 * root._s)
        anchors.verticalCenter: toggleButton.verticalCenter
        spacing: Math.round(8 * root._s)
        visible: root.showToggleButton && !root.expanded && (root.connectionStatusToolVisible || root._loggingBadgeVisible)
        opacity: visible ? 1 : 0

        Behavior on opacity {
            NumberAnimation { duration: 170; easing.type: Easing.OutCubic }
        }

        Repeater {
            model: root.connectionStatusToolVisible ? root.effectiveDevices : 0
            delegate: deviceShortcutDelegate
        }

        LoggingBadge {}
    }

    Rectangle {
        id: panel
        anchors.left: parent.left
        anchors.leftMargin: panelOffsetX
        anchors.top: parent.top
        width: root.expanded ? Math.min(root.maxExpandedWidth,
                                        2 * root.panelPaddingX + root._arrowSlotW + topRow.implicitWidth)
                             : 0
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

        MouseArea {
            anchors.fill: parent
            enabled: root.expanded
            acceptedButtons: Qt.AllButtons
            hoverEnabled: false
            onPressed: function(mouse) {
                if (typeof core !== "undefined" && core) core.setActiveTransientUi(root)
                mouse.accepted = true
            }
            onWheel: function(wheel) { wheel.accepted = true }
        }

        Row {
            id: topRow
            anchors.left: parent.left
            anchors.leftMargin: root.panelPaddingX + root._arrowSlotW
            anchors.verticalCenter: parent.verticalCenter
            spacing: Math.round(8 * root._s)
            height: root.controlHeight

            move: Transition {
                NumberAnimation { properties: "x"; duration: 220; easing.type: Easing.OutCubic }
            }

            Repeater {
                model: root.store ? root.store.quickActionOrderModel : 0
                delegate: Loader {
                    required property string key
                    height: root.controlHeight
                    visible: key === "connections" ? ((root.connectionStatusToolVisible || root._revealActiveKey === "connections") && (root._hasConnectedDevice || root.layoutEditing))
                           : key === "logging"     ? root._loggingBadgeVisible
                           : key === "favorites"   ? root.hasFavoriteLayouts
                           : key === "bottomTrack" ? root.showBtEdit
                           : key === "extraInfo"   ? root.showExtraInfo
                           : key === "autopilot"    ? root.showAutopilot
                           : key === "console"      ? root.showConsole
                           : key === "profiles"     ? root.showProfiles
                           : key === "secondWindow" ? root._showSecondWindow
                           : false
                    active: visible
                    sourceComponent: key === "connections" ? qaConnectionsComp
                                   : key === "logging"      ? qaLoggingComp
                                   : key === "favorites"    ? qaFavoritesComp
                                   : key === "bottomTrack"  ? qaBottomTrackComp
                                   : key === "extraInfo"    ? qaExtraInfoComp
                                   : key === "autopilot"    ? qaAutopilotComp
                                   : key === "console"      ? qaConsoleComp
                                   : key === "profiles"     ? qaProfilesComp
                                   : key === "secondWindow" ? qaSecondWindowComp
                                   : null
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
        x: panel.x + topRow.x + (root._favSlot && root._favSlot.parent ? root._favSlot.parent.x : 0) - layoutsCombo.sidePad
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
            highlightHold: root.draggingKey === "favorites"
            onClicked: root.layoutsMenuOpen = !root.layoutsMenuOpen
        }
    }

    Item {
        id: btEditCombo
        readonly property int sidePad: Math.round(3 * root._s)
        visible: root.showBtEdit && panel.opacity > 0.01
        opacity: panel.opacity
        x: panel.x + topRow.x + (root._btSlot && root._btSlot.parent ? root._btSlot.parent.x : 0) - btEditCombo.sidePad
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
            iconSource: "qrc:/icons/ui/pencil.svg"
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
            highlightHold: root.draggingKey === "bottomTrack"
            onClicked: {
                if (!root.store) return
                root.store.bottomTrackEditorOpen = !root.store.bottomTrackEditorOpen   // keep the menu open
            }

            KCloseBadge { visible: btEditTrigger._open }
        }
    }

    MouseArea {
        anchors.fill: panel
        z: panel.z + 2
        visible: !root.showToggleButton && panel.opacity > 0.01
        enabled: visible
        hoverEnabled: true
        acceptedButtons: Qt.AllButtons
        cursorShape: Qt.ArrowCursor
        preventStealing: true
        onPressed: function(mouse) { mouse.accepted = true }
        onReleased: function(mouse) { mouse.accepted = true }
        onClicked: function(mouse) { mouse.accepted = true }
        onDoubleClicked: function(mouse) { mouse.accepted = true }
        onWheel: function(wheel) { wheel.accepted = true }
    }

}

