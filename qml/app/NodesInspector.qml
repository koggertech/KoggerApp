import QtQuick 2.15
import QtQuick.Controls 2.15
import kqml_types 1.0
import "NodesModel.js" as Model
import "NodesTiming.js" as Timing
import "NodesPayload.js" as Payload

Item {
    id: root

    property var store: null
    property var engine: null
    property var writer: null
    property bool live: false

    signal quickInterrogateRequested(var dev)
    signal payloadSendRequested(int stepId)

    readonly property var st: store ? store.st : null
    readonly property var ll: store ? store.selLifeline : null
    readonly property var ex: store ? store.selExchange : null
    readonly property var stp: store ? store.selStep : null
    readonly property var cs: {
        if (!store || !stp) return null
        var all = store.compiled.steps
        for (var i = 0; i < all.length; ++i) if (all[i].stepId === stp.id) return all[i]
        return null
    }

    component Group: DeviceSettingsGroup {
        width: parent ? parent.width : 0
        preferredWidth: width
        titlePixelSize: 13
        contentPadding: Tokens.spaceSm
        contentSpacing: Tokens.spaceSm
    }
    component Island: KIsland {
        width: parent ? parent.width : 0
        labelPixelSize: Tokens.fontBase
        rowPadding: Tokens.spaceMd
    }
    component Field: KIslandRow {
        default property alias fieldData: _fieldRow.data
        Row { id: _fieldRow; spacing: Tokens.spaceSm }
    }
    component TextRow: KIslandRow {
        id: textRow
        property alias value: _text.value
        property alias placeholder: _text.placeholder
        signal committed(string v)
        slotWidth: Math.round(width * 0.62)
        NodesTextField { id: _text; width: parent.width; onCommitted: function (v) { textRow.committed(v) } }
    }
    component TabsRow: KIslandRow {
        id: tabsRow
        property alias options: _tabs.options
        property alias currentValue: _tabs.currentValue
        signal valueSelected(var v)
        stacked: true
        KTabBar {
            id: _tabs
            width: parent.width
            trackColor: AppPalette.bgDeep
            fontPixelSize: Tokens.fontBase
            onValueSelected: function (v) { tabsRow.valueSelected(v) }
        }
    }
    component SwitchRow: KIslandRow {
        id: switchRow
        property bool checked: false
        signal toggled(bool on)
        interactive: true
        onClicked: _switch.click()
        KSwitch {
            id: _switch
            flat: true
            checked: switchRow.checked
            onToggled: {
                switchRow.toggled(checked)
                checked = Qt.binding(function () { return switchRow.checked })
            }
        }
    }
    component Actions: KIslandRow {
        default property alias actionsData: _actionsFlow.data
        stacked: true
        minHeight: 0
        verticalPadding: Tokens.spaceSm
        Flow { id: _actionsFlow; width: parent ? parent.width : 0; spacing: Tokens.spaceSm }
    }
    component Note: KIslandRow {
        property string text: ""
        property color color: AppPalette.textSecond
        caption: text
        captionColor: color
        minHeight: 0
        verticalPadding: Tokens.spaceSm
    }
    component ValueRow: KIslandRow {
        id: valueRow
        property string value: ""
        Text {
            text: valueRow.value
            color: AppPalette.textStrong
            font.pixelSize: Tokens.fontBase
        }
    }
    component Combo: KCombo {
        fontPixelSize: Tokens.fontBase
    }
    component Btn: UsblButton {
        implicitHeight: Tokens.controlHMd
        fontPixelSize: Tokens.fontSm
    }

    Flickable {
        id: flick
        anchors.fill: parent
        contentWidth: col.width + 2 * Tokens.spaceMd
        contentHeight: col.implicitHeight + 2 * Tokens.spaceMd
        clip: true
        boundsBehavior: Flickable.StopAtBounds

        Column {
            id: col
            x: Tokens.spaceMd; y: Tokens.spaceMd
            width: Math.max(flick.width - 2 * Tokens.spaceMd, Math.round(330 * AppPalette.scale))
            spacing: Tokens.spaceMd

            Island {
                visible: !!root.store && root.store.selKind !== "network"
                KIslandRow {
                    label: "‹ " + qsTr("Network, cycle, payload formats")
                    interactive: true
                    toolTipText: qsTr("Or click an empty part of the diagram")
                    onClicked: root.store.select("network", -1)
                }
            }

            // ── network ──────────────────────────────────────────────
            Column {
                visible: !!root.store && root.store.selKind === "network"
                width: parent.width
                spacing: Tokens.spaceMd

                Group {
                    title: qsTr("Network")
                    collapsedByDefault: false
                    Island {
                        TextRow {
                            label: qsTr("Name")
                            value: root.st ? root.st.name : ""
                            placeholder: qsTr("network name")
                            onCommitted: function (v) { root.store.setNetworkField("name", v) }
                        }
                        TextRow {
                            label: qsTr("Notes")
                            value: root.st ? root.st.notes : ""
                            onCommitted: function (v) { root.store.setNetworkField("notes", v) }
                        }
                        Field {
                            label: qsTr("Max range, m")
                            caption: root.st ? qsTr("a step needs at least %1 s").arg((Timing.requiredMs({ kind: "ranging", maxRangeM: root.st.maxRangeM }) / 1000).toFixed(2)) : ""
                            UsblSpin {
                                from: Model.MIN_RANGE_M; to: Model.MAX_RANGE_M; stepSize: 10
                                devValue: root.st ? root.st.maxRangeM : 300
                                implicitWidth: Math.round(110 * AppPalette.scale)
                                writeBack: function (v) { root.store.setNetworkField("maxRangeM", v) }
                            }
                        }
                        Actions {
                            Btn { text: qsTr("Fit to measured"); onClicked: root.store.fitMaxRangeToMeasured()
                                  toolTipText: qsTr("Max range = the farthest measured beacon × 1.2") }
                            Btn { text: qsTr("Use measured ranges"); onClicked: root.store.useMeasuredRanges()
                                  toolTipText: qsTr("Copy each beacon's last measured range into its planned range") }
                        }
                        TabsRow {
                            label: qsTr("Transmit")
                            caption: !!root.st && root.st.transmit === "pps"
                                     ? qsTr("Each step only arms the request; the head transmits on its next PPS pulse. Without a PPS signal nothing is sent.") : ""
                            options: [ { label: qsTr("Immediately"), value: "immediately" }, { label: qsTr("On PPS"), value: "pps" } ]
                            currentValue: root.st ? root.st.transmit : "immediately"
                            onValueSelected: function (v) { root.store.setNetworkField("transmit", v) }
                        }
                    }
                }

                Group {
                    title: qsTr("Cycle")
                    collapsedByDefault: false
                    Island {
                        footer: root.store ? qsTr("%1 steps · cycle %2 s").arg(root.store.compiled.steps.filter(function (s) { return s.enabled }).length)
                                             .arg((root.store.compiled.totalMs / 1000).toFixed(2)) : ""
                        TabsRow {
                            label: qsTr("Steps")
                            options: [ { label: qsTr("Auto"), value: "auto", tip: qsTr("each step as long as its own exchange needs") },
                                       { label: qsTr("Uniform"), value: "uniform", tip: qsTr("every step as long as the longest one") },
                                       { label: qsTr("Period"), value: "period", tip: qsTr("a fixed interval between steps, as Dwell was") } ]
                            currentValue: root.st ? root.st.cycle.stepMode : "auto"
                            onValueSelected: function (v) { root.store.setNetworkField("stepMode", v) }
                        }
                        Field {
                            visible: !!root.store && root.store.periodMode
                            label: qsTr("Period, s")
                            caption: root.store ? qsTr("between steps, at least %1 s").arg((root.store.periodFloorMs / 1000).toFixed(2)) : ""
                            KSpinBox {
                                from: root.store ? root.store.periodFloorMs : Model.DEFAULT_PERIOD_MS
                                to: Model.MAX_PERIOD_MS; stepSize: 100
                                divisor: 1000; decimals: 1
                                value: root.st ? Math.max(root.st.cycle.periodMs, from) : Model.DEFAULT_PERIOD_MS
                                implicitWidth: Math.round(110 * AppPalette.scale)
                                onValueModified: function (v) { root.store.setPeriodMs(v) }
                            }
                        }
                        Field {
                            visible: !root.store || !root.store.periodMode
                            label: qsTr("Extra margin, ms")
                            caption: qsTr("added to every step")
                            UsblSpin {
                                from: 0; to: 10000; stepSize: 50
                                devValue: root.st ? root.st.cycle.extraMarginMs : 0
                                implicitWidth: Math.round(110 * AppPalette.scale)
                                writeBack: function (v) { root.store.setNetworkField("extraMarginMs", v) }
                            }
                        }
                        SwitchRow {
                            label: qsTr("Repeat")
                            checked: !!root.st && root.st.cycle.repeat
                            onToggled: function (on) { root.store.setNetworkField("repeat", on) }
                        }
                    }
                }

                Group {
                    title: qsTr("Payload formats")
                    collapsedByDefault: true
                    Island {
                        footer: qsTr("A format packs named fields into bits, first field first, least significant bit first. Exchanges use it for request and reply data.")
                        Repeater {
                            model: root.store ? root.store.formats : []
                            delegate: KIslandRow {
                                id: fmtItem
                                required property var modelData
                                property bool open: false
                                stacked: true
                                minHeight: 0
                                verticalPadding: Tokens.spaceSm
                                Column {
                                    width: parent.width
                                    spacing: Tokens.spaceXs
                                    Row {
                                        spacing: Tokens.spaceSm
                                        NodesTextField {
                                            implicitWidth: Math.round(140 * AppPalette.scale)
                                            value: fmtItem.modelData.name
                                            onCommitted: function (v) {
                                                var f = Model.clone(fmtItem.modelData); f.name = v; root.store.setFormat(f.id, f)
                                            }
                                        }
                                        Text {
                                            text: qsTr("%1 bit").arg(Payload.formatBits(fmtItem.modelData))
                                            color: AppPalette.textSecond; font.pixelSize: Tokens.fontSm
                                            anchors.verticalCenter: parent.verticalCenter
                                        }
                                        Btn { text: fmtItem.open ? qsTr("Close") : qsTr("Fields"); onClicked: fmtItem.open = !fmtItem.open }
                                        Btn { text: "×"; danger: true; toolTipText: qsTr("Delete format"); onClicked: root.store.removeFormat(fmtItem.modelData.id) }
                                    }
                                    Repeater {
                                        model: fmtItem.open ? fmtItem.modelData.fields : []
                                        delegate: Row {
                                            required property var modelData
                                            required property int index
                                            spacing: Tokens.spaceXs
                                            function _set(key, v) {
                                                var f = Model.clone(fmtItem.modelData)
                                                f.fields[index][key] = v
                                                root.store.setFormat(f.id, f)
                                            }
                                            NodesTextField { implicitWidth: Math.round(80 * AppPalette.scale); value: modelData.name; onCommitted: function (v) { parent._set("name", v) } }
                                            KCombo {
                                                model: ["uint", "int", "bool"]
                                                currentIndex: Math.max(0, ["uint", "int", "bool"].indexOf(modelData.type))
                                                implicitWidth: Math.round(76 * AppPalette.scale); fontPixelSize: Tokens.fontSm
                                                onActivated: function (i) { parent._set("type", ["uint", "int", "bool"][i]) }
                                            }
                                            UsblSpin { from: 1; to: 64; devValue: modelData.bits; implicitWidth: Math.round(70 * AppPalette.scale); writeBack: function (v) { parent._set("bits", v) } }
                                            NodesTextField { implicitWidth: Math.round(52 * AppPalette.scale); value: modelData.scale ? String(modelData.scale) : ""; placeholder: qsTr("scale")
                                                             onCommitted: function (v) { parent._set("scale", Number(v) || 1) } }
                                            NodesTextField { implicitWidth: Math.round(44 * AppPalette.scale); value: modelData.unit || ""; placeholder: qsTr("unit")
                                                             onCommitted: function (v) { parent._set("unit", v) } }
                                            Btn {
                                                text: "×"; toolTipText: qsTr("Remove field")
                                                onClicked: { var f = Model.clone(fmtItem.modelData); f.fields.splice(index, 1); root.store.setFormat(f.id, f) }
                                            }
                                        }
                                    }
                                    Btn {
                                        visible: fmtItem.open
                                        text: qsTr("+ Field")
                                        onClicked: { var f = Model.clone(fmtItem.modelData); f.fields.push({ name: "f" + f.fields.length, bits: 8, type: "uint" }); root.store.setFormat(f.id, f) }
                                    }
                                }
                            }
                        }
                        Actions {
                            Btn { text: qsTr("+ Format"); onClicked: root.store.addFormat("") }
                        }
                    }
                }
            }

            // ── lifeline ─────────────────────────────────────────────
            Column {
                id: llCol
                visible: !!root.ll
                width: parent.width
                spacing: Tokens.spaceMd
                readonly property var _dev: root.ll && root.store ? root.store.deviceFor(root.ll.id) : null
                readonly property bool _ini: root.ll && root.st ? Model.isInitiator(root.st, root.ll.id) : false
                readonly property bool _ext: !!root.ll && root.ll.kind === "external"
                readonly property var _entry: root.ll && root.store && typeof root.ll.address === "number"
                                              ? root.store._sols[String(root.ll.address)] : null
                readonly property var _rx: root.ll && root.engine ? root.engine.rxLast[root.ll.id] || null : null
                readonly property string _ws: root.ll && root.store ? root.store.writeStateOf(root.ll.id) : ""
                readonly property string _fail: root.ll && root.store ? root.store.writeFailureText(root.ll.id) : ""

                Group {
                    title: qsTr("Lifeline")
                    collapsedByDefault: false
                    Island {
                        KIslandRow {
                            label: root.ll ? root.ll.label : ""
                            labelBold: true
                            leading: UsblAddressBadge {
                                visible: !!root.ll && typeof root.ll.address === "number"
                                address: root.ll && typeof root.ll.address === "number" ? root.ll.address : 0
                            }
                        }
                        TextRow {
                            label: qsTr("Label")
                            value: root.ll ? root.ll.label : ""
                            onCommitted: function (v) { root.store.setLifelineField(root.ll.id, "label", v) }
                        }
                        TabsRow {
                            label: qsTr("Kind")
                            options: [ { label: qsTr("Head"), value: "head" }, { label: qsTr("Beacon"), value: "beacon" },
                                       { label: qsTr("External"), value: "external", tip: qsTr("a party this app does not configure or bind") } ]
                            currentValue: root.ll ? root.ll.kind : "beacon"
                            onValueSelected: function (v) { root.store.setLifelineField(root.ll.id, "kind", v) }
                        }
                        Field {
                            label: qsTr("Address")
                            caption: qsTr("0 is an ordinary address")
                            Combo {
                                model: [qsTr("none")].concat(["0", "1", "2", "3", "4", "5", "6", "7"])
                                currentIndex: root.ll && typeof root.ll.address === "number" ? root.ll.address + 1 : 0
                                implicitWidth: Math.round(100 * AppPalette.scale)
                                onActivated: function (i) { root.store.setLifelineField(root.ll.id, "address", i === 0 ? null : i - 1) }
                            }
                        }
                        SwitchRow {
                            label: qsTr("In the cycle")
                            caption: qsTr("off removes every step to or from it")
                            checked: !!root.ll && root.ll.active !== false
                            onToggled: function (on) { root.store.setLifelineField(root.ll.id, "active", on) }
                        }
                        Field {
                            label: qsTr("Planned range, m")
                            caption: {
                                var m = root.ll && root.store ? root.store.measuredRangeOf(root.ll.id) : NaN
                                return isNaN(m) ? "" : qsTr("measured %1 m").arg(m.toFixed(1))
                            }
                            UsblSpin {
                                from: 0; to: 4000; stepSize: 10
                                devValue: root.ll && typeof root.ll.plannedRangeM === "number" ? Math.round(root.ll.plannedRangeM) : 0
                                implicitWidth: Math.round(110 * AppPalette.scale)
                                writeBack: function (v) { root.store.setLifelineField(root.ll.id, "plannedRangeM", v > 0 ? v : null) }
                            }
                        }
                        Note {
                            visible: !!llCol._entry && typeof llCol._entry.distance === "number"
                            text: llCol._entry ? qsTr("Last fix: %1 m · %2° · RSRQ %3 dB")
                                  .arg(isNaN(llCol._entry.distance) ? "—" : llCol._entry.distance.toFixed(1))
                                  .arg(isNaN(llCol._entry.azimuth) ? "—" : llCol._entry.azimuth.toFixed(1))
                                  .arg(isNaN(llCol._entry.snr) ? "—" : llCol._entry.snr.toFixed(0)) : ""
                        }
                        Note {
                            visible: !!llCol._rx
                            text: llCol._rx ? qsTr("Last data received: %1 · %2 ago").arg(root.store.rxFrameText(llCol._rx))
                                              .arg(DataFieldCatalog.formatAgeMs(Math.max(0, root.engine.nowMs - llCol._rx.atMs))) : ""
                            color: !llCol._rx ? AppPalette.textSecond
                                 : llCol._rx.verdict === "match" ? AppPalette.linkOkText
                                 : llCol._rx.verdict === "differs" ? AppPalette.dangerText : AppPalette.textSecond
                        }
                        Actions {
                            Btn {
                                text: "◀"; toolTipText: qsTr("Move left")
                                onClicked: root.store.moveLifeline(root.ll.id, Math.max(0, root.store.lifelines.indexOf(root.ll) - 1))
                            }
                            Btn {
                                text: "▶"; toolTipText: qsTr("Move right")
                                onClicked: {
                                    var i = -1
                                    for (var k = 0; k < root.store.lifelines.length; ++k) if (root.store.lifelines[k].id === root.ll.id) i = k
                                    root.store.moveLifeline(root.ll.id, i + 1)
                                }
                            }
                            Btn { text: qsTr("Delete lifeline"); danger: true; onClicked: root.store.removeLifeline(root.ll.id) }
                        }
                    }
                }

                Group {
                    title: qsTr("Answering and hearing")
                    collapsedByDefault: true
                    Island {
                        footer: !!root.ll && typeof root.ll.respond === "object"
                                ? qsTr("The window counts from the moment it is written, not from each request.") : ""
                        TabsRow {
                            label: qsTr("Answers")
                            options: [ { label: qsTr("Off"), value: "off" }, { label: qsTr("Always"), value: "always" }, { label: qsTr("Window"), value: "window" } ]
                            currentValue: !root.ll ? "off" : (typeof root.ll.respond === "string" ? root.ll.respond : "window")
                            onValueSelected: function (v) { root.store.setLifelineField(root.ll.id, "respond", v === "window" ? { windowS: 600 } : v) }
                        }
                        Field {
                            visible: !!root.ll && typeof root.ll.respond === "object"
                            label: qsTr("Window, s")
                            UsblSpin {
                                from: 1; to: 4294; stepSize: 10
                                devValue: root.ll && root.ll.respond && root.ll.respond.windowS ? root.ll.respond.windowS : 600
                                implicitWidth: Math.round(110 * AppPalette.scale)
                                writeBack: function (v) { root.store.setLifelineField(root.ll.id, "respond", { windowS: v }) }
                            }
                        }
                        TabsRow {
                            label: qsTr("Hears")
                            caption: {
                                var d = root.ll && root.store ? root.store.compiled.devices[root.ll.id] : null
                                return d ? (d.filter.length ? qsTr("addresses %1").arg(d.filter.join(", ")) : qsTr("nothing")) : ""
                            }
                            options: [ { label: qsTr("Auto"), value: "auto" }, { label: qsTr("Explicit"), value: "explicit" } ]
                            currentValue: root.ll && Array.isArray(root.ll.filter) ? "explicit" : "auto"
                            onValueSelected: function (v) {
                                if (v === "auto") root.store.setLifelineField(root.ll.id, "filter", "auto")
                                else root.store.setLifelineField(root.ll.id, "filter", root.store.compiled.devices[root.ll.id] ? root.store.compiled.devices[root.ll.id].filter : [])
                            }
                        }
                        KIslandRow {
                            visible: !!root.ll && Array.isArray(root.ll.filter)
                            label: qsTr("Addresses")
                            Row {
                                spacing: Tokens.spaceXxs
                                Repeater {
                                    model: 8
                                    delegate: KButton {
                                        id: addrChip
                                        required property int index
                                        readonly property bool _on: !!root.ll && Array.isArray(root.ll.filter) && root.ll.filter.indexOf(index) >= 0
                                        text: String(index)
                                        checkable: true
                                        checked: _on
                                        normalBg: AppPalette.controlRaised
                                        implicitWidth: Math.round(26 * AppPalette.scale)
                                        implicitHeight: Tokens.controlHSm
                                        horizontalPadding: 0
                                        fontPixelSize: Tokens.fontSm
                                        onClicked: {
                                            var f = root.ll.filter.slice()
                                            var i = f.indexOf(index)
                                            if (i >= 0) f.splice(i, 1); else f.push(index)
                                            root.store.setLifelineField(root.ll.id, "filter", f)
                                            checked = Qt.binding(function () { return addrChip._on })
                                        }
                                    }
                                }
                            }
                        }
                        Field {
                            label: qsTr("Blind after request, ms")
                            UsblSpin {
                                from: 0; to: 2000; stepSize: 10
                                devValue: root.ll ? root.ll.monitor.blindAfterRequestMs : 400
                                implicitWidth: Math.round(110 * AppPalette.scale)
                                writeBack: function (v) { root.store.setLifelineField(root.ll.id, "blindAfterRequestMs", v) }
                            }
                        }
                        Field {
                            label: qsTr("Blind after reply, ms")
                            UsblSpin {
                                from: 0; to: 2000; stepSize: 10
                                devValue: root.ll ? root.ll.monitor.blindAfterResponseMs : 400
                                implicitWidth: Math.round(110 * AppPalette.scale)
                                writeBack: function (v) { root.store.setLifelineField(root.ll.id, "blindAfterResponseMs", v) }
                            }
                        }
                        SwitchRow {
                            label: qsTr("Idle listening")
                            caption: qsTr("hear replies outside its own window")
                            checked: !!root.ll && root.ll.monitor.hearResponsesWhenIdle
                            onToggled: function (on) { root.store.setLifelineField(root.ll.id, "hearResponsesWhenIdle", on) }
                        }
                    }
                }

                Group {
                    visible: llCol._ini
                    title: qsTr("Interrogation")
                    collapsedByDefault: true
                    Island {
                        TabsRow {
                            label: qsTr("Keeps time")
                            options: [ { label: qsTr("App"), value: "host" }, { label: qsTr("PPS"), value: "pps" },
                                       { label: qsTr("Device"), value: "periodic", tip: qsTr("device-timed transmission, not available yet") } ]
                            currentValue: root.ll ? root.ll.executor.mode : "host"
                            onValueSelected: function (v) { root.store.setLifelineField(root.ll.id, "executorMode", v) }
                        }
                        Actions {
                            Btn {
                                readonly property bool _on: !!root.engine && root.engine.isRunningFor(root.ll ? root.ll.id : -1)
                                text: _on ? qsTr("■ Stop") : qsTr("▶ Run this head")
                                checked: _on
                                enabled: !!root.engine && root.engine.runnableFor(root.ll ? root.ll.id : -1).length > 0
                                toolTipText: enabled ? qsTr("Interrogate only this head's steps") : qsTr("Bind and connect this head first")
                                onClicked: _on ? root.engine.stop() : root.engine.start(root.ll.id)
                            }
                            Btn {
                                text: qsTr("Step")
                                enabled: !!root.engine && root.engine.runnableFor(root.ll ? root.ll.id : -1).length > 0
                                onClicked: root.engine.step(root.ll.id)
                            }
                        }
                        Field {
                            label: qsTr("Add exchange")
                            Combo {
                                readonly property var _others: root.store && root.ll ? root.store.lifelines.filter(function (x) { return x.id !== root.ll.id }) : []
                                model: [qsTr("to…")].concat(_others.map(function (x) { return x.label + (typeof x.address === "number" ? " · " + x.address : "") }))
                                currentIndex: 0
                                implicitWidth: Math.round(170 * AppPalette.scale)
                                onActivated: function (i) { if (i > 0) root.store.addExchange(root.ll.id, _others[i - 1].id, "ranging"); currentIndex = 0 }
                            }
                        }
                    }
                }

                Group {
                    visible: !llCol._ext
                    title: qsTr("Device")
                    collapsedByDefault: true
                    Island {
                        footer: !!root.ll && root.ll.kind === "beacon" && !!root.ll.binding && !llCol._dev
                                ? qsTr("Settings live in the device's RAM: it must stay powered until deployment. After a power cycle it does not answer and accepts only address 0.") : ""
                        Note {
                            text: {
                                if (!root.ll) return ""
                                var d = llCol._dev
                                var b = root.ll.binding
                                if (d) return qsTr("Bound to %1 SN %2 · fw %3 · %4").arg(d.devName).arg(d.devSN).arg(d.fwVersion).arg(root.store.linkLabelOf(d))
                                if (b) return qsTr("Bound to SN %1 — not connected now").arg(b.serial)
                                return qsTr("Not bound. Pick a connected device below, or bind later: the network can be designed without devices.")
                            }
                        }
                        Field {
                            label: qsTr("Bind to")
                            Combo {
                                readonly property var _cands: root.store ? root.store.devices.filter(function (d) {
                                    var o = root.store.lifelineForDevice(d); return !o || (root.ll && o.id === root.ll.id) }) : []
                                model: [qsTr("choose a device…")].concat(_cands.map(function (d) { return d.devName + " SN " + d.devSN + " · " + root.store.linkLabelOf(d) }))
                                currentIndex: 0
                                implicitWidth: Math.round(190 * AppPalette.scale)
                                onActivated: function (i) { if (i > 0) root.store.bindLifeline(root.ll.id, _cands[i - 1]); currentIndex = 0 }
                            }
                        }
                        Note {
                            visible: !!root.ll && !!root.ll.binding
                            text: {
                                var s = root.store ? root.store.writeStateText(llCol._ws) : ""
                                var d = root.ll && root.store ? root.store.compiled.devices[root.ll.id] : null
                                var h = root.ll && root.store ? root.store.historyFor(root.ll.id) : null
                                var when = h ? " · " + new Date(h.atMs).toLocaleString(Qt.locale(), Locale.ShortFormat) : ""
                                return qsTr("State: %1%2 · fingerprint %3").arg(s).arg(when).arg(d ? d.fingerprint.substring(0, 4) : "—")
                            }
                            color: llCol._ws === "confirmed" ? AppPalette.linkOkText
                                 : (llCol._ws === "rejected" || llCol._ws === "noAnswer" || llCol._ws === "partial") ? AppPalette.dangerText
                                 : AppPalette.textSecond
                        }
                        Note {
                            visible: llCol._fail.length > 0
                            text: llCol._fail
                            color: AppPalette.dangerText
                        }
                        Actions {
                            Btn {
                                text: qsTr("Write")
                                enabled: !!llCol._dev && !!root.writer && !root.writer.busy
                                toolTipText: qsTr("Send this lifeline's whole configuration to its device, frame by frame, with confirmation")
                                onClicked: root.writer.writeLifeline(root.ll.id)
                            }
                            Btn {
                                text: qsTr("Reset to baseline")
                                enabled: !!llCol._dev && !!root.writer && !root.writer.busy
                                toolTipText: qsTr("Silent, hears no address, every slot at defaults")
                                onClicked: root.writer.resetToBaseline(root.ll.id)
                            }
                            Btn {
                                visible: !!llCol._dev
                                text: qsTr("Quick interrogate…")
                                onClicked: root.quickInterrogateRequested(llCol._dev)
                            }
                            Btn {
                                visible: !!root.ll && !!root.ll.binding
                                text: qsTr("Unbind")
                                onClicked: root.store.unbindLifeline(root.ll.id)
                            }
                        }
                    }
                }

            }

            // ── exchange and step ────────────────────────────────────
            Column {
                id: exCol
                visible: !!root.ex
                width: parent.width
                spacing: Tokens.spaceMd
                readonly property bool _ping: !!root.ex && root.ex.kind === "ping"
                readonly property var _res: root.ex && root.store ? root.store.compiled.resolved[root.ex.id] : null
                readonly property var _legs: root.ex && root.engine ? root.engine.legsOf(root.ex.id) : []
                readonly property var _L: root.stp && root.engine ? root.engine.stepLog[root.stp.id] || null : null
                readonly property bool _fix: !!_L && typeof _L.distance === "number"
                readonly property bool _sync: !!_L && !!root.store && root.store.syncText(_L).length > 0

                Group {
                    title: root.store && root.ex ? root.store.exchangeLabel(root.ex) : ""
                    collapsedByDefault: false
                    Island {
                        TextRow {
                            label: qsTr("Label")
                            value: root.ex ? root.ex.label : ""
                            placeholder: qsTr("optional name")
                            onCommitted: function (v) { root.store.setExchangeField(root.ex.id, "label", v) }
                        }
                        Field {
                            label: qsTr("From")
                            Combo {
                                readonly property var _ls: root.store ? root.store.lifelines : []
                                model: _ls.map(function (x) { return x.label })
                                currentIndex: { for (var i = 0; i < _ls.length; ++i) if (root.ex && _ls[i].id === root.ex.from) return i; return 0 }
                                implicitWidth: Math.round(170 * AppPalette.scale)
                                onActivated: function (i) { root.store.setExchangeField(root.ex.id, "from", _ls[i].id) }
                            }
                        }
                        Field {
                            label: qsTr("To")
                            Combo {
                                readonly property var _ls: root.store ? root.store.lifelines : []
                                model: _ls.map(function (x) { return x.label + (typeof x.address === "number" ? " · " + x.address : "") })
                                       .concat(["0", "1", "2", "3", "4", "5", "6", "7"].map(function (a) { return qsTr("address %1 (no lifeline)").arg(a) }))
                                currentIndex: {
                                    if (!root.ex) return 0
                                    if (typeof root.ex.to.address === "number") return _ls.length + root.ex.to.address
                                    for (var i = 0; i < _ls.length; ++i) if (_ls[i].id === root.ex.to.lifeline) return i
                                    return 0
                                }
                                implicitWidth: Math.round(190 * AppPalette.scale)
                                onActivated: function (i) {
                                    if (i < _ls.length) root.store.setExchangeField(root.ex.id, "toLifeline", _ls[i].id)
                                    else root.store.setExchangeField(root.ex.id, "toAddress", i - _ls.length)
                                }
                            }
                        }
                        TabsRow {
                            label: qsTr("Kind")
                            options: [ { label: qsTr("Ranging"), value: "ranging", tip: qsTr("ask and wait for the reply: range + direction") },
                                       { label: qsTr("Ping"), value: "ping", tip: qsTr("send only; listeners hear it, nobody is ranged") } ]
                            currentValue: root.ex ? root.ex.kind : "ranging"
                            onValueSelected: function (v) { root.store.setExchangeField(root.ex.id, "kind", v) }
                        }
                        TabsRow {
                            visible: !exCol._ping
                            label: qsTr("Max range")
                            caption: {
                                if (!root.ex || !root.st) return ""
                                var r = Model.effectiveRangeM(root.st, root.ex)
                                var t = Model.targetLifeline(root.st, root.ex)
                                var p = t && typeof t.plannedRangeM === "number" ? qsTr(" · target planned %1 m").arg(Math.round(t.plannedRangeM)) : ""
                                return qsTr("Answer window %1 s after the reply hold%2").arg((Timing.travelMs(r) / 1000).toFixed(2)).arg(p)
                            }
                            options: [ { label: qsTr("Network · %1 m").arg(root.st ? root.st.maxRangeM : 0), value: "network" },
                                       { label: qsTr("Own"), value: "own" } ]
                            currentValue: root.ex && typeof root.ex.maxRange === "number" ? "own" : "network"
                            onValueSelected: function (v) { root.store.setExchangeField(root.ex.id, "maxRange", v === "own" ? root.st.maxRangeM : "network") }
                        }
                        Field {
                            visible: !exCol._ping && !!root.ex && typeof root.ex.maxRange === "number"
                            label: qsTr("Own max range, m")
                            UsblSpin {
                                from: Model.MIN_RANGE_M; to: Model.MAX_RANGE_M; stepSize: 10
                                devValue: root.ex && typeof root.ex.maxRange === "number" ? root.ex.maxRange : 300
                                implicitWidth: Math.round(110 * AppPalette.scale)
                                writeBack: function (v) { root.store.setExchangeField(root.ex.id, "maxRange", v) }
                            }
                        }
                        Field {
                            label: qsTr("Command id")
                            caption: qsTr("allocated so slots never clash")
                            Combo {
                                model: [qsTr("auto (%1)").arg(exCol._res ? exCol._res.cmd : "—")].concat(["0", "1", "2", "3", "4", "5", "6", "7"])
                                currentIndex: root.ex && typeof root.ex.cmd === "number" ? root.ex.cmd + 1 : 0
                                implicitWidth: Math.round(120 * AppPalette.scale)
                                onActivated: function (i) { root.store.setExchangeField(root.ex.id, "cmd", i === 0 ? "auto" : i - 1) }
                            }
                        }
                        Actions {
                            Btn {
                                visible: !root.stp && !!root.ex
                                text: qsTr("+ Step running it")
                                onClicked: root.store.addStep(root.ex.id)
                            }
                            Btn { text: qsTr("Delete exchange"); danger: true; onClicked: root.store.removeExchange(root.ex.id) }
                        }
                    }
                }

                Group {
                    title: qsTr("Data")
                    collapsedByDefault: true
                    Island {
                        footer: exCol._ping ? "" : qsTr("Reply data is fixed: the target answers with these bytes, which are written into its slot. The head decodes them with the same format.")
                        KIslandRow {
                            stacked: true
                            minHeight: 0
                            verticalPadding: Tokens.spaceSm
                            NodesPayloadEditor {
                                width: parent.width
                                store: root.store
                                title: qsTr("Request data")
                                spec: root.ex ? root.ex.request.data : ({ type: "none" })
                                maxBits: exCol._ping ? Payload.MAX_BITS : 162
                                onEdited: function (d) { root.store.setExchangeData(root.ex.id, "request", d) }
                            }
                        }
                        KIslandRow {
                            visible: !exCol._ping
                            stacked: true
                            minHeight: 0
                            verticalPadding: Tokens.spaceSm
                            NodesPayloadEditor {
                                width: parent.width
                                store: root.store
                                title: qsTr("Reply data")
                                spec: root.ex ? root.ex.response.data : ({ type: "none" })
                                onEdited: function (d) { root.store.setExchangeData(root.ex.id, "response", d) }
                            }
                        }
                        SwitchRow {
                            visible: !exCol._ping
                            label: qsTr("Follow-up")
                            caption: qsTr("after the reply the head sends again, and the target replies again")
                            checked: !!root.ex && !!root.ex.followUp
                            onToggled: function (on) { root.store.setFollowUp(root.ex.id, on) }
                        }
                        KIslandRow {
                            visible: !!root.ex && !!root.ex.followUp
                            stacked: true
                            minHeight: 0
                            verticalPadding: Tokens.spaceSm
                            NodesPayloadEditor {
                                width: parent.width
                                store: root.store
                                title: qsTr("Follow-up data")
                                allowPosition: true
                                maxBits: 162
                                spec: root.ex && root.ex.followUp ? root.ex.followUp.data : ({ type: "none" })
                                onEdited: function (d) { root.store.setExchangeData(root.ex.id, "followUp", d) }
                            }
                        }
                        KIslandRow {
                            visible: !!root.ex && !!root.ex.followUp
                            stacked: true
                            minHeight: 0
                            verticalPadding: Tokens.spaceSm
                            NodesPayloadEditor {
                                width: parent.width
                                store: root.store
                                title: qsTr("Second reply")
                                spec: root.ex && root.ex.followUp ? root.ex.followUp.reply.data : ({ type: "none" })
                                onEdited: function (d) { root.store.setExchangeData(root.ex.id, "followUpReply", d) }
                            }
                        }
                        Field {
                            visible: !!root.ex && !!root.ex.followUp
                            label: qsTr("Follow-up cmd")
                            Combo {
                                model: [qsTr("auto (%1)").arg(exCol._res && exCol._res.followCmd !== null ? exCol._res.followCmd : "—")].concat(["0", "1", "2", "3", "4", "5", "6", "7"])
                                currentIndex: root.ex && root.ex.followUp && typeof root.ex.followUp.cmd === "number" ? root.ex.followUp.cmd + 1 : 0
                                implicitWidth: Math.round(120 * AppPalette.scale)
                                onActivated: function (i) { root.store.setFollowUpCmd(root.ex.id, i === 0 ? "auto" : i - 1) }
                            }
                        }
                    }
                }

                Group {
                    visible: !exCol._ping
                    title: qsTr("Reply rewrite (advanced)")
                    collapsedByDefault: true
                    Island {
                        Field {
                            label: qsTr("Reply address")
                            Combo {
                                model: [qsTr("keep")].concat(["0", "1", "2", "3", "4", "5", "6", "7"])
                                currentIndex: root.ex && root.ex.rewrite && typeof root.ex.rewrite.address === "number" ? root.ex.rewrite.address + 1 : 0
                                implicitWidth: Math.round(100 * AppPalette.scale)
                                onActivated: function (i) {
                                    var rw = root.ex.rewrite ? Model.clone(root.ex.rewrite) : { address: null, cmd: null, asRequest: false }
                                    rw.address = i === 0 ? null : i - 1
                                    root.store.setRewrite(root.ex.id, rw)
                                }
                            }
                        }
                        Field {
                            label: qsTr("Reply cmd")
                            Combo {
                                model: [qsTr("keep")].concat(["0", "1", "2", "3", "4", "5", "6", "7"])
                                currentIndex: root.ex && root.ex.rewrite && typeof root.ex.rewrite.cmd === "number" ? root.ex.rewrite.cmd + 1 : 0
                                implicitWidth: Math.round(100 * AppPalette.scale)
                                onActivated: function (i) {
                                    var rw = root.ex.rewrite ? Model.clone(root.ex.rewrite) : { address: null, cmd: null, asRequest: false }
                                    rw.cmd = i === 0 ? null : i - 1
                                    root.store.setRewrite(root.ex.id, rw)
                                }
                            }
                        }
                        SwitchRow {
                            label: qsTr("Reply as request")
                            caption: qsTr("experimental: the head does not accept it")
                            captionColor: AppPalette.linkIdleText
                            checked: !!root.ex && !!root.ex.rewrite && root.ex.rewrite.asRequest
                            onToggled: function (on) {
                                var rw = root.ex.rewrite ? Model.clone(root.ex.rewrite) : { address: null, cmd: null, asRequest: false }
                                rw.asRequest = on
                                root.store.setRewrite(root.ex.id, rw)
                            }
                        }
                    }
                }

                Group {
                    title: qsTr("Listeners")
                    collapsedByDefault: true
                    Island {
                        footer: qsTr("Lifelines that hear this exchange without answering it: they get its address in their filter and must not respond.")
                        Repeater {
                            model: root.store && root.ex ? root.store.lifelines.filter(function (x) {
                                return x.id !== root.ex.from && !(root.ex.to && root.ex.to.lifeline === x.id) }) : []
                            delegate: KIslandRow {
                                id: lstRow
                                required property var modelData
                                readonly property var _l: {
                                    if (!root.ex) return null
                                    for (var i = 0; i < root.ex.listeners.length; ++i) if (root.ex.listeners[i].lifeline === modelData.id) return root.ex.listeners[i]
                                    return null
                                }
                                readonly property bool _hearsReply: !!_l && !!root.ex && root.ex.kind !== "ping"
                                label: modelData.label
                                caption: !_l ? qsTr("does not listen")
                                       : _hearsReply && _l.hearResponses ? qsTr("hears the request and the reply")
                                       : qsTr("hears the request")
                                Row {
                                    spacing: Tokens.spaceSm
                                    Text {
                                        visible: lstRow._hearsReply
                                        text: qsTr("reply")
                                        color: AppPalette.textSecond
                                        font.pixelSize: Tokens.fontSm
                                        anchors.verticalCenter: parent.verticalCenter
                                    }
                                    KSwitch {
                                        visible: lstRow._hearsReply
                                        flat: true
                                        toolTipText: qsTr("Also hears the reply")
                                        checked: !!lstRow._l && lstRow._l.hearResponses
                                        onToggled: {
                                            root.store.setListener(root.ex.id, lstRow.modelData.id, true, checked)
                                            checked = Qt.binding(function () { return !!lstRow._l && lstRow._l.hearResponses })
                                        }
                                    }
                                    KSwitch {
                                        flat: true
                                        toolTipText: qsTr("Listens to this exchange")
                                        checked: !!lstRow._l
                                        onToggled: {
                                            root.store.setListener(root.ex.id, lstRow.modelData.id, checked, false)
                                            checked = Qt.binding(function () { return !!lstRow._l })
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                Group {
                    visible: exCol._legs.length > 0
                    title: qsTr("Received data")
                    collapsedByDefault: true
                    Island {
                        footer: qsTr("What each receiving device decoded, compared with the data configured here. Only devices connected to this computer report it.")
                        Repeater {
                            model: exCol._legs
                            delegate: Note {
                                required property var modelData
                                readonly property var _e: root.ex && root.engine ? root.engine.rxFor(root.ex.id, modelData.leg, modelData.lifelineId) : null
                                readonly property bool _conn: !!root.store && !!root.store.deviceFor(modelData.lifelineId)
                                label: root.store ? qsTr("%1 at %2").arg(root.store.dataLegText(modelData.leg)).arg(root.store.lifelineLabel(modelData.lifelineId)) : ""
                                labelPixelSize: Tokens.fontSm
                                text: {
                                    if (!root.store) return ""
                                    if (_e) return qsTr("%1 · %2 ago").arg(root.store.rxEntryText(_e))
                                                   .arg(DataFieldCatalog.formatAgeMs(Math.max(0, root.engine.nowMs - _e.atMs)))
                                    if (!_conn) return qsTr("not connected, its decoded data cannot be seen")
                                    return qsTr("nothing received yet")
                                }
                                color: !_e ? AppPalette.textMuted : _e.verdict === "match" ? AppPalette.linkOkText : AppPalette.dangerText
                            }
                        }
                    }
                }

                Group {
                    visible: !!root.stp
                    title: qsTr("Step")
                    collapsedByDefault: false
                    Island {
                        SwitchRow {
                            label: qsTr("In the cycle")
                            checked: !!root.stp && root.stp.on
                            onToggled: function (on) { root.store.setStepField(root.stp.id, "on", on) }
                        }
                        TabsRow {
                            label: qsTr("Duration")
                            caption: {
                                if (!root.cs) return ""
                                var parts = Timing.breakdown(root.cs.spec).map(function (b) {
                                    var name = { travel: qsTr("round trip"), base: qsTr("hold + decode"), reqData: qsTr("request data"),
                                                 respData: qsTr("reply data"), followUp: qsTr("follow-up"), air: qsTr("on air"), tail: qsTr("tail") }[b.key]
                                    return name + " " + Math.round(b.ms)
                                })
                                return qsTr("Needs %1 ms: %2. Runs %3 ms.").arg(root.cs.requiredMs).arg(parts.join(" + ")).arg(root.cs.durationMs)
                            }
                            captionColor: root.cs && root.cs.durationMs < root.cs.requiredMs ? AppPalette.dangerText : AppPalette.textSecond
                            options: [ { label: qsTr("Auto"), value: "auto" }, { label: qsTr("Fixed"), value: "fixed" } ]
                            currentValue: root.stp && typeof root.stp.durationMs === "number" ? "fixed" : "auto"
                            onValueSelected: function (v) { root.store.setStepField(root.stp.id, "durationMs", v === "auto" ? "auto" : (root.cs ? root.cs.durationMs : 1000)) }
                        }
                        Field {
                            visible: !!root.stp && typeof root.stp.durationMs === "number"
                            label: qsTr("Fixed duration, ms")
                            UsblSpin {
                                from: 100; to: 600000; stepSize: 50
                                devValue: root.stp && typeof root.stp.durationMs === "number" ? root.stp.durationMs : 1000
                                implicitWidth: Math.round(110 * AppPalette.scale)
                                writeBack: function (v) { root.store.setStepField(root.stp.id, "durationMs", v) }
                            }
                        }
                        Note {
                            visible: !!exCol._L
                            text: exCol._L ? qsTr("Live: %1 · replies %2 of %3").arg(exCol._L.state).arg(exCol._L.replies || 0).arg(exCol._L.count || 0) : ""
                        }
                        ValueRow {
                            visible: exCol._fix
                            label: qsTr("Range, m")
                            value: exCol._fix ? root.store.fmtNum(exCol._L.distance, 1) : ""
                        }
                        ValueRow {
                            visible: exCol._fix
                            label: qsTr("Azimuth / elevation, °")
                            value: exCol._fix ? root.store.fmtNum(exCol._L.azimuth, 1) + " / " + root.store.fmtNum(exCol._L.elevation, 1) : ""
                        }
                        ValueRow {
                            visible: exCol._fix
                            label: qsTr("RSRQ, dB")
                            value: exCol._fix ? root.store.fmtNum(exCol._L.snr, 0) : ""
                        }
                        ValueRow {
                            visible: exCol._sync
                            label: qsTr("Sync peak / SNR, dB")
                            value: exCol._sync ? root.store.fmtAmpDb(exCol._L.sync.val) + " / " + root.store.fmtNum(exCol._L.sync.snr, 0) : ""
                        }
                        ValueRow {
                            visible: exCol._sync
                            label: qsTr("First arrival / SNR, dB")
                            value: exCol._sync ? root.store.fmtAmpDb(exCol._L.sync.firstVal) + " / " + root.store.fmtNum(exCol._L.sync.firstSnr, 0) : ""
                        }
                        ValueRow {
                            visible: exCol._sync
                            label: qsTr("Noise RMS, dB")
                            value: exCol._sync ? root.store.fmtAmpDb(exCol._L.sync.rms) : ""
                        }
                        ValueRow {
                            visible: exCol._sync
                            label: qsTr("First arrival → peak, ms")
                            value: exCol._sync ? root.store.fmtNum(exCol._L.sync.leadMs, 2) : ""
                        }
                        ValueRow {
                            visible: !!exCol._L && !!exCol._L.beacon
                            label: qsTr("Beacon position")
                            value: exCol._L ? root.store.beaconText(exCol._L) : ""
                        }
                        Actions {
                            Btn { text: qsTr("Send now"); enabled: !!root.engine; onClicked: root.engine.interrogateNow(root.stp.id) }
                            Btn { text: qsTr("Send with payload…"); enabled: !!root.engine; onClicked: root.payloadSendRequested(root.stp.id) }
                            Btn { text: qsTr("Duplicate"); onClicked: root.store.duplicateStep(root.stp.id) }
                            Btn { text: qsTr("Delete step"); danger: true; onClicked: root.store.removeStep(root.stp.id) }
                        }
                    }
                }

            }
        }
    }

    KScrollBar {
        flickable: flick
        anchors.top: flick.top
        anchors.bottom: flick.bottom
        anchors.right: flick.right
    }
    KScrollBar {
        flickable: flick
        orientation: Qt.Horizontal
        anchors.left: flick.left
        anchors.right: flick.right
        anchors.bottom: flick.bottom
    }
}
