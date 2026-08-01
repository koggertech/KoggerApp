import QtQuick 2.15
import kqml_types 1.0

// Configuration half of the USBL UI: the command plan.
//
// A group owns an exclusive set of cmd slots and declares BOTH halves of a transaction,
// so there is no role to be in. Role is an argument to Apply, chosen by which button you
// press at the bottom of the pane.
//
// Reading order is the working order: what the plan is, then what is wrong with it, then
// the button that sends it. See docs/KoggerApp-Docs/usbl-protocol.md.
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
    // What the last slot click did, so the bar's three behaviours are learnable by using it.
    property string _slotNote: qsTr("Click a slot: another group's opens it, this group's releases it, a free one joins it.")
    function _noteFor(action, cmd) {
        switch (action) {
        case "select":  return qsTr("slot %1 belongs to this group — click it again to release it").arg(cmd)
        case "release": return qsTr("slot %1 released — click it again to give it to this group").arg(cmd)
        case "assign":  return qsTr("slot %1 added to this group").arg(cmd)
        case "create":  return qsTr("new group created holding slot %1").arg(cmd)
        }
        return planGroup._slotNote
    }
    // Inbound wiring accent — see UsblTriggerEditor.
    readonly property color _violet: AppPalette.isDark ? "#A78BFA" : "#7C5CD3"

    // Stale marker rides the header so it is visible while the group is collapsed.
    headerActions: Row {
        spacing: Tokens.spaceSm
        rightPadding: Tokens.spaceSm
        Rectangle {
            visible: !!(plan && plan.anyStale)
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
        // Both roles can be applied, so the badge names the ones that are.
        Text {
            visible: text.length > 0
            anchors.verticalCenter: parent ? parent.verticalCenter : undefined
            text: {
                if (!plan || plan.anyStale) return ""
                var i = plan.applyInfo
                if (i["initiator"].applied && i["transponder"].applied) return qsTr("applied · both")
                if (i["initiator"].applied) return qsTr("applied · initiator")
                if (i["transponder"].applied) return qsTr("applied · transponder")
                return ""
            }
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

    // Writes ALL EIGHT slots for `role`, every time -- see Logic.applyWrites. Which frames
    // carry which bytes is decided there so `node` can assert it without a device or a
    // click; this loop only hands the numbers over.
    function _apply(role) {
        if (!dev || !plan) return

        var w = plan.applyWrites(role)
        for (var i = 0; i < w.length; ++i)
            dev.setUsblCmdConfig(w[i].cmd, w[i].event,
                                 w[i].recvFn, w[i].recvBits,
                                 w[i].sendFn, w[i].sendHex,
                                 w[i].eventAction,
                                 w[i].cmdIdAction, w[i].cmdIdRepl,
                                 w[i].addrAction, w[i].addrRepl)
        if (role === "transponder") {
            var accepted = []
            for (var n = 0; n < plan.nodes.length; ++n)
                if (plan.nodes[n].active) accepted.push(plan.nodes[n].addr)
            dev.acousticResponceFilterSlots(accepted)
            dev.setUsblTransponderEnable(true)
        }
        plan.markApplied(role)
    }

    // ── command groups ──────────────────
    Row {
        width: parent.width; height: Tokens.controlHSm; spacing: Tokens.spaceSm
        Text {
            text: qsTr("Command groups"); color: AppPalette.textMuted
            font.pixelSize: Tokens.fontSm; font.bold: true
            anchors.verticalCenter: parent.verticalCenter
        }
        Text {
            text: plan && plan.groups.length
                  ? qsTr("%1 of %2 groups").arg(plan.groups.length).arg(plan.maxGroups)
                  : qsTr("8 hardware slots, none claimed")
            color: AppPalette.textMuted; font.pixelSize: Tokens.fontXs
            width: Math.max(0, parent.width - Math.round(104 * AppPalette.scale)
                            - parent.spacing)
            horizontalAlignment: Text.AlignRight
            elide: Text.ElideRight
            anchors.verticalCenter: parent.verticalCenter
        }
    }

    // Explicit selection, above the bar that shows what the selection owns.
    //
    // The bar alone cannot do this job: a group holding NO slots has no cell in it and
    // would be unreachable — you could add a group and never select it again. These
    // chips also name a group before you have to decode a colour.
    //
    // They SELECT and nothing else. Slots are claimed and released in the bar below;
    // that split is what stops the two from ever disagreeing.
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
                // plan.groupsView, not plan.groups: the view is rebuilt whenever the state
                // is replaced, so a chip's label follows its group's slot edits. Binding to
                // `groups` and calling slotLabel() left the label frozen at creation time.
                model: plan ? plan.groupsView : []
                delegate: Rectangle {
                    required property var modelData
                    required property int index
                    // id so the Row's children can reach _sel/_c -- ancestor properties are
                    // not in QML scope, and a bare reference there is a runtime ReferenceError.
                    id: _tabChip
                    readonly property bool _sel: plan && plan.activeGroup === _tabChip.index
                    readonly property color _c: _tabChip.modelData.color
                    radius: Tokens.radiusSm
                    color: _tabChip._sel ? Qt.rgba(_tabChip._c.r, _tabChip._c.g, _tabChip._c.b, 0.14)
                                         : "transparent"
                    border.width: Tokens.cardBorderWidth
                    border.color: _tabChip._sel ? _tabChip._c : AppPalette.border
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
                            text: "G" + (_tabChip.index + 1)
                            color: _tabChip._sel ? AppPalette.textStrong : AppPalette.textMuted
                            font.pixelSize: Tokens.fontSm; font.bold: true
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        // A group with no slots is legal and must still say so, otherwise
                        // the chip reads as broken rather than as empty.
                        Text {
                            text: _tabChip.modelData.count
                                  ? _tabChip.modelData.label : qsTr("no slots")
                            color: AppPalette.textMuted
                            font.pixelSize: Tokens.fontXs
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                    KTapArea {
                        anchors.fill: parent
                        onTapped: if (plan) plan.activeGroup = _tabChip.index
                    }
                }
            }
            // Last chip, so adding a group is where the groups are. Eight slots is eight
            // groups at most — a ninth could own nothing and would exist only to be
            // deleted, so the chip goes away rather than sitting there doing nothing.
            Rectangle {
                id: _addChip
                visible: !!(plan && plan.canAddGroup)
                radius: Tokens.radiusSm
                color: "transparent"
                border.width: Tokens.cardBorderWidth
                border.color: AppPalette.border
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

    // THE bar: the only place slot ownership is CHANGED. The chips above select a group;
    // this says what every slot belongs to and is where slots move. There used to be a
    // third surface as well -- the selected group's own slot row -- and all three could
    // disagree. See Logic.slotClick for the three outcomes; the note underneath reports
    // which one just happened.
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
                readonly property bool _mine: covCell._owned && !!planGroup._g
                                              && covCell.modelData.groupId === planGroup._g.id
                readonly property color _c: covCell._owned && planGroup.plan
                    ? planGroup.plan.groupColors[covCell.modelData.index % planGroup.plan.groupColors.length]
                    : AppPalette.border
                width: covGrid.cellW
                implicitHeight: Math.round(38 * AppPalette.scale)
                radius: Tokens.radiusSm
                // Three states have to be told apart at a glance, because a click means
                // something different in each: filled = this group's (releases), tinted =
                // another group's (opens it), outline = free (claims it).
                color: covCell._mine
                       ? Qt.rgba(covCell._c.r, covCell._c.g, covCell._c.b, 0.28)
                       : (covCell._owned
                          ? Qt.rgba(covCell._c.r, covCell._c.g, covCell._c.b, 0.10)
                          : "transparent")
                border.width: covCell._mine
                              ? Math.max(1, Math.round(1.5 * AppPalette.scale))
                              : Math.max(1, Math.round(1 * AppPalette.scale))
                border.color: covCell._owned
                              ? (covCell._mine
                                 ? covCell._c
                                 : Qt.rgba(covCell._c.r, covCell._c.g, covCell._c.b, 0.5))
                              : AppPalette.border
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
                        color: covCell._mine ? covCell._c : AppPalette.textMuted
                        font.pixelSize: Math.round(9 * AppPalette.scale)
                        font.bold: covCell._mine
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                }
                KTapArea {
                    anchors.fill: parent
                    onTapped: {
                        if (!planGroup.plan) return
                        var cmd = covCell.modelData.cmd
                        planGroup._slotNote =
                            planGroup._noteFor(planGroup.plan.slotClick(cmd), cmd)
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

            // Which group is open, and the way out of it. Slots are claimed and released
            // in the bar above -- this pane never had a reason to own a second copy.
            Row {
                width: parent.width; height: Tokens.controlHSm; spacing: Tokens.spaceSm
                Rectangle {
                    width: Math.round(10 * AppPalette.scale); height: width
                    radius: width / 2
                    color: (planGroup._g && plan) ? plan.colorOf(planGroup._g)
                                                  : AppPalette.accent
                    anchors.verticalCenter: parent.verticalCenter
                }
                Text {
                    text: planGroup._g && plan
                          ? qsTr("Group %1 \u00b7 slots %2")
                            .arg(plan.groupIndexById(planGroup._g.id) + 1)
                            .arg(plan.slotLabel(planGroup._g))
                          : ""
                    color: AppPalette.textStrong
                    font.pixelSize: Tokens.fontSm; font.bold: true
                    width: Math.max(0, parent.width - Math.round(10 * AppPalette.scale)
                                    - Tokens.controlHSm - parent.spacing * 2)
                    elide: Text.ElideRight
                    anchors.verticalCenter: parent.verticalCenter
                }
                KCircleIconButton {
                    glyph: "\u00d7"
                    width: Tokens.controlHSm; height: Tokens.controlHSm
                    glyphPixelSize: Math.round(14 * AppPalette.scale)
                    borderWidth: Tokens.cardBorderWidth
                    anchors.verticalCenter: parent.verticalCenter
                    toolTipText: qsTr("Remove this group")
                    onClicked: if (plan && planGroup._g) plan.removeGroup(planGroup._g.id)
                }
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

    Rectangle { width: parent.width; height: 1; color: AppPalette.border }

    // ── apply ───────────────────────
    // Last, because it is the last thing you do. Role is not a mode: each button writes
    // its own half, and both are always live -- applying an empty half resets that side
    // of the device to defaults, which is a real and useful thing to want.
    Text {
        text: qsTr("Apply"); color: AppPalette.textMuted
        font.pixelSize: Tokens.fontSm; font.bold: true
    }

    Text {
        width: parent.width; wrapMode: Text.WordWrap
        color: AppPalette.textMuted; font.pixelSize: Tokens.fontXs
        text: qsTr("Each button writes all 8 slots of one half: configured slots get their "
                 + "configuration, the rest are reset to defaults.")
    }

    Row {
        id: applyRow
        width: parent.width
        spacing: Tokens.spaceSm

        Repeater {
            model: [
                { "role": "initiator",   "name": qsTr("Apply as initiator"),
                  "sub": qsTr("we ask · we read the answers") },
                { "role": "transponder", "name": qsTr("Apply as transponder"),
                  "sub": qsTr("we are asked · we answer") }
            ]
            delegate: Rectangle {
                id: applyCard
                required property var modelData
                readonly property var _i: planGroup.plan
                    ? planGroup.plan.applyInfo[applyCard.modelData.role] : null
                readonly property bool _live: !!(planGroup.dev && applyCard._i)
                width: (applyRow.width - applyRow.spacing) / 2
                radius: Tokens.radiusMd
                color: AppPalette.rowRaised
                border.width: applyCard._i && applyCard._i.stale
                              ? Math.max(1, Math.round(1.5 * AppPalette.scale))
                              : Tokens.cardBorderWidth
                border.color: applyCard._i && applyCard._i.stale
                              ? AppPalette.linkIdleBorder : AppPalette.border
                implicitHeight: _ac.implicitHeight + 2 * Tokens.spaceMd
                opacity: applyCard._live ? 1.0 : 0.5
                Column {
                    id: _ac
                    x: Tokens.spaceMd; y: Tokens.spaceMd
                    width: parent.width - 2 * Tokens.spaceMd
                    spacing: Tokens.spaceXxs
                    Text {
                        text: applyCard.modelData.name; color: AppPalette.textStrong
                        font.pixelSize: Tokens.fontSm; font.bold: true
                        width: parent.width; elide: Text.ElideRight
                    }
                    Text {
                        text: applyCard.modelData.sub; color: AppPalette.textMuted
                        font.pixelSize: Tokens.fontXs
                        width: parent.width; elide: Text.ElideRight
                    }
                    Text {
                        text: applyCard._i
                              ? qsTr("%1 frames · %2 configured")
                                .arg(applyCard._i.frames).arg(applyCard._i.configured)
                              : ""
                        color: AppPalette.textMuted; font.pixelSize: Tokens.fontXs
                        width: parent.width; elide: Text.ElideRight
                    }
                    Text {
                        visible: !!(applyCard._i
                                    && (applyCard._i.stale || applyCard._i.applied))
                        text: applyCard._i && applyCard._i.stale
                              ? qsTr("changed since applied")
                              : qsTr("applied")
                        color: applyCard._i && applyCard._i.stale
                               ? AppPalette.linkIdleText : AppPalette.textMuted
                        font.pixelSize: Tokens.fontXs
                        font.bold: !!(applyCard._i && applyCard._i.stale)
                    }
                }
                KTapArea {
                    anchors.fill: parent
                    active: applyCard._live
                    onTapped: planGroup._apply(applyCard.modelData.role)
                }
            }
        }
    }

}
