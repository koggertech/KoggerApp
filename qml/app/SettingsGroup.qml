import QtQuick 2.15
import kqml_types 1.0

Item {
    id: root

    property string title: ""
    // Optional short subtitle rendered between header bar and content card.
    property string description: ""
    property bool collapsible: true
    property bool expandable: true   // false → header-only: no chevron/expand/body (description still shown)
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
    property alias headerActions: headerActionsRow.data
    readonly property int _headerH: Math.round(36 * AppPalette.scale)
    readonly property int headerActionSize: _headerH
    readonly property bool _bodyShown: expandable && (!collapsible || expanded)

    property bool _stateReady: false

    width: preferredWidth
    implicitWidth: preferredWidth
    implicitHeight: island.height

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

    // Predict the height the group will have once expanded — island.height
    // animates, so reading root.height mid-expand gives an intermediate value.
    // bodyCol is always laid out (clipped when collapsed), so its implicitHeight
    // already reflects the expanded state.
    function _predictedFullHeight() {
        return root._headerH + Tokens.spaceSm + bodyCol.implicitHeight + root.contentPadding
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

    Rectangle {
        id: island
        width: root.width
        clip: true
        radius: Tokens.radiusLg

        // Header colour flows down into the content background — one island.
        readonly property color _headerColor: !root.confirmed
                ? AppPalette.dangerBg
                : (headerMouse.containsMouse ? AppPalette.cardHover : AppPalette.card)
        readonly property color _bottomColor: !root.confirmed
                ? AppPalette.dangerBg
                : (root._bodyShown ? AppPalette.bgDeep : island._headerColor)

        height: root._bodyShown
                ? headerRow.height + Tokens.spaceSm + bodyCol.implicitHeight + root.contentPadding
                : headerRow.height

        Behavior on height {
            NumberAnimation { duration: Anim.disclosureMs; easing.type: Anim.disclosureEasing }
        }

        // Gradient blends flush from the header's bottom edge down over one row
        // (header colour → content bg), not spread over the whole group.
        // Non-linear: two curved mid-stops make the header colour decay FAST near
        // the top, then ease into the content bg (ease-out, not a straight blend).
        readonly property real _seamStart: Math.min(1, headerRow.height / Math.max(1, height))
        readonly property real _seamEnd: Math.min(1, (headerRow.height * 2) / Math.max(1, height))
        readonly property real _seamSpan: _seamEnd - _seamStart
        function _mix(a, b, t) {
            return Qt.rgba(a.r + (b.r - a.r) * t,
                           a.g + (b.g - a.g) * t,
                           a.b + (b.b - a.b) * t,
                           a.a + (b.a - a.a) * t)
        }

        gradient: Gradient {
            GradientStop { position: 0.0;               color: island._headerColor
                Behavior on color { ColorAnimation { duration: 110; easing.type: Easing.OutCubic } } }
            GradientStop { position: island._seamStart; color: island._headerColor
                Behavior on color { ColorAnimation { duration: 110; easing.type: Easing.OutCubic } } }
            GradientStop {
                position: island._seamStart + island._seamSpan * 0.25
                color: island._mix(island._headerColor, island._bottomColor, 0.58)
                Behavior on color { ColorAnimation { duration: 110; easing.type: Easing.OutCubic } }
            }
            GradientStop {
                position: island._seamStart + island._seamSpan * 0.55
                color: island._mix(island._headerColor, island._bottomColor, 0.91)
                Behavior on color { ColorAnimation { duration: 110; easing.type: Easing.OutCubic } }
            }
            GradientStop { position: island._seamEnd;   color: island._bottomColor
                Behavior on color { ColorAnimation { duration: Anim.disclosureMs; easing.type: Anim.disclosureEasing } } }
            GradientStop { position: 1.0;               color: island._bottomColor
                Behavior on color { ColorAnimation { duration: Anim.disclosureMs; easing.type: Anim.disclosureEasing } } }
        }

        // ── Header row ────────────────────────────────────────────────────
        Item {
            id: headerRow
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: root._headerH

            activeFocusOnTab: root.collapsible
            Keys.onReturnPressed: if (root.collapsible && root.expandable) root.expanded = !root.expanded
            Keys.onEnterPressed:  if (root.collapsible && root.expandable) root.expanded = !root.expanded
            Keys.onSpacePressed:  if (root.collapsible && root.expandable) root.expanded = !root.expanded

            KFocusRing { id: focusRing; radius: Tokens.radiusLg }

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
                id: headerTitleRow
                anchors.fill: parent
                anchors.leftMargin: Tokens.spaceXl
                // Reserve room on the right for the header action buttons so the title never runs under them.
                anchors.rightMargin: Tokens.spaceLg
                                     + (headerActionsRow.width > 0 ? headerActionsRow.width + Tokens.spaceMd : 0)
                spacing: Tokens.spaceMd

                DisclosureIndicator {
                    anchors.verticalCenter: parent.verticalCenter
                    width: Math.round(10 * AppPalette.scale)
                    height: Math.round(10 * AppPalette.scale)
                    expanded: root.expanded
                    indicatorColor: AppPalette.textSecond
                    visible: root.collapsible
                    opacity: root.expandable ? 1.0 : 0.0   // keep the slot (title stays put), just hide the arrow
                }

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    width: Math.max(0, parent.width - (root.collapsible ? Math.round(10 * AppPalette.scale) + parent.spacing : 0))
                    text: root.title
                    color: root.titleColor
                    font.pixelSize: Math.max(Math.round(16 * AppPalette.scale), root.titlePixelSize)
                    font.bold: true
                    elide: Text.ElideRight
                }
            }

            MouseArea {
                id: headerMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: (root.collapsible && root.expandable) ? Qt.PointingHandCursor : Qt.ArrowCursor
                onPressed: if (root.collapsible && root.expandable) focusRing.suppress()
                onClicked: {
                    if (root.collapsible && root.expandable) {
                        headerRow.forceActiveFocus()
                        root.expanded = !root.expanded
                    }
                }
            }

            Row {
                id: headerActionsRow
                z: 1
                anchors.right: parent.right
                anchors.rightMargin: 0                    // flush right — rounded chips
                anchors.verticalCenter: parent.verticalCenter
                spacing: Tokens.spaceSm
            }
        }

        // ── Body: description + content (clipped while collapsed) ─────────
        // Kept laid out (not visible:false) while collapsed so its implicitHeight
        // stays valid for the expand animation and scroll prediction; `enabled`
        // instead of `visible` keeps the clipped controls out of the Tab chain
        // and input without zeroing that implicitHeight.
        Column {
            id: bodyCol
            anchors.top: headerRow.bottom
            anchors.topMargin: Tokens.spaceSm
            x: root.contentPadding
            width: island.width - 2 * root.contentPadding
            spacing: Tokens.spaceSm
            enabled: root._bodyShown

            Text {
                id: descriptionLabel
                visible: root.description.length > 0
                text: root.description
                color: AppPalette.textMuted
                font.pixelSize: Tokens.fontSm
                wrapMode: Text.WordWrap
                width: parent.width
            }

            Column {
                id: contentColumn
                width: parent.width
                spacing: root.contentSpacing
            }
        }
    }

    // Border drawn on a separate, non-clipped overlay — a bordered Rectangle
    // with clip:true (the island) loses ~1px of its own border to the clip.
    Rectangle {
        anchors.fill: island
        color: "transparent"
        radius: island.radius
        border.width: root.confirmed ? 0 : 1
        border.color: AppPalette.dangerBorder
    }
}
