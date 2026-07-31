import QtQuick 2.15
import kqml_types 1.0

// One inbound event trigger of a command group: on-request (ev 1, the transponder half)
// or on-reply (ev 2, the initiator half).
//
// A trigger is a slot whose defaults you replace by attaching payload sections. Receive
// and send are independent attachments, not modes, so every picker here offers only a
// payload FORMAT. Absence is never a menu item: an unattached section emits the carrying
// enum's zero value, and the number of attached sections decides which struct is used —
// one section fits USBLCmdSlotConfig, two need USBLCmdConfig.
Rectangle {
    id: root

    property var plan: null
    property int groupId: -1
    property string which: "request"   // "request" (ev 1) | "response" (ev 2)

    readonly property var _t: (plan && groupId >= 0) ? plan.trigger(groupId, which) : null
    readonly property int _sections: plan ? plan.sectionCount(_t) : 0
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
            height: parent.height
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
                text: root.which === "request"
                      ? qsTr("Handle request  (ev 1)") : qsTr("Handle reply  (ev 2)")
                color: AppPalette.textStrong
                font.pixelSize: Tokens.fontXs; font.bold: true
                anchors.verticalCenter: parent.verticalCenter
            }
            Text {
                text: root.plan ? root.plan.structOf(root._t) : ""
                color: root._violet
                font.pixelSize: Tokens.fontXs
                width: Math.max(0, parent.width - Math.round(130 * AppPalette.scale)
                                - Tokens.controlHSm - parent.spacing * 2)
                horizontalAlignment: Text.AlignRight
                elide: Text.ElideRight
                anchors.verticalCenter: parent.verticalCenter
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

        // Disposition is only expressible while nothing is attached — USBLCmdSlotConfig
        // has one `function` field, so "stay silent" and "carry a bit array" are exclusive.
        KTabBar {
            visible: root._sections === 0
            width: parent.width
            buttonHeight: Math.round(24 * AppPalette.scale)
            fontPixelSize: Tokens.fontXs
            options: root.plan ? root.plan.dispositions.map(function (d) {
                return { "value": d.id, "text": d.label }
            }) : []
            currentValue: root._t ? root._t.disposition : "ack"
            onValueSelected: function (v) {
                if (root.plan) root.plan.setDisposition(root.groupId, root.which, v)
            }
        }

        // ── attached payload sections ─────────────────────────────────────
        Repeater {
            model: [
                { "side": "recv", "title": qsTr("receive payload") },
                { "side": "send", "title": qsTr("send payload") }
            ]
            delegate: Rectangle {
                required property var modelData
                readonly property var _s: root._t
                    ? (modelData.side === "recv" ? root._t.recv : root._t.send) : null
                readonly property var _fmt: (root.plan && _s)
                    ? root.plan.findBy(root.plan.formats, _s.fmt) : null
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
                                if (!root.plan || !_s) return 0
                                for (var i = 0; i < root.plan.formats.length; ++i)
                                    if (root.plan.formats[i].id === _s.fmt) return i
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
                        visible: modelData.side === "recv" && !!(_fmt && _fmt.sized)
                        UsblSpin {
                            from: 0; to: 4096; stepSize: 8
                            devValue: _s ? _s.bits : 0
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
                        visible: modelData.side === "send" && !!(_fmt && _fmt.sized)
                        UsblHexField {
                            value: _s ? _s.payload : ""
                            placeholder: qsTr("hex bytes")
                            implicitWidth: Math.round(130 * AppPalette.scale)
                            anchors.verticalCenter: parent.verticalCenter
                            onCommitted: function (v) { root._setSection("send", "payload", v) }
                        }
                        Text {
                            text: qsTr("%1 bit").arg(root.plan && _s
                                ? root.plan.payloadBytes(_s.payload) * 8 : 0)
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
                label: qsTr("role")
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
