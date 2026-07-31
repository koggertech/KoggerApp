import QtQuick 2.15
import kqml_types 1.0

// Configuration half of the USBL UI: the command plan.
//
// A group owns an exclusive set of cmd slots and declares both halves of a transaction.
// The role selector decides which half is written to the device — it deliberately does
// NOT change this view, because the plan is one document regardless of which end you are
// configuring. See docs/KoggerApp-Docs/usbl-protocol.md.
DeviceSettingsGroup {
    id: planGroup

    property var dev: null
    property var plan: null

    // VESTIGIAL. The store now REPLACES its whole state on every edit (UsblPlanLogic.js),
    // so `plan.groups`, `plan.trigger()` and friends already change identity and bindings
    // re-evaluate on their own. The few `var _d = _rev` reads left below are harmless
    // no-ops kept to avoid churning working bindings -- do not copy the pattern.
    readonly property int _rev: plan ? plan.rev : 0

    title: qsTr("USBL command plan")
    titlePixelSize: 13
    stateKey: "dev.usblPlan"
    collapsedByDefault: true
    visible: !!(dev && (dev.isUSBL || dev.isUSBLBeacon))

    // The active group. No copy needed: the store replaces its whole state on every edit, so
    // `plan.groups` is a new array of new objects each time and every binding reading
    // `_g.ini.reply` re-evaluates by itself. Never write through this — go via the store.
    readonly property var _g: (plan && plan.groups.length)
                              ? plan.groups[Math.min(plan.activeGroup, plan.groups.length - 1)]
                              : null
    property string _slotNote: qsTr("Exclusive — taking a slot removes it from the group that held it.")
    // Inbound wiring accent — see UsblTriggerEditor.
    readonly property color _violet: AppPalette.isDark ? "#A78BFA" : "#7C5CD3"

    // Stale marker rides the header so it is visible while the group is collapsed.
    headerActions: Row {
        spacing: Tokens.spaceSm
        rightPadding: Tokens.spaceSm
        Rectangle {
            visible: !!(plan && plan.stale)
            anchors.verticalCenter: parent ? parent.verticalCenter : undefined
            implicitWidth: _staleTxt.implicitWidth + Tokens.spaceLg * 2
            width: implicitWidth
            height: planGroup.headerActionSize - Tokens.spaceSm * 2
            radius: height / 2
            color: "transparent"
            border.width: Math.max(1, Math.round(1.5 * AppPalette.scale))
            border.color: AppPalette.linkIdleBorder
            Text {
                id: _staleTxt
                anchors.centerIn: parent
                text: qsTr("re-apply")
                color: AppPalette.linkIdleText
                font.pixelSize: Tokens.fontXs; font.bold: true
            }
        }
        Text {
            visible: !!(plan && plan.appliedOnce && !plan.stale)
            anchors.verticalCenter: parent ? parent.verticalCenter : undefined
            text: qsTr("applied as %1").arg(plan ? plan.role : "")
            color: AppPalette.textMuted
            font.pixelSize: Tokens.fontXs
        }
    }

    component MiniSwitch: Rectangle {
        id: ms
        property bool checked: false
        signal toggled()
        implicitWidth: Math.round(36 * AppPalette.scale)
        implicitHeight: Math.round(20 * AppPalette.scale)
        radius: height / 2
        color: ms.checked ? AppPalette.toggleOn : AppPalette.trackOff
        border.width: Math.max(1, Math.round(1 * AppPalette.scale))
        border.color: ms.checked ? AppPalette.toggleOnBorder : AppPalette.trackOffBorder
        Rectangle {
            width: ms.height - Math.round(6 * AppPalette.scale); height: width; radius: width / 2
            y: (ms.height - height) / 2
            x: ms.checked ? ms.width - width - y : y
            color: AppPalette.knob
            Behavior on x { NumberAnimation { duration: 140; easing.type: Easing.OutCubic } }
        }
        KTapArea { anchors.fill: parent; onTapped: ms.toggled() }
    }

    // A labelled field row sized like the rest of the settings page.
    component FieldRow: Row {
        id: fr
        property string label: ""
        default property alias fieldData: _fr.data
        width: parent ? parent.width : 0
        height: Tokens.controlHMd
        spacing: Tokens.spaceSm
        Text {
            text: fr.label
            color: AppPalette.textMuted
            font.pixelSize: Tokens.fontSm
            width: Math.round(74 * AppPalette.scale)
            elide: Text.ElideRight
            anchors.verticalCenter: parent.verticalCenter
        }
        Row {
            id: _fr
            spacing: Tokens.spaceSm
            // No `height: parent.height` -- the enclosing Row sizes itself from its
            // children, so binding back to it is a feedback loop. verticalCenter already
            // does the alignment this was reaching for.
            anchors.verticalCenter: parent.verticalCenter
        }
    }

    // ── apply as role ─────────────────────────────────────────────────────
    Text {
        text: qsTr("Apply this plan as")
        color: AppPalette.textMuted; font.pixelSize: Tokens.fontSm; font.bold: true
    }

    Row {
        id: roleRow
        width: parent.width
        spacing: Tokens.spaceSm
        readonly property real cellW: (width - spacing) / 2

        Repeater {
            model: [
                { "id": "initiator",   "name": qsTr("Initiator"),   "sub": qsTr("asks · reads responses") },
                { "id": "transponder", "name": qsTr("Transponder"), "sub": qsTr("is asked · answers requests") }
            ]
            delegate: Rectangle {
                required property var modelData
                readonly property bool _sel: planGroup.plan && planGroup.plan.role === modelData.id
                width: roleRow.cellW
                radius: Tokens.radiusMd
                color: _sel ? AppPalette.accentBg : AppPalette.card
                border.width: _sel ? Math.max(1, Math.round(1.5 * AppPalette.scale)) : Tokens.cardBorderWidth
                border.color: _sel ? AppPalette.accentBorder : AppPalette.border
                implicitHeight: _rb.implicitHeight + 2 * Tokens.spaceMd
                Column {
                    id: _rb
                    x: Tokens.spaceMd; y: Tokens.spaceMd
                    width: parent.width - 2 * Tokens.spaceMd
                    spacing: 0
                    Text {
                        text: modelData.name; color: AppPalette.textStrong
                        font.pixelSize: Tokens.fontSm; font.bold: true
                    }
                    Text {
                        text: modelData.sub; color: AppPalette.textMuted
                        font.pixelSize: Tokens.fontXs
                        width: parent.width; elide: Text.ElideRight
                    }
                }
                KTapArea {
                    anchors.fill: parent
                    onTapped: if (plan) plan.setRole(modelData.id)
                }
            }
        }
    }

    // ── apply ─────────────────────────────────────────────────────────────
    Rectangle {
        width: parent.width
        radius: Tokens.radiusMd
        color: AppPalette.rowRaised
        border.width: Tokens.cardBorderWidth
        border.color: AppPalette.border
        implicitHeight: _applyCol.implicitHeight + 2 * Tokens.spaceMd
        Column {
            id: _applyCol
            x: Tokens.spaceMd; y: Tokens.spaceMd
            width: parent.width - 2 * Tokens.spaceMd
            spacing: Tokens.spaceSm
            // Releases are destructive on the device, so they are named before you press it.
            Text {
                visible: !!(plan && plan.releaseFrames > 0)
                width: parent.width; wrapMode: Text.WordWrap
                color: AppPalette.linkIdleText; font.pixelSize: Tokens.fontXs; font.bold: true
                text: qsTr("%1 slot(s) will be switched off — configured earlier, no longer in this plan")
                      .arg(plan ? plan.releaseFrames : 0)
            }
            UsblButton {
                text: qsTr("Apply as %1").arg(plan ? plan.role : "")
                implicitHeight: Tokens.controlHSm
                fontPixelSize: Tokens.fontXs
                enabled: !!(dev && plan && (plan.setupFrames > 0 || plan.releaseFrames > 0))
                onClicked: planGroup._apply()
            }
        }
    }

    // Writes only the half the selected role owns. Slot frames are per cmd_id — there is
    // no wildcard, so a group of N slots costs N frames.
    //
    // One section fits USBLCmdSlotConfig; two sections, or any rewrite rule, need
    // USBLCmdConfig. structOf() decides, and the two paths use DIFFERENT Function
    // numbering — cfgWire for USBLCmdConfig, slotWire for USBLCmdSlotConfig.
    function _apply() {
        if (!dev || !plan) return

        // Release first: slots this role wrote previously but no longer configures. The
        // device keeps whatever it was last told, so detaching a handler in the UI changes
        // nothing until Disabled is actually sent.
        var stale = plan.staleWrites()
        for (var s = 0; s < stale.length; ++s)
            dev.setUsblCmdSlotDisposition(stale[s].cmd, stale[s].event, 0)   // FunctionDisabled

        for (var i = 0; i < plan.groups.length; ++i) {
            var g = plan.groups[i]
            var t = plan.role === "initiator" ? g.ini.reply : g.tr.request
            if (!t) continue
            var asCmdConfig = plan.structOf(t) === "USBLCmdConfig"
            var ev = plan.role === "initiator" ? 2 : 1
            for (var j = 0; j < g.slots.length; ++j) {
                var cmd = g.slots[j]
                if (asCmdConfig) {
                    var rf = t.recv ? plan.findBy(plan.formats, t.recv.fmt).cfgWire : 0
                    var sf = t.send ? plan.findBy(plan.formats, t.send.fmt).cfgWire : 0
                    dev.setUsblCmdConfig(cmd, ev,
                                         rf, t.recv ? t.recv.bits : 0,
                                         sf, t.send ? t.send.payload : "",
                                         t.adv.eventAction === "Same" ? 1 : 0,
                                         t.adv.cmdIdAction === "Replacement" ? 1 : 0, t.adv.cmdIdRepl,
                                         t.adv.addrAction === "Replacement" ? 1 : 0, t.adv.addrRepl)
                } else if (t.send) {
                    dev.setCmdSlotAsModemResponse(cmd, t.send.payload,
                                                  plan.payloadBytes(t.send.payload) * 8)
                } else if (t.recv) {
                    dev.setCmdSlotAsModemReceiver(cmd, t.recv.bits)
                } else {
                    // Nothing attached — the disposition IS the configuration, and it is
                    // the only way to switch a slot the device already holds back off.
                    dev.setUsblCmdSlotDisposition(cmd, ev,
                        plan.findBy(plan.dispositions, t.disposition).wire)
                }
            }
        }
        if (plan.role === "transponder") {
            var accepted = []
            for (var n = 0; n < plan.nodes.length; ++n)
                if (plan.nodes[n].active) accepted.push(plan.nodes[n].addr)
            dev.acousticResponceFilterSlots(accepted)
            dev.setUsblTransponderEnable(true)
        }
        plan.markApplied()
    }

    Rectangle { width: parent.width; height: 1; color: AppPalette.border }

    // ── slot coverage ─────────────────────────────────────────────────────
    Row {
        width: parent.width; height: Tokens.controlHSm; spacing: Tokens.spaceSm
        Text {
            text: qsTr("Slot coverage"); color: AppPalette.textMuted
            font.pixelSize: Tokens.fontSm; font.bold: true
            anchors.verticalCenter: parent.verticalCenter
        }
        Text {
            text: qsTr("8 hardware slots")
            color: AppPalette.textMuted; font.pixelSize: Tokens.fontXs
            width: Math.max(0, parent.width - Math.round(90 * AppPalette.scale) - parent.spacing)
            horizontalAlignment: Text.AlignRight
            elide: Text.ElideRight
            anchors.verticalCenter: parent.verticalCenter
        }
    }

    Grid {
        width: parent.width
        columns: 8
        columnSpacing: Tokens.spaceXxs
        rowSpacing: Tokens.spaceXxs
        id: covGrid
        readonly property real cellW: (width - columnSpacing * 7) / 8
        Repeater {
            model: planGroup.plan ? planGroup.plan.coverage : []
            delegate: Rectangle {
                id: covCell
                required property var modelData
                readonly property bool _owned: covCell.modelData.groupId >= 0
                readonly property color _c: covCell._owned && planGroup.plan
                    ? planGroup.plan.groupColors[covCell.modelData.index % planGroup.plan.groupColors.length]
                    : AppPalette.border
                width: covGrid.cellW
                implicitHeight: Math.round(34 * AppPalette.scale)
                radius: Tokens.radiusSm
                color: covCell._owned ? Qt.rgba(covCell._c.r, covCell._c.g, covCell._c.b, 0.16) : "transparent"
                border.width: Math.max(1, Math.round(1 * AppPalette.scale))
                border.color: covCell._owned ? covCell._c : AppPalette.border
                Column {
                    anchors.centerIn: parent
                    spacing: 0
                    Text {
                        text: String(covCell.modelData.cmd)
                        color: covCell._owned ? covCell._c : AppPalette.textMuted
                        font.pixelSize: Tokens.fontSm; font.bold: true
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    Text {
                        text: covCell._owned ? "G" + (covCell.modelData.index + 1) : "—"
                        color: AppPalette.textMuted
                        font.pixelSize: Math.round(9 * AppPalette.scale)
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                }
                KTapArea {
                    anchors.fill: parent
                    active: covCell._owned
                    onTapped: if (planGroup.plan) planGroup.plan.activeGroup = covCell.modelData.index
                }
            }
        }
    }

    // ── command groups: tabs ──────────────────────────────────────────────
    Text {
        text: qsTr("Command groups"); color: AppPalette.textMuted
        font.pixelSize: Tokens.fontSm; font.bold: true
    }

    Flickable {
        width: parent.width
        implicitHeight: _tabRow.implicitHeight
        contentWidth: _tabRow.implicitWidth
        contentHeight: _tabRow.implicitHeight
        flickableDirection: Flickable.HorizontalFlick
        clip: true
        Row {
            id: _tabRow
            spacing: Tokens.spaceXxs
            Repeater {
                // plan.groupsView, not plan.groups: the view is rebuilt whenever `rev`
                // changes, so the chips follow in-place slot edits. Binding to `groups`
                // and calling slotLabel() left the label frozen at whatever it was when
                // the delegate was created.
                model: plan ? plan.groupsView : []
                delegate: Rectangle {
                    required property var modelData
                    required property int index
                    // id so the Row's children can reach _sel/_c -- ancestor properties are not in
                    // QML scope, and a bare reference there is a silent runtime ReferenceError.
                    id: _tabChip
                    readonly property bool _sel: plan && plan.activeGroup === index
                    readonly property color _c: _tabChip.modelData.color
                    radius: Tokens.radiusSm
                    color: _sel ? Qt.rgba(_c.r, _c.g, _c.b, 0.14) : "transparent"
                    border.width: Tokens.cardBorderWidth
                    border.color: _sel ? _c : "transparent"
                    implicitWidth: _tabContent.implicitWidth + Tokens.spaceMd * 2
                    implicitHeight: Tokens.controlHMd
                    Row {
                        id: _tabContent
                        anchors.centerIn: parent
                        spacing: Tokens.spaceXs
                        Rectangle {
                            width: Math.round(7 * AppPalette.scale); height: width; radius: 2
                            color: _tabChip._c
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        Text {
                            text: "G" + (index + 1)
                            color: _tabChip._sel ? AppPalette.textStrong : AppPalette.textMuted
                            font.pixelSize: Tokens.fontSm; font.bold: true
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        Text {
                            text: _tabChip.modelData.label
                            color: AppPalette.textMuted
                            font.pixelSize: Tokens.fontXs
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                    KTapArea { anchors.fill: parent; onTapped: if (plan) plan.activeGroup = index }
                }
            }
            Rectangle {
                radius: Tokens.radiusSm
                color: "transparent"
                border.width: Tokens.cardBorderWidth
                border.color: "transparent"
                implicitWidth: Tokens.controlHMd; implicitHeight: Tokens.controlHMd
                Text {
                    anchors.centerIn: parent
                    text: "+"; color: AppPalette.accent
                    font.pixelSize: Tokens.fontXl; font.bold: true
                }
                KTapArea { anchors.fill: parent; onTapped: if (plan) plan.addGroup() }
            }
        }
    }

    // ── active group pane ─────────────────────────────────────────────────
    Rectangle {
        width: parent.width
        visible: !!planGroup._g
        radius: Tokens.radiusMd
        color: AppPalette.rowRaised
        border.width: Tokens.cardBorderWidth
        border.color: AppPalette.border
        implicitHeight: _pane.implicitHeight + 2 * Tokens.spaceMd

        Rectangle {
            width: parent.width; height: Math.max(2, Math.round(2 * AppPalette.scale))
            radius: height / 2
            color: {
                var _d = planGroup._rev
                return (planGroup._g && plan) ? plan.colorOf(planGroup._g) : AppPalette.accent
            }
        }

        Column {
            id: _pane
            x: Tokens.spaceMd; y: Tokens.spaceMd
            width: parent.width - 2 * Tokens.spaceMd
            spacing: Tokens.spaceMd

            // slots — exclusive selectable buttons
            Row {
                width: parent.width; height: Tokens.controlHSm; spacing: Tokens.spaceSm
                Text {
                    text: qsTr("cmd slots"); color: AppPalette.textMuted
                    font.pixelSize: Tokens.fontSm; font.bold: true
                    anchors.verticalCenter: parent.verticalCenter
                }
                Text {
                    // plan.activeSlotCount, not _g.slots.length: reading a field off the
                    // mutable group object is not a tracked dependency, so the count froze.
                    text: qsTr("%1 of %2").arg(plan ? plan.activeSlotCount : 0)
                                          .arg(plan ? plan.slotCount : 8)
                    color: AppPalette.textMuted; font.pixelSize: Tokens.fontXs
                    width: Math.max(0, parent.width - Math.round(70 * AppPalette.scale)
                                    - Tokens.controlHSm - parent.spacing * 2)
                    horizontalAlignment: Text.AlignRight
                    anchors.verticalCenter: parent.verticalCenter
                }
                KCircleIconButton {
                    glyph: "×"
                    width: Tokens.controlHSm; height: Tokens.controlHSm
                    glyphPixelSize: Math.round(14 * AppPalette.scale)
                    borderWidth: Tokens.cardBorderWidth
                    anchors.verticalCenter: parent.verticalCenter
                    toolTipText: qsTr("Remove this group")
                    onClicked: if (plan && planGroup._g) plan.removeGroup(planGroup._g.id)
                }
            }

            Grid {
                id: slotGrid
                width: parent.width
                columns: 8
                columnSpacing: Tokens.spaceXxs
                rowSpacing: Tokens.spaceXxs
                readonly property real cellW: (width - columnSpacing * 7) / 8
                Repeater {
                    model: planGroup.plan ? planGroup.plan.slotCount : 8
                    delegate: Rectangle {
                        id: slotBtn
                        required property int index
                        // Ownership comes from plan.coverage — the same rev-derived array the
                        // coverage strip above uses, so the two can no longer disagree.
                        // plan.ownerOf() in a binding is a function call with no dependency on
                        // `rev`, which is why these buttons kept showing a stale selection
                        // while the strip was correct.
                        readonly property var _cov: (planGroup.plan
                            && slotBtn.index < planGroup.plan.coverage.length)
                            ? planGroup.plan.coverage[slotBtn.index] : null
                        readonly property int _ownerId: slotBtn._cov ? slotBtn._cov.groupId : -1
                        readonly property bool _mine: !!(planGroup._g
                            && slotBtn._ownerId === planGroup._g.id)
                        readonly property bool _taken: slotBtn._ownerId >= 0 && !slotBtn._mine
                        readonly property color _gc: planGroup._g && planGroup.plan
                            ? planGroup.plan.colorOf(planGroup._g) : AppPalette.accent
                        readonly property color _oc: (slotBtn._ownerId >= 0 && planGroup.plan)
                            ? planGroup.plan.groupColors[slotBtn._cov.index
                                % planGroup.plan.groupColors.length]
                            : AppPalette.border
                        width: slotGrid.cellW
                        implicitHeight: Tokens.controlHMd
                        radius: Tokens.radiusSm
                        color: slotBtn._mine
                               ? Qt.rgba(slotBtn._gc.r, slotBtn._gc.g, slotBtn._gc.b, 0.22) : AppPalette.card
                        border.width: Math.max(1, Math.round(1 * AppPalette.scale))
                        border.color: slotBtn._mine ? slotBtn._gc
                                    : (slotBtn._taken
                                       ? Qt.rgba(slotBtn._oc.r, slotBtn._oc.g, slotBtn._oc.b, 0.55)
                                       : AppPalette.border)
                        Text {
                            anchors.centerIn: parent
                            text: String(slotBtn.index)
                            color: slotBtn._mine ? slotBtn._gc : AppPalette.textMuted
                            font.pixelSize: Tokens.fontSm; font.bold: true
                        }
                        // Owner marker, so a taken slot shows where it currently lives.
                        Rectangle {
                            visible: slotBtn._taken
                            width: Math.round(5 * AppPalette.scale); height: width; radius: width / 2
                            anchors.top: parent.top; anchors.right: parent.right
                            anchors.margins: Math.round(2 * AppPalette.scale)
                            color: slotBtn._oc
                        }
                        KTapArea {
                            anchors.fill: parent
                            onTapped: {
                                if (!planGroup.plan || !planGroup._g) return
                                var prev = planGroup.plan.toggleSlot(planGroup._g.id, slotBtn.index)
                                planGroup._slotNote = prev
                                    ? qsTr("slot %1 taken from group %2")
                                          .arg(slotBtn.index)
                                          .arg(planGroup.plan.groupIndexById(prev.id) + 1)
                                    : qsTr("Exclusive — taking a slot removes it from the group that held it.")
                            }
                        }
                    }
                }
            }

            Text {
                width: parent.width; wrapMode: Text.WordWrap
                text: planGroup._slotNote
                color: AppPalette.textMuted; font.pixelSize: Tokens.fontXs
            }

            // ══ INITIATOR ══
            // The accent rail must NOT be a child of the Column it spans: a Column derives
            // its height from its children, so `height: parent.height` on a child closes a
            // feedback loop. Qt detects the polish() loop and ABORTS the layout pass -- which
            // silently breaks every positioner on the page, not just this one. Rail and
            // content are siblings inside a plain Item instead.
            Item {
                width: parent.width
                implicitHeight: _iniCol.implicitHeight

                Rectangle {
                    width: Math.max(2, Math.round(2 * AppPalette.scale))
                    height: parent.height
                    color: AppPalette.accent
                }

                Column {
                    id: _iniCol
                    x: Tokens.spaceMd
                    width: parent.width - Tokens.spaceMd
                    spacing: Tokens.spaceSm

                    Row {
                        width: parent.width
                        spacing: Tokens.spaceSm
                        Text {
                            text: qsTr("Initiator"); color: AppPalette.textStrong
                            font.pixelSize: Tokens.fontSm; font.bold: true
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        Text {
                            text: plan && planGroup._g ? plan.subroleInitiator(planGroup._g) : ""
                            color: AppPalette.textMuted; font.pixelSize: Tokens.fontXs
                            font.italic: true
                            width: Math.max(0, parent.width - Math.round(60 * AppPalette.scale) - parent.spacing)
                            horizontalAlignment: Text.AlignRight
                            elide: Text.ElideRight
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }

                    // Send request — USBLPingRequest, one frame per schedule step
                    Rectangle {
                        visible: !!(planGroup._g && planGroup._g.ini.send)
                        width: parent.width
                        radius: Tokens.radiusSm
                        color: AppPalette.card
                        border.width: Tokens.cardBorderWidth
                        border.color: AppPalette.border
                        implicitHeight: _sendCol.implicitHeight + 2 * Tokens.spaceSm
                        Column {
                            id: _sendCol
                            x: Tokens.spaceMd; y: Tokens.spaceSm
                            width: parent.width - 2 * Tokens.spaceMd
                            spacing: Tokens.spaceXs
                            readonly property var _snd: planGroup._g ? planGroup._g.ini.send : null
                            readonly property var _fn: {
                                var _d = planGroup._rev
                                return (plan && _snd) ? plan.findBy(plan.pingFunctions, _snd.fn) : null
                            }

                            Row {
                                width: parent.width; spacing: Tokens.spaceSm
                                Text {
                                    text: qsTr("Send request"); color: AppPalette.textStrong
                                    font.pixelSize: Tokens.fontXs; font.bold: true
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                                // Spacer, not a label: the wire struct name belongs in the
                                // protocol doc, not on an operator's screen. Keeps the ×
                                // pushed to the right edge.
                                Item {
                                    width: Math.max(0, parent.width - Math.round(80 * AppPalette.scale)
                                                    - Tokens.controlHSm - parent.spacing * 2)
                                    height: 1
                                }
                                KCircleIconButton {
                                    glyph: "×"
                                    width: Tokens.controlHSm; height: Tokens.controlHSm
                                    glyphPixelSize: Math.round(13 * AppPalette.scale)
                                    borderWidth: Tokens.cardBorderWidth
                                    anchors.verticalCenter: parent.verticalCenter
                                    toolTipText: qsTr("Remove")
                                    onClicked: if (plan && planGroup._g) plan.detachSend(planGroup._g.id)
                                }
                            }
                            FieldRow {
                                label: qsTr("carries")
                                KCombo {
                                    model: plan ? plan.pingFunctions.map(function (f) { return f.label }) : []
                                    currentIndex: {
                                        if (!plan || !_sendCol._snd) return 0
                                        for (var i = 0; i < plan.pingFunctions.length; ++i)
                                            if (plan.pingFunctions[i].id === _sendCol._snd.fn) return i
                                        return 0
                                    }
                                    implicitWidth: Math.round(190 * AppPalette.scale)
                                    fontPixelSize: Tokens.fontXs
                                    anchors.verticalCenter: parent.verticalCenter
                                    onActivated: function (i) {
                                        if (plan && planGroup._g)
                                            plan.setSendField(planGroup._g.id, "fn", plan.pingFunctions[i].id)
                                    }
                                }
                            }
                            FieldRow {
                                label: qsTr("payload")
                                visible: !!(_sendCol._fn && _sendCol._fn.payload)
                                UsblHexField {
                                    value: _sendCol._snd ? _sendCol._snd.payload : ""
                                    placeholder: qsTr("hex bytes")
                                    implicitWidth: Math.round(140 * AppPalette.scale)
                                    anchors.verticalCenter: parent.verticalCenter
                                    onCommitted: function (v) {
                                        if (plan && planGroup._g)
                                            plan.setSendField(planGroup._g.id, "payload", v)
                                    }
                                }
                                Text {
                                    text: {
                                        var _d = planGroup._rev
                                        return qsTr("%1 bit").arg(plan && _sendCol._snd
                                            ? plan.payloadBytes(_sendCol._snd.payload) * 8 : 0)
                                    }
                                    color: AppPalette.textMuted; font.pixelSize: Tokens.fontXs
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                            }
                            FieldRow {
                                label: qsTr("max range")
                                UsblSpin {
                                    from: 0; to: 2000000; stepSize: 1000
                                    devValue: _sendCol._snd ? _sendCol._snd.reply : 0
                                    implicitWidth: Math.round(110 * AppPalette.scale)
                                    anchors.verticalCenter: parent.verticalCenter
                                    writeBack: function (v) {
                                        if (plan && planGroup._g) plan.setSendField(planGroup._g.id, "reply", v)
                                    }
                                }
                                Text {
                                    text: (_sendCol._snd && _sendCol._snd.reply === 0)
                                          ? qsTr("mm · expect no reply") : qsTr("mm")
                                    color: AppPalette.textMuted; font.pixelSize: Tokens.fontXs
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                            }
                        }
                    }

                    UsblTriggerEditor {
                        visible: !!(planGroup._g && planGroup._g.ini.reply)
                        width: parent.width
                        plan: planGroup.plan
                        groupId: planGroup._g ? planGroup._g.id : -1
                        which: "response"
                    }

                    Row {
                        width: parent.width
                        spacing: Tokens.spaceSm
                        UsblButton {
                            visible: !!(planGroup._g && !planGroup._g.ini.send)
                            text: qsTr("+ Send request")
                            implicitHeight: Tokens.controlHSm; fontPixelSize: Tokens.fontXs
                            onClicked: if (plan && planGroup._g) plan.attachSend(planGroup._g.id)
                        }
                        UsblButton {
                            visible: !!(planGroup._g && !planGroup._g.ini.reply)
                            text: qsTr("+ Handle reply")
                            implicitHeight: Tokens.controlHSm; fontPixelSize: Tokens.fontXs
                            onClicked: if (plan && planGroup._g) plan.attachTrigger(planGroup._g.id, "reply")
                        }
                    }
                }
            }

            // ══ TRANSPONDER ══
            // The accent rail must NOT be a child of the Column it spans: a Column derives
            // its height from its children, so `height: parent.height` on a child closes a
            // feedback loop. Qt detects the polish() loop and ABORTS the layout pass -- which
            // silently breaks every positioner on the page, not just this one. Rail and
            // content are siblings inside a plain Item instead.
            Item {
                width: parent.width
                implicitHeight: _trCol.implicitHeight

                Rectangle {
                    width: Math.max(2, Math.round(2 * AppPalette.scale))
                    height: parent.height
                    color: planGroup._violet
                }

                Column {
                    id: _trCol
                    x: Tokens.spaceMd
                    width: parent.width - Tokens.spaceMd
                    spacing: Tokens.spaceSm

                    Row {
                        width: parent.width
                        spacing: Tokens.spaceSm
                        Text {
                            text: qsTr("Transponder"); color: AppPalette.textStrong
                            font.pixelSize: Tokens.fontSm; font.bold: true
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        Text {
                            text: plan && planGroup._g ? plan.subroleTransponder(planGroup._g) : ""
                            color: AppPalette.textMuted; font.pixelSize: Tokens.fontXs
                            font.italic: true
                            width: Math.max(0, parent.width - Math.round(80 * AppPalette.scale) - parent.spacing)
                            horizontalAlignment: Text.AlignRight
                            elide: Text.ElideRight
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }

                    UsblTriggerEditor {
                        visible: !!(planGroup._g && planGroup._g.tr.request)
                        width: parent.width
                        plan: planGroup.plan
                        groupId: planGroup._g ? planGroup._g.id : -1
                        which: "request"
                    }

                    UsblButton {
                        visible: !!(planGroup._g && !planGroup._g.tr.request)
                        text: qsTr("+ Handle request")
                        implicitHeight: Tokens.controlHSm; fontPixelSize: Tokens.fontXs
                        onClicked: if (plan && planGroup._g) plan.attachTrigger(planGroup._g.id, "request")
                    }
                    Text {
                        visible: !!(planGroup._g && !planGroup._g.tr.request)
                        width: parent.width
                        wrapMode: Text.WordWrap
                        text: qsTr("nothing written as transponder — slots keep device defaults")
                        color: AppPalette.textMuted; font.pixelSize: Tokens.fontXs
                    }
                }
            }
        }
    }

    Rectangle { width: parent.width; height: 1; color: AppPalette.border }

    // ── contract check ────────────────────────────────────────────────────
    Row {
        width: parent.width; height: Tokens.controlHSm; spacing: Tokens.spaceSm
        Text {
            text: qsTr("Plan check"); color: AppPalette.textMuted
            font.pixelSize: Tokens.fontSm; font.bold: true
            anchors.verticalCenter: parent.verticalCenter
        }
        Text {
            readonly property int _n: plan ? plan.issues.length : 0
            text: _n ? qsTr("%1 issue(s)").arg(_n) : qsTr("both halves agree")
            color: _n ? AppPalette.linkIdleText : AppPalette.textMuted
            font.pixelSize: Tokens.fontXs; font.bold: _n > 0
            width: Math.max(0, parent.width - Math.round(90 * AppPalette.scale) - parent.spacing)
            horizontalAlignment: Text.AlignRight
            elide: Text.ElideRight
            anchors.verticalCenter: parent.verticalCenter
        }
    }

    Column {
        width: parent.width
        spacing: Tokens.spaceXs
        Text {
            visible: !plan || !plan.issues.length
            width: parent.width; wrapMode: Text.WordWrap
            text: qsTr("Every group's request and response halves line up, so the two devices will understand each other.")
            color: AppPalette.textMuted; font.pixelSize: Tokens.fontXs
        }
        Repeater {
            model: plan ? plan.issues : []
            delegate: Rectangle {
                required property var modelData
                readonly property bool _crit: modelData.sev === "crit"
                width: parent.width
                radius: Tokens.radiusSm
                color: _crit ? AppPalette.dangerBg : AppPalette.linkIdleBg
                border.width: Tokens.cardBorderWidth
                border.color: _crit ? AppPalette.dangerBorder : AppPalette.linkIdleBorder
                implicitHeight: _iss.implicitHeight + 2 * Tokens.spaceSm
                Column {
                    id: _iss
                    x: Tokens.spaceMd; y: Tokens.spaceSm
                    width: parent.width - 2 * Tokens.spaceMd
                    spacing: 1
                    Text {
                        text: modelData.key
                        color: AppPalette.textStrong
                        font.pixelSize: Tokens.fontXs; font.bold: true
                    }
                    Text {
                        text: modelData.text
                        color: AppPalette.text
                        font.pixelSize: Tokens.fontXs
                        width: parent.width; wrapMode: Text.WordWrap
                    }
                }
            }
        }
    }
}

