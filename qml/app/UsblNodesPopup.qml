import QtQuick 2.15
import kqml_types 1.0
import "UsblNodeLogic.js" as Node

// The acoustic-node panel on the scene: one row per node in the USBL plan.
//
// WHY IT IS NOT A CELL IN A WIDGET GRID. A grid panel's size is a pure function of
// cols x rows x 84 px, and that is load-bearing -- the occupancy validator, the drop maths, the
// editor overlay's cell grid and the aspect-ray scale snapping all derive from it. This panel's
// height comes from how many nodes the plan holds, which changes while you are looking at it.
// So it is a different KIND of panel (WorkspaceStore.widgetKinds) that reuses everything else a
// panel has: the position/scale/z instance, the shown map, docking, the list, the limit.
//
// WHAT A ROW SAYS is not decided here. Node.panelRows composes it, the same function the
// settings pane's rows go through, so the two surfaces cannot describe one beacon differently.
// This file translates codes into words and paints them.
//
// A ROW IS A NODE, not a step. Range and SNR are cached per ADDRESS, so per-command rows would
// repeat one node's numbers down its commands. The per-command detail survives as the cmd chip.
BasePanePopup {
    id: root

    required property var store
    required property var def
    property var engine: null

    property real widgetScale: 1.0

    readonly property real _bgAlpha: {
        var t = (def && typeof def.transparency === "number") ? def.transparency : 0
        return Math.max(0, Math.min(1, 1 - t / 100))
    }

    readonly property real _appScale: AppPalette.appScale
    readonly property real _k: _appScale * widgetScale

    // ── what to draw ──────────────────────────────────────────────────────
    readonly property var _rows: engine ? engine.rows : []
    // Eight is the protocol's address range, and a taller card than that stops being glanceable
    // over a chart. Past it the panel says how many it is not showing rather than growing.
    readonly property int _maxRows: 8
    readonly property int _shown: Math.min(_rows.length, _maxRows)
    readonly property int _hidden: Math.max(0, _rows.length - _maxRows)

    readonly property bool _noDevice: !engine || !engine.hasDevice
    readonly property bool _noNodes: !_noDevice && _rows.length === 0
    // Every row reads Idle with the schedule stopped, which is true and looks like a fault. One
    // line saying why costs less than an operator deciding the panel is broken.
    readonly property bool _stopped: !_noDevice && !_noNodes && !engine.running
    readonly property bool _note: _noDevice || _noNodes || _stopped

    // ── metrics ───────────────────────────────────────────────────────────
    // WIDTH IS THE WIDEST ROW, not a grid multiple. It used to be the width of a four-cell grid
    // panel, on the reasoning that this should sit in the same visual family as the panels it
    // shares a scene with -- but a row is a badge, a range, an SNR and three chips, and none of
    // that reaches a four-cell width. The family resemblance cost a third of the card in empty
    // space, on a panel whose whole argument for existing is that it is glanceable over a chart.
    // Height is still the only dimension the data moves.
    readonly property real _baseW: Math.round(_rowW * _appScale) + contentPadding * 2
    // Line 1 (badge + range at 1.2x + SNR + the age chip pushed right) is the binding one; line 2
    // is three chips and fits inside it.
    readonly property real _rowW: 260

    // CHIP HEIGHT COMES FROM THE THEME, via Tokens.chipH, but it cannot be used raw here.
    // Tokens are in AppPalette.scale space; this panel's metrics are in appScale space (the
    // widget system's 84 px cell is), and the two differ by appScaleBoost — so a chip taking
    // Tokens.chipH directly renders ~11% shorter than the identical chip in the settings pane.
    // Dividing back out gives the unscaled height, which `_k` then puts in the panel's space.
    readonly property real _chipH: Math.round((Tokens.chipH / Math.max(0.01, AppPalette.scale)) * _k)
    // Two lines of chips plus the gutters between and around them. Derived rather than a
    // literal, so the row grows when the theme's controls do instead of clipping them.
    readonly property real _rowH: _chipH * 2 + Math.round(12 * _k)
    readonly property real _noteH: Math.round(18 * _k)
    readonly property real _gap: Math.round(4 * _k)
    readonly property real _pad: Math.round(8 * _k)

    readonly property real _contentW: Math.round(_rowW * _k)
    readonly property real _contentH: {
        var n = Math.max(1, _shown)          // an empty plan still occupies one line, which says so
        var h = n * _rowH + (n - 1) * _gap
        if (_hidden > 0) h += _gap + _noteH
        if (_note)      h += _gap + _noteH
        return h
    }

    popupVisible: true
    dragEnabled: true
    dragAnywhere: true
    headerReserved: false
    // No corner grip, exactly like a grid panel: scale is a panel property, set in its editor,
    // not something a drag on the scene should redefine. Height is the data's to decide.
    resizeEnabled: false
    collapseButtonVisible: false
    fullscreenMode: false
    panelColor: "transparent"
    panelBorderColor: "transparent"
    panelRadius: Tokens.radiusLg
    ghostFollowsContent: true
    ghostRadius: Tokens.radiusLg
    headerDragBarLength: 0
    snapEdgeCenters: true

    // Imperative, not a binding: BasePanePopup writes into expandedWidth/Height itself, so a
    // binding would be broken by the base class the first time it did. Same reason and same
    // shape as DataWidgetPopup._applyScale -- the only difference is that one term now comes
    // from the plan, which is what "auto-extending" amounts to.
    function _applyScale() {
        expandedWidth  = Math.round(_contentW + _pad * 2 + contentPadding * 2)
        expandedHeight = Math.round(_contentH + _pad * 2 + contentPadding * 2)
    }

    property bool _synced: false

    function syncFromStore() {
        if (!def || !def.id)
            return
        widgetScale = store.widgetScale(def.id)
        _applyScale()
        var p = store.widgetPosition(def.id, popupWidth, popupHeight)
        var rb = store.widgetRevealBounds(popupWidth, popupHeight)
        var nx = Math.max(rb.minX, Math.min(rb.maxX, p.x))
        var ny = Math.max(rb.minY, Math.min(rb.maxY, p.y))
        suspendSignals = true
        panelX = clampX(nx)
        panelY = clampY(ny)
        suspendSignals = false
        _synced = true
    }

    onWidgetScaleChanged: _applyScale()
    on_ContentHChanged: _applyScale()
    on_ContentWChanged: _applyScale()
    onDefChanged: { _applyScale(); Qt.callLater(syncFromStore) }

    Component.onCompleted: {
        syncFromStore()
        Qt.callLater(syncFromStore)
        Qt.callLater(resolveOverlapWithSibling)
    }

    onPositionCommitted: function(x, y, w, h) {
        if (_synced && def && def.id)
            store.setWidgetPosition(def.id, x, y, w, h)
    }

    onInteractionStarted: if (store && def && def.id) store.widgetBringToFront(def.id)

    dockState: (store && def && def.id) ? store.popupDock(popupId) : null
    onDockCommitted: function(targetId, side, gap, crossOffset) {
        store.setPopupDock(popupId, { targetId: targetId, side: side, gap: gap, cross: crossOffset })
    }

    // ── the words ─────────────────────────────────────────────────────────
    // The pane keeps its own copies of these tables: qsTr's context is the file, so a shared
    // one would need a QML singleton in kqml_types, which cannot import a module from qml/app.
    // test_usbl_node_logic.mjs asserts BOTH files name exactly the codes the reducer can return,
    // which is what keeps two copies from drifting into two vocabularies.
    readonly property var _opText: ({
        "off":     qsTr("Off"),
        "idle":    qsTr("Idle"),
        "waiting": qsTr("Waiting")
    })
    readonly property var _replyText: ({
        "replied": qsTr("Replied"),
        "stale":   qsTr("Stale"),
        "none":    "—"
    })

    function _fmtAge(ms) {
        if (ms < 0) return ""
        var s = ms / 1000
        return (s < 10 ? s.toFixed(1) : Math.round(s)) + qsTr(" s")
    }
    function _num(v, digits, suffix) {
        if (v === undefined || v === null || isNaN(v)) return "—"
        return v.toFixed(digits) + (suffix ? suffix : "")
    }

    // Same three-state fill as the pane's badges, so a row cannot be read one way here and
    // another way in settings. Nothing animates on a loop: with a sub-second answer window the
    // eye has nothing to track, and a fading border erases the mark it is drawn on.
    component Chip: Rectangle {
        id: chip
        property string label: ""
        property string code: ""       // "waiting" | "replied" | "stale" | anything else
        property bool outlineOnly: false
        // The theme's chip height, already in this panel's scale space (root._chipH).
        // Everything else about the chip is a fraction of it, so one number moves the whole
        // thing when the theme's control height changes.
        property real h: Tokens.chipH
        readonly property bool _out: chip.code === "waiting"
        readonly property bool _ok:  chip.code === "replied"
        readonly property bool _bad: chip.code === "stale"
        implicitWidth: _chipText.implicitWidth + Math.round(chip.h * 0.55)
        implicitHeight: chip.h
        radius: Tokens.radiusSm
        color: chip.outlineOnly ? "transparent"
             : chip._out ? AppPalette.accentBg
             : chip._ok  ? AppPalette.linkOkBg
             : chip._bad ? AppPalette.linkIdleBg : "transparent"
        border.width: Math.max(1, Math.round(chip.h / 22))
        border.color: chip._out ? AppPalette.accentBorder
                    : chip._ok  ? AppPalette.linkOkBorder
                    : chip._bad ? AppPalette.linkIdleBorder : AppPalette.border
        Behavior on color { ColorAnimation { duration: 200; easing.type: Easing.OutCubic } }
        Text {
            id: _chipText
            anchors.centerIn: parent
            text: chip.label
            color: chip._out ? AppPalette.accentText
                 : chip._ok  ? AppPalette.linkOkText
                 : chip._bad ? AppPalette.linkIdleText : AppPalette.textMuted
            font.pixelSize: Math.max(8, Math.round(chip.h * 0.44))
            font.bold: true
        }
    }

    Rectangle {
        anchors.fill: parent
        radius: root.panelRadius
        color: Qt.rgba(AppPalette.bg.r, AppPalette.bg.g, AppPalette.bg.b, root._bgAlpha)
        border.width: 0

        Column {
            width: root._contentW
            anchors.centerIn: parent
            spacing: root._gap

            // The states in which there are no rows to draw. Each says what is missing rather
            // than leaving an empty card, which reads as a broken panel.
            Text {
                visible: root._noDevice || root._noNodes
                width: parent.width
                height: root._rowH
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
                elide: Text.ElideRight
                text: root._noDevice ? qsTr("No acoustic device")
                                     : qsTr("No nodes in the USBL plan")
                color: AppPalette.textMuted
                font.pixelSize: Math.max(8, Math.round(12 * root._k))
            }

            Repeater {
                // Modelled on the COUNT, not the array. `rows` is recomposed on every clock
                // tick, and a Repeater bound to the array itself would tear down and rebuild
                // every delegate once a second.
                model: root._shown
                delegate: Item {
                    id: nodeRow
                    required property int index
                    readonly property var _r: root._rows[index]

                    width: root._contentW
                    height: root._rowH

                    Rectangle {
                        anchors.fill: parent
                        radius: Tokens.radiusSm
                        // A DARKER row while this node's request is out -- the only thing on the
                        // panel that moves.
                        //
                        // It used to be the accent colour at 14% alpha. Alpha was the wrong tool:
                        // the panel sits on a scene whose transparency the operator controls, so
                        // a translucent fill composited against whatever chart happened to be
                        // underneath -- over dark water it read as nothing at all, and the one
                        // moving mark on the panel was invisible exactly where the panel is used.
                        // A solid colour derived from the row's own surface is the same mark
                        // everywhere. Two factors because a dark theme needs the bigger nudge to
                        // shift at all, which is how bgDeep and groupBorder are already built.
                        color: nodeRow._r && nodeRow._r.op === "waiting"
                               ? Qt.darker(AppPalette.rowRaised, AppPalette.isDark ? 1.35 : 1.10)
                               : AppPalette.rowRaised
                        border.width: Tokens.cardBorderWidth
                        border.color: AppPalette.border
                        Behavior on color { ColorAnimation { duration: 200; easing.type: Easing.OutCubic } }
                        // Switched off keeps its numbers, dimmed: the last thing a node said is
                        // worth reading after you stop asking it.
                        opacity: (nodeRow._r && nodeRow._r.active) ? 1.0 : 0.62
                    }

                    // Line 1: who, how far, how strong -- and how old, on the right.
                    Row {
                        id: _line1
                        anchors.left: parent.left
                        anchors.leftMargin: Math.round(6 * root._k)
                        anchors.top: parent.top
                        anchors.topMargin: Math.round(4 * root._k)
                        spacing: Math.round(6 * root._k)

                        // The same badge the settings pane's rows use. A bare number next to
                        // "18.3 m" read as a row index; the shape is what says it is an address.
                        UsblAddressBadge {
                            address: nodeRow._r ? nodeRow._r.addr : 0
                            diameter: root._chipH
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        // The range is what the panel is READ for -- the chips beside it say
                        // whether to believe it, but this is the number. Set 1.2x the row's other
                        // text so it wins at a glance over a chart, which is the distance this
                        // panel is looked at from.
                        Text {
                            text: root._num(nodeRow._r && nodeRow._r.entry
                                            ? nodeRow._r.entry.distance : NaN, 1) + " " + qsTr("m")
                            color: AppPalette.textStrong
                            font.pixelSize: Math.max(8, Math.round(13 * 1.2 * root._k))
                            font.bold: true
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        // SNR is absent on the v1/v2 solution paths -- the payload carries none
                        // and the projection leaves it NAN -- so this is an em dash far more
                        // often than it is a number. It used to read a confident 0.
                        Text {
                            text: root._num(nodeRow._r && nodeRow._r.entry
                                            ? nodeRow._r.entry.snr : NaN, 0) + " " + qsTr("dB")
                            color: AppPalette.textMuted
                            font.pixelSize: Math.max(7, Math.round(11 * root._k))
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }

                    Chip {
                        anchors.right: parent.right
                        anchors.rightMargin: Math.round(6 * root._k)
                        anchors.verticalCenter: _line1.verticalCenter
                        h: root._chipH
                        outlineOnly: true
                        visible: !!nodeRow._r && nodeRow._r.ageMs >= 0
                        // Outlined amber past the warning threshold rather than filled: it is a
                        // note about the data, not a verdict on the beacon. It is also the only
                        // thing on a row that moves while nothing is being interrogated.
                        code: (nodeRow._r && nodeRow._r.aged) ? "stale" : ""
                        label: nodeRow._r ? root._fmtAge(nodeRow._r.ageMs) : ""
                    }

                    // Line 2: the three verdicts, in the pane's own words.
                    Row {
                        anchors.left: parent.left
                        anchors.leftMargin: Math.round(6 * root._k)
                        anchors.bottom: parent.bottom
                        anchors.bottomMargin: Math.round(4 * root._k)
                        spacing: Math.round(4 * root._k)

                        Chip {
                            h: root._chipH
                            code: nodeRow._r ? nodeRow._r.op : ""
                            label: nodeRow._r ? (root._opText[nodeRow._r.op] || nodeRow._r.op) : ""
                        }
                        Chip {
                            h: root._chipH
                            code: nodeRow._r ? nodeRow._r.reply : ""
                            label: nodeRow._r ? (root._replyText[nodeRow._r.reply] || nodeRow._r.reply) : ""
                        }
                        // WHICH command was last asked, and how that one ended. The row's reply
                        // chip beside it reports the last RESOLVED outcome, so the two disagree
                        // for as long as a request is out -- both true, and that is the point.
                        // Absent until something has been asked: a chip about no command is
                        // worse than no chip.
                        Chip {
                            h: root._chipH
                            visible: !!nodeRow._r && nodeRow._r.lastCmd >= 0
                            code: nodeRow._r ? nodeRow._r.lastCmdState : ""
                            label: nodeRow._r ? ("cmd " + nodeRow._r.lastCmd) : ""
                        }
                    }
                }
            }

            Text {
                visible: root._hidden > 0
                width: parent.width
                height: root._noteH
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignRight
                text: qsTr("+%1 more").arg(root._hidden)
                color: AppPalette.textMuted
                font.pixelSize: Math.max(7, Math.round(10 * root._k))
            }

            Text {
                visible: root._stopped
                width: parent.width
                height: root._noteH
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
                elide: Text.ElideRight
                // Not a button. Starting a schedule transmits, and a transmit control under a
                // finger on a moving boat is a different feature from a readout.
                text: qsTr("Schedule stopped")
                color: AppPalette.textMuted
                font.pixelSize: Math.max(7, Math.round(10 * root._k))
            }
        }
    }
}
