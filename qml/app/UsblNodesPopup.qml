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
// Nor are the words decided here any more: UsblOpBadge and UsblStateBadge draw the two state
// axes and own their vocabulary, so this file lays out a row and paints the numbers.
//
// THE TOP LINE IS A STATUS BAR and the bottom line is the readings -- the shape the settings
// pane's row already had. It was the other way round until the two state chips became two
// badges; see docs/KoggerApp-Docs/usbl-node-row.md.
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
    // WIDTH DOES NOT MOVE WITH THE DATA. Height follows the row count; width does not follow
    // anything a beacon does. A card that resizes while you are looking at it is wrong on a
    // panel that floats over a chart -- it shifts under the cursor and drags its dock with it.
    //
    // It is still MEASURED rather than pinned, because a literal encodes a layout and a locale:
    // the verdict badge sizes itself to the widest of its three words, which are short in
    // English and will not be everywhere. But it is measured off STATIC samples -- the widest
    // string each slot's formatter can produce -- so the result is a constant for a given theme
    // and language and never a function of what the nodes are doing.
    //
    // TWO BUGS LIVED HERE and both made the card too narrow for its own rows. The prototype was
    // laid out unscaled and the result multiplied by _k, which is not the same as laying it out
    // at the real size: text width does not scale linearly with point size and every chip rounds
    // its own padding. And the rows' own side margins were never in the total at all -- 6 px
    // each side, against a 4 px gutter. It is laid out at the real sizes now, and the margins
    // are added explicitly.
    readonly property real _rowPadH: Math.round(6 * _k)
    readonly property real _uChipH: Tokens.chipH / Math.max(0.01, AppPalette.scale)
    // Two sizes in the verdict badge: the word leads, the age is a note beside it.
    readonly property real _uWordFont: Tokens.fontSm   / Math.max(0.01, AppPalette.scale)
    readonly property real _uAgeFont:  Tokens.fontXxs  / Math.max(0.01, AppPalette.scale)
    readonly property real _uCmdFont:  Tokens.fontBase / Math.max(0.01, AppPalette.scale)

    // CHIP HEIGHT COMES FROM THE THEME, via Tokens.chipH, but it cannot be used raw here.
    // Tokens are in AppPalette.scale space; this panel's metrics are in appScale space (the
    // widget system's 84 px cell is), and the two differ by appScaleBoost — so a chip taking
    // Tokens.chipH directly renders ~11% shorter than the identical chip in the settings pane.
    // Dividing back out gives the unscaled height, which `_k` then puts in the panel's space.
    readonly property real _chipH: Math.round(_uChipH * _k)
    readonly property real _wordFont: Math.max(8, Math.round(_uWordFont * _k))
    readonly property real _ageFont:  Math.max(7, Math.round(_uAgeFont * _k))
    readonly property real _cmdFont:  Math.max(9, Math.round(_uCmdFont * _k))
    // The number the panel is READ for. The status bar took the top line, so this one carries
    // only the range and the SNR and can afford the size.
    readonly property real _rangeFont: Math.max(10, Math.round(22 * _k))
    // Two lines plus the gutters between and around them. The status bar is a chip tall; the
    // readings line is as tall as the range number RENDERS, which at 22 px is taller than a
    // chip -- so it comes off the same prototype the width does rather than being assumed to be
    // another chipH. Derived rather than literal, so the row grows when the theme's controls do
    // instead of clipping them.
    readonly property real _readH: Math.max(_chipH, Math.ceil(_protoRead.implicitHeight))
    readonly property real _rowH: _chipH + _readH + Math.round(12 * _k)
    readonly property real _noteH: Math.round(18 * _k)
    readonly property real _gap: Math.round(4 * _k)
    readonly property real _pad: Math.round(8 * _k)

    // Same scale space as the prototype, so nothing is converted and nothing accumulates.
    readonly property real _contentW: Math.ceil(Math.max(_protoStatus.implicitWidth,
                                                         _protoRead.implicitWidth)
                                                + _rowPadH * 2)
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
    // There are none left here. This panel and the settings pane used to keep a copy of the
    // state vocabulary each, because qsTr's context is the file and a shared table would have
    // needed a singleton in kqml_types, which cannot import from qml/app. UsblStateBadge is a
    // plain component in qml/app, so it can own them -- and one table cannot drift from itself.

    // TIERED, and that is what makes a fixed slot honest. Seconds only ran to "86400 s" after a
    // night on the bench -- a string that grows without bound cannot share a column with
    // anything, and the slot it sits in would have to resize to hold it.
    function _fmtAge(ms) {
        if (ms < 0) return ""
        var s = ms / 1000
        if (s < 10)  return s.toFixed(1) + qsTr(" s")
        if (s < 100) return Math.round(s) + qsTr(" s")
        var m = s / 60
        if (m < 100) return Math.round(m) + qsTr(" m")
        return Math.round(m / 60) + qsTr(" h")
    }
    function _num(v, digits, suffix) {
        if (v === undefined || v === null || isNaN(v)) return "—"
        return v.toFixed(digits) + (suffix ? suffix : "")
    }

    // WHICH COMMAND was last sent. The only chip on the status bar that is neither of the two
    // state axes, so it is outlined and never filled -- the verdict badge beside it is the one
    // thing on a healthy row allowed to wear a colour.
    //
    // A prompt rather than the word "CMD": a chevron and an underscore, which reads as a command
    // line at any size. A lone chevron was drawn and rejected -- DisclosureIndicator draws
    // exactly that shape one row to the left in the settings pane, and one shape may not mean
    // two things.
    //
    // The number sits in a MEASURED SLOT, not a self-sized one. With the age now inside the
    // verdict badge there is nothing anchored to the right of this bar, so a command id growing
    // from 0 to 12 would shift every badge beside it.
    component CmdChip: Rectangle {
        id: chip
        property int cmd: 0
        property string numSample: "0"
        // The theme's chip height, already in this panel's scale space (root._chipH). Everything
        // else about the chip is a fraction of it, so one number moves the whole thing when the
        // theme's control height changes.
        property real h: Tokens.chipH
        // The id is a number the operator READS, not a label to be recognised at a glance like
        // the verdict word beside it -- digits need more size than caps to be read, not less.
        property real fontPixelSize: Tokens.fontBase
        readonly property real _mark: Math.round(chip.h * 0.58)
        readonly property real _gap:  Math.round(chip.h * 0.21)
        readonly property real _padH: Math.round(chip.h * 0.29)
        implicitWidth: Math.ceil(_padH * 2 + _mark + _gap + _mNum.width)
        implicitHeight: chip.h
        radius: Tokens.radiusSm
        color: "transparent"
        border.width: Math.max(1, Math.round(chip.h / 22))
        border.color: AppPalette.border

        TextMetrics { id: _mNum; font: _numText.font; text: chip.numSample }

        Row {
            anchors.left: parent.left
            anchors.leftMargin: chip._padH
            anchors.verticalCenter: parent.verticalCenter
            spacing: chip._gap
            UsblCmdMark {
                width: chip._mark
                height: chip._mark
                ink: chip._ink
                anchors.verticalCenter: parent.verticalCenter
            }
            Text {
                id: _numText
                width: _mNum.width
                anchors.verticalCenter: parent.verticalCenter
                text: String(chip.cmd)
                color: chip._ink
                font.pixelSize: chip.fontPixelSize
                font.bold: true
                font.letterSpacing: font.pixelSize * 0.05
            }
        }

        // A CONTROL'S TEXT COLOUR, not the muted one. This chip names the command the host is
        // sending; muted made it read as a caption about the row rather than as part of it, and
        // every control in the app labels itself with AppPalette.text.
        readonly property color _ink: AppPalette.text
        // The mark repaints itself off its own `ink` binding.
    }

    // The range mark: a dimension line, which is the drawing convention for "this is a length"
    // and -- unlike a dot-and-ring span, which was drawn and rejected -- shares no shape with
    // the operation badge one line above it.
    component RangeMark: Canvas {
        id: rm
        property color ink: AppPalette.textMuted
        antialiasing: true
        onPaint: {
            var ctx = getContext("2d")
            if (!ctx)
                return
            ctx.clearRect(0, 0, width, height)
            var w = width, h = height, y = h / 2
            ctx.strokeStyle = rm.ink
            ctx.fillStyle = rm.ink
            ctx.lineCap = "round"
            ctx.lineWidth = Math.max(1, w * 0.094)
            ctx.beginPath()
            ctx.moveTo(w * 0.094, h * 0.22); ctx.lineTo(w * 0.094, h * 0.78)
            ctx.moveTo(w * 0.906, h * 0.22); ctx.lineTo(w * 0.906, h * 0.78)
            ctx.stroke()
            ctx.lineWidth = Math.max(1, w * 0.081)
            ctx.beginPath()
            ctx.moveTo(w * 0.25, y); ctx.lineTo(w * 0.75, y)
            ctx.stroke()
            ctx.beginPath()
            ctx.moveTo(w * 0.288, h * 0.35); ctx.lineTo(w * 0.113, y)
            ctx.lineTo(w * 0.288, h * 0.65); ctx.closePath(); ctx.fill()
            ctx.beginPath()
            ctx.moveTo(w * 0.712, h * 0.35); ctx.lineTo(w * 0.887, y)
            ctx.lineTo(w * 0.712, h * 0.65); ctx.closePath(); ctx.fill()
        }
        onInkChanged: rm.requestPaint()
        onWidthChanged: rm.requestPaint()
        onHeightChanged: rm.requestPaint()
        Component.onCompleted: rm.requestPaint()
    }

    // ── what the card has to be wide enough for ───────────────────────────
    // Laid out but never drawn, at exactly the sizes the real rows use, out of exactly the
    // components the real rows use -- a prototype that drifts from the row measures the wrong
    // thing.
    //
    // EVERY SAMPLE HERE IS STATIC: the widest string each slot's formatter can produce, not the
    // widest one currently on screen. That is the whole point. The card's width is then a
    // constant for a given theme and language, and no beacon can change it.
    readonly property string _cmdSample: "888"
    // The widest _fmtAge can return: three digits and a unit. See its tiers above.
    readonly property string _ageSample: _fmtAge(3596400000)
    // Four digits and a decimal -- past any acoustic range this protocol carries.
    readonly property string _rangeSample: "8888.8"

    Item {
        visible: false
        enabled: false
        Row {
            id: _protoStatus
            spacing: Math.round(6 * root._k)
            UsblAddressBadge { address: 8; diameter: root._chipH }
            UsblOpBadge { diameter: root._chipH }
            CmdChip { h: root._chipH; numSample: root._cmdSample; fontPixelSize: root._cmdFont }
            UsblStateBadge {
                chipHeight: root._chipH
                fontPixelSize: root._wordFont
                ageFontPixelSize: root._ageFont
                ageSample: root._ageSample
            }
        }
        Row {
            id: _protoRead
            spacing: Math.round(6 * root._k)
            Item { width: Math.round(root._chipH * 0.66); height: root._chipH }
            Text {
                text: root._rangeSample
                font.pixelSize: root._rangeFont; font.bold: true
            }
            Text { text: qsTr("m"); font.pixelSize: Math.max(7, Math.round(11 * root._k)) }
            Text {
                text: "888 " + qsTr("dB")
                font.pixelSize: Math.max(7, Math.round(11 * root._k))
            }
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
                        // THE FRAME IS THE CURSOR, exactly as in the settings pane: the node
                        // last interrogated, or -- before anything has been asked -- the one the
                        // next Step will take. Every other row carries no border at all, so the
                        // frame is findable rather than being one outline among eight.
                        //
                        // It outlives the answer window closing, which is the point: with no
                        // cycle strip anywhere, this is the only thing that says where in the
                        // schedule you are.
                        border.width: (nodeRow._r && nodeRow._r.cursor)
                                      ? Math.max(1, Math.round(1.5 * root._k)) : 0
                        border.color: AppPalette.accentBorder
                        Behavior on color { ColorAnimation { duration: 200; easing.type: Easing.OutCubic } }
                        // Switched off keeps its numbers, dimmed: the last thing a node said is
                        // worth reading after you stop asking it.
                        opacity: (nodeRow._r && nodeRow._r.active) ? 1.0 : 0.62
                    }

                    // THE STATUS BAR, on top: who, what we are doing, what we last asked, what
                    // came back -- and on the right, how old the readings underneath are.
                    //
                    // Everything that is a STATE lives on this line and everything that is a
                    // READING lives on the next, which is the same shape the settings pane's row
                    // already has. It used to be the other way round here.
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
                        UsblOpBadge {
                            code: nodeRow._r ? nodeRow._r.op : "idle"
                            diameter: root._chipH
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        // WHICH command was last asked. The verdict badge beside it reports the
                        // last RESOLVED outcome, so the two describe different commands for as
                        // long as a request is out -- both true, and that is the point. Absent
                        // until something has been asked: a chip about no command is worse than
                        // no chip.
                        CmdChip {
                            h: root._chipH
                            visible: !!nodeRow._r && nodeRow._r.lastCmd >= 0
                            cmd: nodeRow._r ? nodeRow._r.lastCmd : 0
                            numSample: root._cmdSample
                            fontPixelSize: root._cmdFont
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        // The verdict, and how old the readings under it are. The age lives in
                        // here rather than in a chip of its own: it is a fact ABOUT this
                        // exchange's numbers, and one chip carrying both is one less object on a
                        // bar that already has three. It warns in ink only -- filling the chip
                        // stays the verdict's alone, or a node that is answering but has not
                        // been asked lately would look like a failure.
                        UsblStateBadge {
                            code: nodeRow._r ? nodeRow._r.reply : "none"
                            chipHeight: root._chipH
                            fontPixelSize: root._wordFont
                            ageFontPixelSize: root._ageFont
                            age: (nodeRow._r && nodeRow._r.ageMs >= 0)
                                 ? root._fmtAge(nodeRow._r.ageMs) : ""
                            aged: !!(nodeRow._r && nodeRow._r.aged)
                            lost: !!(nodeRow._r && nodeRow._r.lost)
                            ageSample: root._ageSample
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }

                    // THE READINGS, underneath: the number this panel is read for, at a size the
                    // freed line can carry, with a mark saying what kind of number it is.
                    Row {
                        id: _line2
                        anchors.left: parent.left
                        anchors.leftMargin: Math.round(6 * root._k)
                        anchors.bottom: parent.bottom
                        anchors.bottomMargin: Math.round(4 * root._k)
                        spacing: Math.round(6 * root._k)

                        RangeMark {
                            width: Math.round(root._chipH * 0.66)
                            height: width
                            ink: AppPalette.textMuted
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        Text {
                            id: _range
                            text: root._num(nodeRow._r && nodeRow._r.entry
                                            ? nodeRow._r.entry.distance : NaN, 1)
                            color: AppPalette.textStrong
                            font.pixelSize: root._rangeFont
                            font.bold: true
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        // On the number's baseline, not its centre: a unit floating beside a
                        // 22 px figure reads as part of it only if the two sit on one line.
                        Text {
                            text: qsTr("m")
                            color: AppPalette.textMuted
                            font.pixelSize: Math.max(7, Math.round(11 * root._k))
                            anchors.baseline: _range.baseline
                        }
                    }

                    // SNR is absent on the v1/v2 solution paths -- the payload carries none and
                    // the projection leaves it NAN -- so this is an em dash far more often than
                    // it is a number. It used to read a confident 0.
                    Text {
                        anchors.right: parent.right
                        anchors.rightMargin: Math.round(6 * root._k)
                        anchors.verticalCenter: _line2.verticalCenter
                        text: root._num(nodeRow._r && nodeRow._r.entry
                                        ? nodeRow._r.entry.snr : NaN, 0) + " " + qsTr("dB")
                        color: AppPalette.textMuted
                        font.pixelSize: Math.max(7, Math.round(11 * root._k))
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
