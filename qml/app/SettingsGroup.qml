import QtQuick 2.15
import kqml_types 1.0

Item {
    id: root

    property string title: ""
    // Optional short subtitle rendered between header bar and content card.
    property string description: ""
    property bool collapsible: true
    property bool collapsedByDefault: true
    property bool expanded: !collapsedByDefault
    property var stateStore: null
    property string stateKey: ""
    property real preferredWidth: 250
    property int contentSpacing: Tokens.spaceMd
    // Inner padding of the dark content card.
    property int contentPadding: Tokens.spaceMd
    property color titleColor: AppPalette.text
    property int titlePixelSize: Math.round(15 * AppPalette.scale)
    // When false, the group header turns red — used to signal that a setting in
    // the group has been changed locally but the device hasn't acked yet.
    property bool confirmed: true
    default property alias contentData: contentColumn.data

    property bool _stateReady: false

    width: preferredWidth
    implicitWidth: preferredWidth
    implicitHeight: contentWrapper.implicitHeight

    function loadExpandedState() {
        if (!stateStore || typeof stateStore.isSettingsGroupExpanded !== "function") {
            _stateReady = true
            return
        }

        var key = typeof stateKey === "string" ? stateKey.trim() : ""
        if (key === "") {
            _stateReady = true
            return
        }

        expanded = stateStore.isSettingsGroupExpanded(key)
        _stateReady = true
    }

    onStateStoreChanged: {
        _stateReady = false
        loadExpandedState()
    }

    onStateKeyChanged: {
        _stateReady = false
        loadExpandedState()
    }

    onExpandedChanged: {
        if (_stateReady && stateStore && typeof stateStore.setSettingsGroupExpanded === "function") {
            var key = typeof stateKey === "string" ? stateKey.trim() : ""
            if (key !== "")
                stateStore.setSettingsGroupExpanded(key, expanded)
        }
        if (expanded)
            scrollIntoViewTimer.restart()
        else
            scrollIntoViewTimer.stop()
    }

    Connections {
        target: root.stateStore
        ignoreUnknownSignals: true

        function onSettingsGroupExpandedMapChanged() {
            root.loadExpandedState()
        }
    }

    Component.onCompleted: {
        loadExpandedState()
        if (stateStore && typeof stateStore.registerSettingsGroup === "function")
            stateStore.registerSettingsGroup(root)
        if (stateStore
                && typeof stateStore.pendingScrollGroupKey === "string"
                && stateStore.pendingScrollGroupKey === stateKey) {
            pendingTopScrollTimer.restart()
        }
    }

    Component.onDestruction: {
        if (stateStore && typeof stateStore.unregisterSettingsGroup === "function")
            stateStore.unregisterSettingsGroup(root)
    }

    // ── Auto-scroll-into-view on expand ──────────────────────────────────
    // When the user opens a group at the bottom of the panel, scroll the
    // enclosing Flickable so the whole group ends up visible. Uses a small
    // Timer so the scroll animation runs in parallel with the card expand.
    Timer {
        id: scrollIntoViewTimer
        interval: 60
        repeat: false
        onTriggered: root._scrollIntoView()
    }

    Timer {
        id: pendingTopScrollTimer
        interval: 220
        repeat: false
        onTriggered: {
            root._scrollToTop()
            if (root.stateStore
                    && root.stateStore.pendingScrollGroupKey === root.stateKey)
                root.stateStore.pendingScrollGroupKey = ""
        }
    }

    Connections {
        target: root.stateStore
        ignoreUnknownSignals: true
        function onPendingScrollGroupKeyChanged() {
            // Group already alive when openAppSettingsAtGroup() ran with no
            // prior registry hit — pick up the request now.
            if (root.stateStore
                    && root.stateStore.pendingScrollGroupKey === root.stateKey
                    && root.stateKey !== "") {
                pendingTopScrollTimer.restart()
            }
        }
    }

    function _findAncestorFlickable() {
        var item = root.parent
        while (item) {
            if (item.contentY !== undefined
                    && item.contentHeight !== undefined
                    && item.flickableDirection !== undefined)
                return item
            item = item.parent
        }
        return null
    }

    // Predict the height the group will have once expanded — the binding on
    // contentCard.height animates over 200ms, so reading root.height at the
    // moment of expand would give the intermediate value.
    function _predictedFullHeight() {
        // Predict height in the EXPANDED state, even if some children are
        // currently invisible because of the opacity-fade (descriptionLabel)
        // or zero-height collapse (contentCard). Sum the implicit heights they
        // will have once the expand animation completes.
        var h = 0
        var countedKids = 0
        for (var i = 0; i < contentWrapper.children.length; ++i) {
            var c = contentWrapper.children[i]
            if (!c) continue
            var ch = 0
            if (c === contentCard) {
                ch = contentColumn.implicitHeight + 2 * root.contentPadding
            } else if (c === descriptionLabel) {
                // Description label fades via opacity 0/1; its visible flag
                // is false while collapsed, but implicitHeight is still the
                // text content height — include it iff there IS a description.
                ch = root.description.length > 0 ? c.implicitHeight : 0
            } else if (c.visible) {
                ch = c.implicitHeight > 0 ? c.implicitHeight : c.height
            }
            if (ch > 0) {
                h += ch
                countedKids++
            }
        }
        if (countedKids > 1)
            h += (countedKids - 1) * contentWrapper.spacing
        return h
    }

    function _scrollToTop() {
        var flick = _findAncestorFlickable()
        if (!flick) return

        var topInContent = root.mapToItem(flick.contentItem, 0, 0).y
        var target = Math.max(0, topInContent - Tokens.spaceLg)
        // Predict content height in case the expand animation hasn't finished
        // and the Flickable hasn't grown its contentHeight yet.
        var fullH = _predictedFullHeight()
        var predictedContentH = Math.max(flick.contentHeight,
                                         topInContent + fullH + Tokens.spaceLg)
        target = Math.min(target, Math.max(0, predictedContentH - flick.height))

        if (Math.abs(target - flick.contentY) < 0.5) return

        scrollIntoViewAnim.target = flick
        scrollIntoViewAnim.from = flick.contentY
        scrollIntoViewAnim.to = target
        scrollIntoViewAnim.restart()
    }

    function _scrollIntoView() {
        var flick = _findAncestorFlickable()
        if (!flick) return

        var topInContent = root.mapToItem(flick.contentItem, 0, 0).y
        var fullH = _predictedFullHeight()
        var bottomInContent = topInContent + fullH

        var viewBottom = flick.contentY + flick.height
        if (bottomInContent <= viewBottom + 0.5)
            return  // already fully visible — nothing to do

        // Scroll just enough to fit the group bottom (with a small breath).
        var target = bottomInContent - flick.height + Tokens.spaceLg
        // Never scroll past the group's top (otherwise its header disappears).
        target = Math.min(target, topInContent)
        // Predict the new contentHeight after card expansion finishes.
        var predictedContentH = Math.max(flick.contentHeight,
                                         topInContent + fullH + Tokens.spaceLg)
        target = Math.max(0, Math.min(target, predictedContentH - flick.height))

        if (Math.abs(target - flick.contentY) < 0.5) return

        scrollIntoViewAnim.target = flick
        scrollIntoViewAnim.from = flick.contentY
        scrollIntoViewAnim.to = target
        scrollIntoViewAnim.restart()
    }

    NumberAnimation {
        id: scrollIntoViewAnim
        property: "contentY"
        duration: 240
        easing.type: Easing.OutCubic
    }

    // Danger underlay — tints the whole group (header + content) when unconfirmed.
    Rectangle {
        anchors.fill: parent
        visible: !root.confirmed
        color: AppPalette.dangerBg
        border.color: AppPalette.dangerBorder
        border.width: 1
        radius: Tokens.radiusLg
        z: -1
    }

    Column {
        id: contentWrapper

        width: root.width
        spacing: Tokens.spaceSm

        // ── Header bar ────────────────────────────────────────────────────
        Rectangle {
            visible: root.title !== ""
            width: parent.width
            height: Math.round(36 * AppPalette.scale)
            radius: Tokens.radiusLg
            color: !root.confirmed
                   ? AppPalette.dangerBg
                   : (headerMouse.containsMouse ? AppPalette.bgDeep : AppPalette.card)
            border.width: 1
            border.color: !root.confirmed
                          ? AppPalette.dangerBorder
                          : (root.expanded ? AppPalette.borderFocus : AppPalette.border)

            Rectangle {
                width: Math.round(4 * AppPalette.scale)
                height: parent.height - Math.round(10 * AppPalette.scale)
                radius: Math.round(2 * AppPalette.scale)
                anchors.left: parent.left
                anchors.leftMargin: Math.round(5 * AppPalette.scale)
                anchors.verticalCenter: parent.verticalCenter
                color: !root.confirmed
                       ? AppPalette.dangerBorder
                       : (root.expanded ? AppPalette.accentBar : (headerMouse.containsMouse ? AppPalette.borderFocus : AppPalette.borderHover))
            }

            Row {
                anchors.fill: parent
                anchors.leftMargin: Tokens.spaceXl
                anchors.rightMargin: Tokens.spaceLg
                spacing: Tokens.spaceMd

                DisclosureIndicator {
                    anchors.verticalCenter: parent.verticalCenter
                    width: Math.round(10 * AppPalette.scale)
                    height: Math.round(10 * AppPalette.scale)
                    expanded: root.expanded
                    indicatorColor: AppPalette.textSecond
                    visible: root.collapsible
                }

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: root.title
                    color: root.titleColor
                    font.pixelSize: Math.max(Math.round(16 * AppPalette.scale), root.titlePixelSize)
                    font.bold: true
                }
            }

            MouseArea {
                id: headerMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: root.collapsible ? Qt.PointingHandCursor : Qt.ArrowCursor
                onClicked: {
                    if (root.collapsible)
                        root.expanded = !root.expanded
                }
            }
        }

        // ── Optional subtitle / description ──────────────────────────────
        // Opacity-fades together with the card for a smooth expand/collapse.
        Text {
            id: descriptionLabel
            visible: root.description.length > 0 && opacity > 0.01
            text: root.description
            color: AppPalette.textMuted
            font.pixelSize: Tokens.fontSm
            wrapMode: Text.WordWrap
            width: parent.width
            leftPadding: Tokens.spaceXxs
            opacity: (!root.collapsible || root.expanded) ? 1.0 : 0.0
            Behavior on opacity {
                NumberAnimation { duration: 160; easing.type: Easing.OutCubic }
            }
        }

        // ── Dark rounded card containing all group content ───────────────
        // Animates its height between 0 (collapsed) and full content height
        // (expanded). `clip: true` hides content during the shrink phase so
        // children don't peek above/below the card border.
        Rectangle {
            id: contentCard
            clip: true
            width: parent.width
            height: (!root.collapsible || root.expanded)
                    ? contentColumn.implicitHeight + 2 * root.contentPadding
                    : 0
            visible: height > 0.5
            radius: Tokens.radiusLg
            color: AppPalette.bgDeep
            border.color: AppPalette.border
            border.width: 1

            Behavior on height {
                NumberAnimation {
                    duration: 200
                    easing.type: Easing.OutCubic
                }
            }

            Column {
                id: contentColumn
                x: root.contentPadding
                y: root.contentPadding
                width: parent.width - 2 * root.contentPadding
                spacing: root.contentSpacing
            }
        }
    }
}
