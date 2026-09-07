import QtQuick 2.15
import kqml_types 1.0

// One inbound event trigger of a command group: on-request (ev 1, the transponder half)
// or on-reply (ev 2, the initiator half).
//
// A trigger is a slot whose defaults you replace by attaching payload sections. Receive
// and send are independent attachments, not modes, so every picker here offers only a
// payload FORMAT. Absence is never a menu item: an unattached section emits the carrying
// enum's zero value. Every combination goes out as one v6 USBLCmdConfig.
Rectangle {
    id: root

    property var plan: null
    property int groupId: -1
    property string which: "request"   // "request" (ev 1) | "response" (ev 2)

    // VESTIGIAL. The store now REPLACES its whole state on every edit (UsblPlanLogic.js),
    // so `plan.groups`, `plan.trigger()` and friends already change identity and bindings
    // re-evaluate on their own. The few `var _d = _rev` reads left below are harmless
    // no-ops kept to avoid churning working bindings -- do not copy the pattern.
    readonly property int _rev: plan ? plan.rev : 0

    // No copy needed: the store replaces its state on every edit, so plan.trigger() returns a
    // new object each time and every `_t.recv` / `_t.adv.*` binding re-evaluates. Writes go
    // through plan.setSectionField() by id, never through this.
    readonly property var _t: {
        var _d = plan ? plan.st : null
        return (plan && groupId >= 0) ? plan.trigger(groupId, which) : null
    }
    readonly property int _sections: {
        var _d = root._rev
        return plan ? plan.sectionCount(_t) : 0
    }
    readonly property int _ev: which === "request" ? 1 : 2
    // Inbound wiring accent. Not an AppPalette token yet — the palette has no hue distinct
    // from `accent` for a second semantic channel; promote it if this UI ships.
    readonly property color _violet: AppPalette.isDark ? "#A78BFA" : "#7C5CD3"

    radius: Tokens.radiusSm
    color: AppPalette.card
    border.width: Tokens.cardBorderWidth
    border.color: AppPalette.border
    implicitHeight: _col.implicitHeight + 2 * Tokens.spaceSm

    function _setSection(side, field, v) {
        if (plan) plan.setSectionField(groupId, which, side, field, v)
    }

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
            font.pixelSize: Tokens.fontXs
            width: Math.round(62 * AppPalette.scale)
            elide: Text.ElideRight
            anchors.verticalCenter: parent.verticalCenter
        }
        Row {
            id: _fr
            spacing: Tokens.spaceSm
            // No `height: parent.height` -- the enclosing Row derives its height from its
            // children, so that binding is a feedback loop. verticalCenter aligns it.
            anchors.verticalCenter: parent.verticalCenter
        }
    }

    Column {
        id: _col
        x: Tokens.spaceMd; y: Tokens.spaceSm
        width: parent.width - 2 * Tokens.spaceMd
        spacing: Tokens.spaceXs

        Row {
            width: parent.width; spacing: Tokens.spaceSm
            Text {
                // No "(ev 1)"/"(ev 2)": that is the wire's eventFilter value, the same kind
                // of implementation leak as the struct name. root._ev still carries it.
                text: root.which === "request"
                      ? qsTr("Handle request") : qsTr("Handle reply")
                color: AppPalette.textStrong
                font.pixelSize: Tokens.fontXs; font.bold: true
                anchors.verticalCenter: parent.verticalCenter
            }
            // Spacer, not a label: the wire struct is an implementation detail.
            Item {
                width: Math.max(0, parent.width - Math.round(130 * AppPalette.scale)
                                - Tokens.controlHSm - parent.spacing * 2)
                height: 1
            }
            KCircleIconButton {
                glyph: "×"
                width: Tokens.controlHSm; height: Tokens.controlHSm
                glyphPixelSize: Math.round(13 * AppPalette.scale)
                borderWidth: Tokens.cardBorderWidth
                anchors.verticalCenter: parent.verticalCenter
                toolTipText: qsTr("Remove handler")
                onClicked: if (root.plan) root.plan.detachTrigger(root.groupId, root.which)
            }
        }

        // A handler with nothing attached still answers — the device has no per-slot
        // "silent" or "off" any more. Say so rather than leave an empty card.
        Text {
            visible: root._sections === 0
            width: parent.width
            text: qsTr("Answers with no payload. Attach a section to carry data.")
            color: AppPalette.textMuted
            font.pixelSize: Tokens.fontXs
            wrapMode: Text.WordWrap
        }

        // ── attached payload sections ─────────────────────────────────────
        Repeater {
            model: [
                { "side": "recv", "title": qsTr("receive payload") },
                { "side": "send", "title": qsTr("send payload") }
            ]
            delegate: Rectangle {
                required property var modelData
                // id so the nested FieldRows can reach _s/_fmt: ancestor properties are not in
                // QML scope. These read fine on this Rectangle's own bindings and fail inside
                // its children, which is why nothing catches it at build time.
                id: _payCard
                readonly property var _s: root._t
                    ? (modelData.side === "recv" ? root._t.recv : root._t.send) : null
                readonly property var _fmt: {
                    var _d = root._rev
                    return (root.plan && _payCard._s)
                        ? root.plan.findBy(root.plan.formats, _payCard._s.fmt) : null
                }
                visible: !!_s
                width: parent.width
                radius: Tokens.radiusSm
                color: AppPalette.rowRaised
                border.width: Tokens.cardBorderWidth
                border.color: AppPalette.border
                implicitHeight: visible ? _sec.implicitHeight + 2 * Tokens.spaceXs : 0

                Rectangle {
                    width: Math.max(2, Math.round(2 * AppPalette.scale))
                    height: parent.height
                    color: root._violet
                }

                Column {
                    id: _sec
                    x: Tokens.spaceMd; y: Tokens.spaceXs
                    width: parent.width - 2 * Tokens.spaceMd
                    spacing: Tokens.spaceXs

                    Row {
                        width: parent.width; spacing: Tokens.spaceSm
                        Text {
                            text: modelData.title
                            color: root._violet
                            font.pixelSize: Tokens.fontXs; font.bold: true
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        Item {
                            width: Math.max(0, parent.width - Math.round(100 * AppPalette.scale)
                                            - Tokens.controlHSm - parent.spacing * 2)
                            height: 1
                        }
                        KCircleIconButton {
                            glyph: "×"
                            width: Tokens.controlHSm; height: Tokens.controlHSm
                            glyphPixelSize: Math.round(13 * AppPalette.scale)
                            borderWidth: Tokens.cardBorderWidth
                            anchors.verticalCenter: parent.verticalCenter
                            toolTipText: qsTr("Detach")
                            onClicked: if (root.plan)
                                root.plan.detachSection(root.groupId, root.which, modelData.side)
                        }
                    }

                    FieldRow {
                        label: qsTr("format")
                        KCombo {
                            model: root.plan ? root.plan.formats.map(function (f) { return f.label }) : []
                            currentIndex: {
                                if (!root.plan || !_payCard._s) return 0
                                for (var i = 0; i < root.plan.formats.length; ++i)
                                    if (root.plan.formats[i].id === _payCard._s.fmt) return i
                                return 0
                            }
                            implicitWidth: Math.round(170 * AppPalette.scale)
                            fontPixelSize: Tokens.fontXs
                            anchors.verticalCenter: parent.verticalCenter
                            onActivated: function (i) {
                                root._setSection(modelData.side, "fmt", root.plan.formats[i].id)
                            }
                        }
                    }

                    FieldRow {
                        label: qsTr("expect")
                        visible: modelData.side === "recv" && !!(_payCard._fmt && _payCard._fmt.sized)
                        UsblSpin {
                            from: 0; to: 4096; stepSize: 8
                            devValue: _payCard._s ? _payCard._s.bits : 0
                            implicitWidth: Math.round(96 * AppPalette.scale)
                            anchors.verticalCenter: parent.verticalCenter
                            writeBack: function (v) { root._setSection("recv", "bits", v) }
                        }
                        Text {
                            text: qsTr("bit"); color: AppPalette.textMuted
                            font.pixelSize: Tokens.fontXs
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }

                    FieldRow {
                        label: qsTr("bytes")
                        visible: modelData.side === "send" && !!(_payCard._fmt && _payCard._fmt.sized)
                        UsblHexField {
                            value: _payCard._s ? _payCard._s.payload : ""
                            placeholder: qsTr("hex bytes")
                            implicitWidth: Math.round(130 * AppPalette.scale)
                            anchors.verticalCenter: parent.verticalCenter
                            onCommitted: function (v) { root._setSection("send", "payload", v) }
                        }
                        Text {
                            text: {
                                var _d = root._rev
                                return qsTr("%1 bit").arg(root.plan && _payCard._s
                                    ? root.plan.payloadBytes(_payCard._s.payload) * 8 : 0)
                            }
                            color: AppPalette.textMuted; font.pixelSize: Tokens.fontXs
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                }
            }
        }

        // ── attach buttons ────────────────────────────────────────────────
        Row {
            width: parent.width
            spacing: Tokens.spaceSm
            visible: !!root._t && (!root._t.recv || !root._t.send)
            UsblButton {
                visible: !!root._t && !root._t.recv
                text: qsTr("+ Receive payload")
                implicitHeight: Tokens.controlHSm; fontPixelSize: Tokens.fontXs
                onClicked: if (root.plan) root.plan.attachSection(root.groupId, root.which, "recv")
            }
            UsblButton {
                visible: !!root._t && !root._t.send
                text: qsTr("+ Send payload")
                implicitHeight: Tokens.controlHSm; fontPixelSize: Tokens.fontXs
                onClicked: if (root.plan) root.plan.attachSection(root.groupId, root.which, "send")
            }
        }

        // ── per-trigger advanced: USBLCmdConfig rewrite rules ─────────────
        UsblButton {
            visible: !!root._t && !root._t.advOpen
            text: qsTr("+ advanced")
            implicitHeight: Tokens.controlHSm; fontPixelSize: Tokens.fontXs
            onClicked: if (root.plan) root.plan.setAdvOpen(root.groupId, root.which, true)
        }

        Column {
            visible: !!root._t && root._t.advOpen
            width: parent.width
            spacing: Tokens.spaceXs
            topPadding: Tokens.spaceXs

            Row {
                width: parent.width; spacing: Tokens.spaceSm
                Text {
                    text: qsTr("response rewrite")
                    color: AppPalette.textMuted
                    font.pixelSize: Tokens.fontXs; font.bold: true
                    anchors.verticalCenter: parent.verticalCenter
                }
                Item {
                    width: Math.max(0, parent.width - Math.round(110 * AppPalette.scale)
                                    - Tokens.controlHSm - parent.spacing * 2)
                    height: 1
                }
                KCircleIconButton {
                    glyph: "×"
                    width: Tokens.controlHSm; height: Tokens.controlHSm
                    glyphPixelSize: Math.round(13 * AppPalette.scale)
                    borderWidth: Tokens.cardBorderWidth
                    anchors.verticalCenter: parent.verticalCenter
                    toolTipText: qsTr("Remove advanced options")
                    onClicked: if (root.plan) root.plan.setAdvOpen(root.groupId, root.which, false)
                }
            }

            FieldRow {
                label: qsTr("direction")
                KCombo {
                    model: [qsTr("Swap request/response"), qsTr("Keep same")]
                    currentIndex: (root._t && root._t.adv.eventAction === "Same") ? 1 : 0
                    implicitWidth: Math.round(190 * AppPalette.scale)
                    fontPixelSize: Tokens.fontXs
                    anchors.verticalCenter: parent.verticalCenter
                    onActivated: function (i) {
                        if (root.plan) root.plan.setAdvField(root.groupId, root.which,
                                                            "eventAction", i === 1 ? "Same" : "Swap")
                    }
                }
            }
            FieldRow {
                label: qsTr("send cmd")
                KCombo {
                    model: [qsTr("Echo incoming"), qsTr("Replace")]
                    currentIndex: (root._t && root._t.adv.cmdIdAction === "Replacement") ? 1 : 0
                    implicitWidth: Math.round(150 * AppPalette.scale)
                    fontPixelSize: Tokens.fontXs
                    anchors.verticalCenter: parent.verticalCenter
                    onActivated: function (i) {
                        if (root.plan) root.plan.setAdvField(root.groupId, root.which, "cmdIdAction",
                                                            i === 1 ? "Replacement" : "Incoming")
                    }
                }
            }
            FieldRow {
                label: qsTr("cmd →")
                visible: !!(root._t && root._t.adv.cmdIdAction === "Replacement")
                UsblSpin {
                    from: 0; to: 255; stepSize: 1
                    devValue: root._t ? root._t.adv.cmdIdRepl : 0
                    implicitWidth: Math.round(84 * AppPalette.scale)
                    anchors.verticalCenter: parent.verticalCenter
                    writeBack: function (v) {
                        if (root.plan) root.plan.setAdvField(root.groupId, root.which, "cmdIdRepl", v)
                    }
                }
            }
            FieldRow {
                label: qsTr("send addr")
                KCombo {
                    model: [qsTr("Echo incoming"), qsTr("Replace")]
                    currentIndex: (root._t && root._t.adv.addrAction === "Replacement") ? 1 : 0
                    implicitWidth: Math.round(150 * AppPalette.scale)
                    fontPixelSize: Tokens.fontXs
                    anchors.verticalCenter: parent.verticalCenter
                    onActivated: function (i) {
                        if (root.plan) root.plan.setAdvField(root.groupId, root.which, "addrAction",
                                                            i === 1 ? "Replacement" : "Incoming")
                    }
                }
            }
            FieldRow {
                label: qsTr("addr →")
                visible: !!(root._t && root._t.adv.addrAction === "Replacement")
                UsblSpin {
                    from: 0; to: 255; stepSize: 1
                    devValue: root._t ? root._t.adv.addrRepl : 0
                    implicitWidth: Math.round(84 * AppPalette.scale)
                    anchors.verticalCenter: parent.verticalCenter
                    writeBack: function (v) {
                        if (root.plan) root.plan.setAdvField(root.groupId, root.which, "addrRepl", v)
                    }
                }
            }
        }
    }
}
