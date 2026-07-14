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
    property bool bodyAnimated: true   // false → expand/collapse snaps (programmatic, no flicker)
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
    property bool showAccentBar: true
    property bool uniformBody: false
    property bool scrollIntoViewOnExpand: true
    default property alias contentData: contentColumn.data
    property alias headerActions: headerActionsRow.data
    readonly property int _headerH: Math.round(36 * AppPalette.scale)
    readonly property int headerActionSize: _headerH
    readonly property bool _bodyShown: expandable && (!collapsible || expanded)

    // ── Sticky header ─────────────────────────────────────────────────────
    // When an expanded group is taller than the viewport, its header floats down
    // within the (clipped) island to stay pinned at the top of the scroll area,
    // then scrolls away once the group's bottom passes. `_flick` is the enclosing
    // Flickable; the binding re-evaluates on scroll (contentY/contentHeight) and
    // on the group's own height change.
    property var _flick: null
    readonly property real _stickyHeaderY: {
        if (!_flick || !root._bodyShown)
            return 0
        var _dep = _flick.contentY + _flick.contentHeight   // re-eval on scroll / layout
        var vpY = root.mapToItem(_flick, 0, 0).y            // group top in the viewport
        var maxY = Math.max(0, island.height - root._headerH)
        return Math.max(0, Math.min(maxY, -vpY))
    }
    // Fade the (sticky) header out over the last header-height of the group, so it
    // dissolves as the group's content scrolls past the top instead of jumping off.
    readonly property real _headerFade: {
        if (!_flick || !root._bodyShown)
            return 1
        var _dep = _flick.contentY + _flick.contentHeight
        var bottomVp = root.mapToItem(_flick, 0, island.height).y   // group bottom in viewport
        return Math.max(0, Math.min(1, bottomVp / Math.max(1, root._headerH)))
    }
    readonly property bool _pinningToTop: scrollIntoViewAnim.running
    readonly property real _headerTopInFlick: {
        if (!_flick || !root._bodyShown)
            return 1e6
        var _dep = _flick.contentY + _flick.contentHeight
        var vpY = root.mapToItem(_flick, 0, 0).y
        return vpY + root._stickyHeaderY
    }

    property bool _stateReady: false
    property bool _userExpandToggle: false

    function _userToggleExpanded() {
        if (!root.collapsible || !root.expandable)
            return
        root._userExpandToggle = true
        root.expanded = !root.expanded
    }

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
        // Capture the collapsing height ABOVE this group BEFORE the store collapses
        // siblings (accordion) — so the parallel scroll can aim at the final layout.
        var deltaAbove = expanded ? _collapsingBodyHeightAbove() : 0
        if (_stateReady && stateStore && typeof stateStore.setSettingsGroupExpanded === "function") {
            var key = typeof stateKey === "string" ? stateKey.trim() : ""
            if (key !== "")
                stateStore.setSettingsGroupExpanded(key, expanded)
        }
        if (expanded) {
            if (root._userExpandToggle && root.scrollIntoViewOnExpand) {
                _animateExpandScroll(deltaAbove)
                scrollIntoViewTimer.restart()
            }
        } else {
            scrollIntoViewTimer.stop()
            if (root._userExpandToggle)
                _animateCollapseScroll()
        }
        root._userExpandToggle = false
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
        _flick = _findAncestorFlickable()
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
    // Accordion: expanding a group collapses the others, so the layout shifts
    // while animating. Wait for it to settle, THEN scroll just enough to reveal
    // the group: nothing if it already fits, enough to show its bottom otherwise,
    // and only pin the header to the top when the group is taller than the viewport.
    Timer {
        id: scrollIntoViewTimer
        interval: Anim.disclosureMs + 40
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

    // Total body height of OTHER expanded groups sitting ABOVE this one. Accordion
    // collapses them when this group opens, so this group shifts up by that amount;
    // accounted for so the parallel scroll aims at the final (settled) layout.
    function _collapsingBodyHeightAbove() {
        if (!_flick || !stateStore || !stateStore._settingsGroupInstances)
            return 0
        var arr = stateStore._settingsGroupInstances
        var myTop = root.mapToItem(_flick.contentItem, 0, 0).y
        var sum = 0
        for (var i = 0; i < arr.length; ++i) {
            var g = arr[i]
            if (!g || g === root || !g.expanded || !g.collapsible)
                continue
            if (g.mapToItem(_flick.contentItem, 0, 0).y < myTop)
                sum += Math.max(0, g.height - g._headerH)
        }
        return sum
    }

    // Scroll toward where the group WILL sit once it has expanded and the accordion
    // has collapsed the siblings — started in parallel with those animations so
    // expand + scroll play as one pass. `deltaAbove` = collapsing height above.
    function _animateExpandScroll(deltaAbove) {
        if (!_flick) return
        var topInContent = root.mapToItem(_flick.contentItem, 0, 0).y - deltaAbove
        var fullH = _predictedFullHeight()
        var bottomInContent = topInContent + fullH
        var vpH = _flick.height
        var cy = _flick.contentY
        // Already fully visible where it will land → don't scroll.
        if (topInContent >= cy - 0.5 && bottomInContent <= cy + vpH + 0.5)
            return
        var target = bottomInContent - vpH + Tokens.spaceLg
        target = Math.min(target, topInContent)   // never past the header
        var finalContentH = _flick.contentHeight + (fullH - root._headerH) - deltaAbove
        target = Math.max(0, Math.min(target, finalContentH - vpH))
        if (Math.abs(target - cy) < 0.5) return
        scrollIntoViewAnim.target = _flick
        scrollIntoViewAnim.from = cy
        scrollIntoViewAnim.to = target
        scrollIntoViewAnim.restart()
    }

    function _animateCollapseScroll() {
        if (!_flick) return
        var fullH = _predictedFullHeight()
        var finalContentH = _flick.contentHeight - (fullH - root._headerH)   // after this group shrinks
        var maxCY = Math.max(0, finalContentH - _flick.height)
        var cy = _flick.contentY
        var target = cy
        if (root.mapToItem(_flick, 0, 0).y < -1)
            target = Math.max(0, root.mapToItem(_flick.contentItem, 0, 0).y - Tokens.spaceLg)
        target = Math.min(target, maxCY)   // clamp to the shrunken content — no over-scroll
        if (Math.abs(target - cy) < 0.5) return
        scrollIntoViewAnim.target = _flick
        scrollIntoViewAnim.from = cy
        scrollIntoViewAnim.to = target
        scrollIntoViewAnim.restart()
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
                : (root.uniformBody ? island._headerColor
                                    : (root._bodyShown ? AppPalette.bgDeep : island._headerColor))

        height: root._bodyShown
                ? headerRow.height + Tokens.spaceSm + bodyCol.implicitHeight + root.contentPadding
                : headerRow.height

        Behavior on height {
            enabled: root.bodyAnimated
            NumberAnimation { duration: Anim.disclosureMs; easing.type: Anim.disclosureEasing }
        }

        // Gradient blends flush from the header's bottom edge down over half a row
        // (header colour → content bg), not spread over the whole group.
        // Non-linear: two curved mid-stops make the header colour decay FAST near
        // the top, then ease into the content bg (ease-out, not a straight blend).
        readonly property real _seamStart: Math.min(1, headerRow.height / Math.max(1, height))
        readonly property real _seamEnd: Math.min(1, (headerRow.height * 1.5) / Math.max(1, height))
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

        // ── Header row (sticky — floats to stay at the viewport top) ────────
        Item {
            id: headerRow
            y: root._stickyHeaderY
            anchors.left: parent.left
            anchors.right: parent.right
            height: root._headerH
            z: 2   // above the body so it hides content scrolling behind it
            opacity: root._headerFade   // fade out as the group scrolls past the top

            activeFocusOnTab: root.collapsible
            Keys.onReturnPressed: root._userToggleExpanded()
            Keys.onEnterPressed:  root._userToggleExpanded()
            Keys.onSpacePressed:  root._userToggleExpanded()

            Rectangle {
                anchors.fill: parent
                visible: root._stickyHeaderY > 0
                color: AppPalette.bg

                Rectangle {
                    anchors.fill: parent
                    color: island._headerColor
                    radius: island.radius
                }
                Rectangle {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    height: island.radius
                    color: island._headerColor
                }
            }

            // Seam under the floating header — mirrors the group's own header→body
            // gradient (same fast decay, half-row tall) so the sticky header blends
            // into the content scrolling beneath it.
            Rectangle {
                visible: root._stickyHeaderY > 0
                anchors.top: parent.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                height: Math.round(root._headerH * 0.5)
                gradient: Gradient {
                    GradientStop { position: 0.0;  color: island._headerColor }
                    GradientStop { position: 0.25; color: Qt.rgba(island._headerColor.r, island._headerColor.g, island._headerColor.b, 0.42) }
                    GradientStop { position: 0.55; color: Qt.rgba(island._headerColor.r, island._headerColor.g, island._headerColor.b, 0.09) }
                    GradientStop { position: 1.0;  color: Qt.rgba(island._headerColor.r, island._headerColor.g, island._headerColor.b, 0.0) }
                }
            }

            KFocusRing { id: focusRing; radius: Tokens.radiusLg }

            Rectangle {
                visible: root.showAccentBar
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
                anchors.leftMargin: root.showAccentBar ? Tokens.spaceXl : Tokens.spaceMd
                // Reserve room on the right for the header action buttons so the title never runs under them.
                anchors.rightMargin: Tokens.spaceLg
                                     + (headerActionsRow.width > 0 ? headerActionsRow.width + Tokens.spaceMd : 0)
                spacing: root.showAccentBar ? Tokens.spaceMd : Tokens.spaceSm

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
                        root._userToggleExpanded()
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
            // Fixed offset (not anchored to the header, which now floats).
            anchors.top: parent.top
            anchors.topMargin: root._headerH + Tokens.spaceSm
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

    Rectangle {
        x: 0
        y: root._stickyHeaderY
        width: island.width
        height: island.height - root._stickyHeaderY
        color: "transparent"
        radius: island.radius
        border.width: (!root.confirmed || root._bodyShown) ? 1 : 0
        border.color: root.confirmed ? AppPalette.groupBorder : AppPalette.dangerBorder
    }
}
