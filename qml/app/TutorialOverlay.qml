import QtQuick
import kqml_types 1.0
import "TutorialLayout.js" as TutorialLayout

Item {
    id: root

    required property var store

    signal closeTransientUiRequested()

    readonly property var controller: (typeof tutorial !== "undefined" && tutorial) ? tutorial : null
    readonly property bool active: !!root.controller && root.controller.active
    readonly property bool showing: root.active && root.controller.phase === "showing"
    readonly property var step: root.active ? root.controller.step : ({})

    readonly property var dimCfg: root.step.dim || ({})
    readonly property var spotCfg: root.step.spotlight || ({})
    readonly property var arrowCfg: root.step.arrow || ({})
    readonly property var bubbleCfg: root.step.bubble || ({})
    readonly property var buttonsCfg: root.step.buttons || ({})
    readonly property string advanceMode: root.step.advance || "target"
    readonly property var targetKeys: root.step.targets || []

    readonly property real _margin: Tokens.spaceXl
    readonly property real _scale: AppPalette.scale
    readonly property bool _blockInput: root.dimCfg.blockInput !== false
    readonly property bool _passThrough: root.showing && root.spotCfg.passThrough === true
                                         && root.advanceMode !== "anywhere"

    property int _serial: -1
    property var _rects: []
    property var _holes: []
    property bool _waiting: false
    property real _waitStartMs: 0
    property bool _missingCenter: false
    property bool _tapped: false
    property bool _shownOnce: false
    property bool _morphPending: false
    property int _scrolledSerial: -1
    property var _currentStep: null
    property var _targetItems: []

    readonly property var _anchorRect: root._missingCenter ? null : TutorialLayout.unite(root._holes)

    readonly property var _layout: TutorialLayout.layoutBubble({
        area: { width: root.width, height: root.height },
        size: { width: bubble.width, height: bubble.height },
        margin: root._margin,
        hole: root._anchorRect,
        placement: root.bubbleCfg.placement || "auto",
        arrowEnabled: root.arrowCfg.enabled !== false,
        arrowDirection: root.arrowCfg.direction || "auto",
        arrowLength: root._arrowLength,
        arrowOffset: (root.arrowCfg.offset || 0) * root._scale,
        arrowTravel: coachArrow.travel,
        gap: Tokens.spaceSm,
        tailSize: bubble.tailSize,
        cornerRadius: Tokens.radiusLg
    })
    readonly property real _arrowLength: Math.round((root.arrowCfg.length !== undefined ? root.arrowCfg.length : 44) * root._scale)

    anchors.fill: parent
    z: ZOrder.tutorialOverlay
    visible: root.active

    Keys.enabled: root.active
    Keys.onShortcutOverride: function (event) {
        event.accepted = event.key !== Qt.Key_Escape && event.key !== Qt.Key_F11 && !root._systemKey(event.key)
    }
    Keys.onPressed: function (event) {
        event.accepted = !root._systemKey(event.key)
        if (event.accepted && !event.isAutoRepeat)
            root._keyPressed(event.key)
    }
    Keys.onReleased: function (event) {
        event.accepted = !root._systemKey(event.key)
    }

    function _systemKey(key) {
        return key === Qt.Key_Back || key === Qt.Key_VolumeUp || key === Qt.Key_VolumeDown
            || key === Qt.Key_VolumeMute || key === Qt.Key_MediaPlay || key === Qt.Key_MediaPause
            || key === Qt.Key_MediaTogglePlayPause || key === Qt.Key_MediaNext || key === Qt.Key_MediaPrevious
    }

    function _grabKeys() {
        if (root.active && !root.activeFocus)
            root.forceActiveFocus()
    }

    function _focusIsLegit() {
        const item = root.Window.activeFocusItem
        if (!item || item === root.parent || item === root.Window.contentItem)
            return false
        let underLayer = false
        for (let p = item; p; p = p.parent) {
            if (p === root || root._targetItems.indexOf(p) >= 0)
                return true
            if (p === root.parent)
                underLayer = true
        }
        return !underLayer
    }

    function _keyPressed(key) {
        if (!root.showing)
            return
        const forward = key === Qt.Key_Right || key === Qt.Key_Return || key === Qt.Key_Enter || key === Qt.Key_Space
        const backward = key === Qt.Key_Left || key === Qt.Key_Backspace
        if (bubble.confirming) {
            if (key === Qt.Key_Return || key === Qt.Key_Enter)
                root.controller.stop()
            return
        }
        if (forward && (root.advanceMode === "anywhere" || (root.advanceMode === "button" && root.buttonsCfg.next === true)))
            root._advance()
        else if (backward && root.buttonsCfg.back === true && root.controller.canGoBack)
            root.controller.back()
    }

    function _host(on) {
        if (on === "core")
            return (typeof core !== "undefined" && core) ? core : null
        return root.store
    }

    function _runActions(list) {
        if (!list)
            return ""
        for (let i = 0; i < list.length; ++i) {
            const action = list[i]
            const on = action.on || "store"
            const host = root._host(on)
            try {
                if (action.call !== undefined) {
                    const fn = host ? host[action.call] : undefined
                    if (typeof fn !== "function")
                        return "unknown " + on + " function '" + action.call + "'"
                    fn.apply(host, action.args || [])
                } else if (action.set !== undefined) {
                    if (!host || !(action.set in host))
                        return "unknown " + on + " property '" + action.set + "'"
                    host[action.set] = action.value
                }
            } catch (e) {
                return String(e)
            }
        }
        return ""
    }

    function _beginStep() {
        if (!root.active)
            return
        root._grabKeys()
        if (root._currentStep)
            root._runActions(root._currentStep.exit)
        root._currentStep = root.controller.step
        root._serial = root.controller.stepSerial
        root._rects = []
        root._morphPending = true
        root._missingCenter = false
        root._tapped = false
        root._shownOnce = false
        root._waiting = false
        bubble.confirming = false

        const error = root._runActions(root._currentStep.enter)
        if (error !== "") {
            root.controller.stepFailed(root._serial, error)
            return
        }
        delayTimer.interval = Math.max(1, root._currentStep.delayMs || 0)
        delayTimer.restart()
    }

    function _startWaiting() {
        root._waiting = true
        root._waitStartMs = Date.now()
        root._track()
    }

    function _visibleRect(item) {
        const r = item.mapToItem(root, 0, 0, item.width, item.height)
        let rect = TutorialLayout.intersect({ x: r.x, y: r.y, width: r.width, height: r.height },
                                            { x: 0, y: 0, width: root.width, height: root.height })
        for (let p = item.parent; rect && p; p = p.parent) {
            if (!p.clip)
                continue
            const c = p.mapToItem(root, 0, 0, p.width, p.height)
            rect = TutorialLayout.intersect(rect, { x: c.x, y: c.y, width: c.width, height: c.height })
        }
        return rect
    }

    function _isClipped(item, rect) {
        return !rect || rect.width < item.width - 1 || rect.height < item.height - 1
    }

    function _scrollIntoView(item) {
        let moved = false
        const margin = Tokens.spaceLg
        for (let p = item.parent; p; p = p.parent) {
            if (p.contentY === undefined || p.contentHeight === undefined || !p.contentItem || p.height <= 0)
                continue
            const r = item.mapToItem(p.contentItem, 0, 0, item.width, item.height)
            const lo = p.originY || 0
            const hi = lo + Math.max(0, p.contentHeight - p.height)
            let y = p.contentY
            if (r.y < y + margin)
                y = r.y - margin
            else if (r.y + r.height > y + p.height - margin)
                y = r.y + r.height - p.height + margin
            y = Math.max(lo, Math.min(hi, y))
            if (Math.abs(y - p.contentY) > 0.5) {
                p.contentY = y
                moved = true
            }
        }
        return moved
    }

    function _resolve() {
        const items = root.targetKeys.map(function (k) { return root.controller.anchor(k) })
        root._targetItems = items.filter(function (it) { return !!it })
        if (!items[0])
            return null
        const rects = items.map(function (it) { return it ? root._visibleRect(it) : null })
        const clipped = items.some(function (it, i) { return !!it && root._isClipped(it, rects[i]) })
        if (clipped && root._scrolledSerial !== root._serial) {
            root._scrolledSerial = root._serial
            let moved = false
            for (let k = items.length - 1; k >= 0; --k) {
                if (items[k])
                    moved = root._scrollIntoView(items[k]) || moved
            }
            if (moved)
                return null
        }
        if (!rects[0])
            return null
        return rects.filter(function (r) { return !!r })
    }

    function _applyRects(rects) {
        if (root._morphPending) {
            root._morphPending = false
            spotlight.morph()
        }
        let changed = rects.length !== root._rects.length || (rects.length === 0 && root._holes.length > 0)
        for (let i = 0; !changed && i < rects.length; ++i)
            changed = !TutorialLayout.sameRect(rects[i], root._rects[i])
        if (!changed)
            return
        root._rects = rects
        const shape = root.spotCfg.shape || "roundRect"
        const padding = (root.spotCfg.padding || 0) * root._scale
        const radius = (root.spotCfg.radius || 0) * root._scale
        const holes = rects.map(function (r) { return TutorialLayout.holeFor(r, shape, padding, radius) })
        const merge = root.spotCfg.merge || "auto"
        root._holes = merge === "all" ? TutorialLayout.uniteHoles(holes)
                    : TutorialLayout.mergeHoles(holes, merge === "none" ? 0 : (root.spotCfg.mergeGap || 0) * root._scale)
        const first = rects.length > 0 ? rects[0] : null
        root.controller.reportTarget(root._serial, !!first,
                                     first ? Qt.rect(first.x, first.y, first.width, first.height) : Qt.rect(0, 0, 0, 0))
    }

    function _conditionMet() {
        const until = root.step.until
        if (!until)
            return true
        const host = root._host(until.on || "store")
        return !!host && host[until.property] === until.equals
    }

    function _readyToAdvance() {
        if (bubble.confirming || !root._shownOnce || !root._conditionMet())
            return false
        return root.advanceMode === "condition" || (root.advanceMode === "target" && root._tapped)
    }

    function _track() {
        if (!root.active || root.controller.stepSerial !== root._serial)
            return
        if (root._readyToAdvance()) {
            root._advance()
            return
        }
        if (root.targetKeys.length > 0 && !root._missingCenter) {
            const rects = root._resolve()
            if (root._waiting) {
                if (rects) {
                    root._waiting = false
                    root._applyRects(rects)
                    root.controller.stepReady(root._serial)
                } else if (Date.now() - root._waitStartMs >= (root.step.timeoutMs || 0)) {
                    root._handleMissing()
                    return
                }
            } else if (root.showing) {
                if (rects) {
                    root._applyRects(rects)
                } else {
                    root.controller.stepSuspended(root._serial)
                    root._applyRects([])
                    root._waiting = true
                    root._waitStartMs = Date.now()
                }
            }
        } else if (root._waiting) {
            root._waiting = false
            root._applyRects([])
            root.controller.stepReady(root._serial)
        }

        if (root.showing)
            root._shownOnce = true
    }

    function _handleMissing() {
        const policy = root.step.onMissing || "center"
        root._waiting = false
        if (policy === "skip") {
            const serial = root._serial
            Qt.callLater(function () {
                if (root.active && root.controller.stepSerial === serial)
                    root.controller.skipStep()
            })
        } else if (policy === "abort") {
            root.controller.stepFailed(root._serial, "anchor '" + root.targetKeys[0] + "' is not available")
        } else {
            root._missingCenter = true
            root._applyRects([])
            root.controller.stepReady(root._serial)
        }
    }

    function _advance() {
        const serial = root._serial
        Qt.callLater(function () {
            if (root.active && root.controller.stepSerial === serial)
                root.controller.advance()
        })
    }

    function _markTapped() {
        if (!root.showing)
            return
        root._tapped = true
        root._shownOnce = true
        if (root._conditionMet())
            root._advance()
    }

    function _insideHole(x, y) {
        for (let i = 0; i < root._holes.length; ++i) {
            const h = root._holes[i]
            if (x >= h.x && x <= h.x + h.width && y >= h.y && y <= h.y + h.height)
                return true
        }
        return false
    }

    function handleEscape() {
        if (!root.active)
            return false
        if (root.buttonsCfg.close === false)
            return true
        if (bubble.confirming)
            bubble.confirming = false
        else if (root.showing)
            bubble.confirming = true
        else
            root.controller.stop()
        return true
    }

    function _blockerClicked(x, y) {
        if (!root.showing)
            return
        if (bubble.confirming)
            bubble.nudge()
        else if (root.advanceMode === "anywhere")
            root._advance()
        else if (root.advanceMode === "target" && !root._passThrough && root._insideHole(x, y))
            root._markTapped()
        else if (bubble.shown)
            bubble.nudge()
    }

    function _finishScenario() {
        if (root._currentStep)
            root._runActions(root._currentStep.exit)
        root._currentStep = null
        root._serial = -1
        root._rects = []
        root._holes = []
        root._targetItems = []
        root._waiting = false
        bubble.confirming = false
        delayTimer.stop()
        if (root.activeFocus && root.parent)
            root.parent.forceActiveFocus()
    }

    Connections {
        target: root.controller
        ignoreUnknownSignals: true
        function onActiveChanged() {
            if (root.controller.active) {
                root.closeTransientUiRequested()
                root._grabKeys()
            }
            else
                root._finishScenario()
        }
        function onStepSerialChanged() {
            if (root.controller.active)
                root._beginStep()
        }
        function onFailed(id, reason) {
            if (typeof notifications !== "undefined" && notifications)
                notifications.warning(qsTr("Tutorial stopped: a step could not be shown"))
        }
    }

    Connections {
        target: root.store
        ignoreUnknownSignals: true
        function onInputLockedChanged() {
            if (root.store.inputLocked && root.active)
                root.controller.stop()
        }
    }

    Timer {
        id: delayTimer
        repeat: false
        onTriggered: root._startWaiting()
    }

    Timer {
        interval: Math.max(1, root.step.autoAdvanceMs || 0)
        running: root.showing && root.advanceMode === "timer" && !bubble.confirming
        repeat: false
        onTriggered: root._advance()
    }

    FrameAnimation {
        running: root.active && (root._waiting || root.showing)
        onTriggered: {
            if (!root.activeFocus && !root._focusIsLegit())
                root._grabKeys()
            root._track()
        }
    }

    KSpotlight {
        id: spotlight
        anchors.fill: parent
        dimEnabled: root.dimCfg.enabled !== false
        dimOpacity: root.dimCfg.opacity !== undefined ? root.dimCfg.opacity : 0.72
        holes: root.spotCfg.enabled !== false ? root._holes : []
        ringEnabled: root.showing && root.spotCfg.enabled !== false
        pulse: root.spotCfg.pulse !== false
    }

    MouseArea {
        id: blocker
        anchors.fill: parent
        enabled: root.active && root._blockInput
        hoverEnabled: true
        preventStealing: true
        acceptedButtons: Qt.AllButtons
        containmentMask: QtObject {
            function contains(point: point): bool {
                return !(root._passThrough && root._insideHole(point.x, point.y))
            }
        }
        onClicked: function (mouse) { root._blockerClicked(mouse.x, mouse.y) }
        onWheel: function (wheel) { wheel.accepted = true }
    }

    DropArea {
        anchors.fill: parent
        enabled: root.active
    }

    TapHandler {
        enabled: root.showing && !root._blockInput && root.advanceMode === "anywhere" && !bubble.confirming
        onTapped: root._advance()
    }

    Repeater {
        model: (root.showing && root._passThrough && root.advanceMode === "target" && !bubble.confirming) ? root._holes.length : 0

        Item {
            required property int index
            readonly property var hole: root._holes[index] || ({ x: 0, y: 0, width: 0, height: 0 })
            x: hole.x
            y: hole.y
            width: hole.width
            height: hole.height

            TapHandler {
                onTapped: root._markTapped()
            }
        }
    }

    KCoachArrow {
        id: coachArrow
        visible: root.showing && root.arrowCfg.enabled !== false && root._layout.arrowVisible
        direction: root._layout.direction
        tipX: root._layout.tipX
        tipY: root._layout.tipY
        length: root._arrowLength
        bounce: root.arrowCfg.bounce !== false
    }

    KCoachBubble {
        id: bubble
        x: root._layout.x
        y: root._layout.y
        shown: root.showing && (root.bubbleCfg.enabled !== false || bubble.confirming)
        maxWidth: Math.min((root.bubbleCfg.maxWidth || 360) * root._scale, root.width - root._margin * 2)
        title: root.step.title || ""
        text: root.step.text || ""
        iconSource: root.bubbleCfg.icon ? "qrc:/icons/" + root.bubbleCfg.icon : ""
        counterText: root.bubbleCfg.counter !== false && root.step.count > 1
                     ? qsTr("%1 / %2").arg(root.step.index + 1).arg(root.step.count) : ""
        hintText: root.bubbleCfg.hint === true ? qsTr("Tap anywhere to continue") : ""
        tailSide: root.bubbleCfg.tail !== false ? root._layout.tailSide : ""
        tailOffset: root._layout.tailOffset
        showBack: root.buttonsCfg.back === true
        backEnabled: !!root.controller && root.controller.canGoBack
        showSkip: root.buttonsCfg.skip === true
        showNext: root.buttonsCfg.next === true
        backLabel: root.buttonsCfg.backLabel || qsTr("Back")
        skipLabel: root.buttonsCfg.skipLabel || qsTr("Skip tutorial")
        nextLabel: root.buttonsCfg.nextLabel || qsTr("Next")
        onBackClicked: root.controller.back()
        onSkipClicked: root.controller.finish()
        showClose: root.buttonsCfg.close !== false
        onNextClicked: root._advance()
        onCloseConfirmed: root.controller.stop()
    }

    KCircleIconButton {
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: root._margin
        width: Tokens.controlHXl
        height: Tokens.controlHXl
        visible: root.showing && root.buttonsCfg.close !== false && root.bubbleCfg.enabled === false && !bubble.confirming
        iconSource: "qrc:/icons/ui/x.svg"
        iconTintColor: AppPalette.text
        fillColor: AppPalette.card
        fillHoverColor: AppPalette.cardHover
        borderColor: AppPalette.border
        borderWidth: 1
        toolTipText: qsTr("Close tutorial")
        onClicked: bubble.confirming = true
    }
}
