import QtQuick 2.15
import QtQml
import QtCore
import QtQuick.Controls 2.15
import QtQuick.Dialogs
import kqml_types 1.0
import "NodesModel.js" as Model
import "NodesPayload.js" as Payload

Item {
    id: root

    property var store: null
    property var engine: null
    property var writer: null
    property var workspaceStore: null
    property bool active: true
    property bool keysEnabled: true
    readonly property bool _keys: active && keysEnabled
                                  && !(typeof core !== "undefined" && core && core.missionEditorActive)

    property string mode: "design"
    property bool devicesShown: width > 900 * AppPalette.scale
    property bool inspectorShown: true
    property bool bottomShown: true

    readonly property bool _live: mode === "live"

    readonly property real _defDevicesW: Math.round(230 * AppPalette.scale)
    readonly property real _defInspectorW: Math.round(340 * AppPalette.scale)
    readonly property real _defBottomH: Math.round(168 * AppPalette.scale)
    readonly property real _minSideW: Math.round(170 * AppPalette.scale)
    readonly property real _minCanvasW: Math.round(240 * AppPalette.scale)
    readonly property real _minBottomH: Math.round(80 * AppPalette.scale)
    readonly property real _minPanesH: Math.round(160 * AppPalette.scale)
    readonly property real _splitHit: Math.round(8 * AppPalette.scale)

    Settings {
        id: layoutPrefs
        category: "main/nodesLayout"
        property real devicesW: 0
        property real inspectorW: 0
        property real bottomH: 0
    }

    readonly property real devicesW: {
        if (!devicesShown) return 0
        var want = layoutPrefs.devicesW > 0 ? layoutPrefs.devicesW : _defDevicesW
        var room = width - (inspectorShown ? inspectorW : 0) - _minCanvasW
        return Math.max(_minSideW, Math.min(want, room))
    }
    readonly property real inspectorW: {
        if (!inspectorShown) return 0
        var want = layoutPrefs.inspectorW > 0 ? layoutPrefs.inspectorW : _defInspectorW
        var room = width - (devicesShown ? _minSideW : 0) - _minCanvasW
        return Math.max(_minSideW, Math.min(want, room))
    }
    readonly property real bottomH: {
        if (!bottomShown) return 0
        var want = layoutPrefs.bottomH > 0 ? layoutPrefs.bottomH : _defBottomH
        var room = height - banners.y - banners.height - _minPanesH
        return Math.max(_minBottomH, Math.min(want, room))
    }
    function resetLayout() { layoutPrefs.devicesW = 0; layoutPrefs.inspectorW = 0; layoutPrefs.bottomH = 0 }

    component Splitter: Item {
        id: sp
        property bool horizontalLine: false
        property real _p0: 0
        signal dragStarted()
        signal dragged(real delta)
        signal reset()
        z: 20
        Rectangle {
            anchors.centerIn: parent
            width: sp.horizontalLine ? parent.width : Math.max(1, Math.round((spArea.containsMouse || spArea.pressed ? 3 : 1) * AppPalette.scale))
            height: sp.horizontalLine ? Math.max(1, Math.round((spArea.containsMouse || spArea.pressed ? 3 : 1) * AppPalette.scale)) : parent.height
            color: spArea.containsMouse || spArea.pressed ? AppPalette.accentBorder : AppPalette.border
        }
        MouseArea {
            id: spArea
            anchors.fill: parent
            hoverEnabled: true
            preventStealing: true
            cursorShape: sp.horizontalLine ? Qt.SplitVCursor : Qt.SplitHCursor
            onPressed: function (mouse) {
                var p = mapToItem(root, mouse.x, mouse.y)
                sp._p0 = sp.horizontalLine ? p.y : p.x
                sp.dragStarted()
            }
            onPositionChanged: function (mouse) {
                if (!pressed) return
                var p = mapToItem(root, mouse.x, mouse.y)
                sp.dragged((sp.horizontalLine ? p.y : p.x) - sp._p0)
            }
            onDoubleClicked: sp.reset()
        }
        KToolTip {
            text: qsTr("Drag to resize · double-click for the default size")
            shown: spArea.containsMouse && !spArea.pressed
        }
    }

    component Btn: UsblButton {
        implicitHeight: Tokens.controlHMd
        fontPixelSize: Tokens.fontSm
    }
    component Divider: Rectangle {
        width: 1
        height: Tokens.controlHMd
        color: AppPalette.border
    }

    function guardUnsaved(action) {
        if (store && store.dirty && store.lifelines.length) {
            unsavedPopup.pending = action
            unsavedPopup.open()
        } else {
            action()
        }
    }
    function _prepare(dlg, folder, name) {
        if (folder && folder.length) dlg.currentFolder = folder
        if (dlg.fileMode === FileDialog.SaveFile && name) dlg.selectedFile = (folder && folder.length ? folder + "/" : "") + name
    }
    function _safeName() {
        var n = store && store.st.name.length ? store.st.name : "network"
        return n.replace(/[^A-Za-z0-9_\-]+/g, "_")
    }
    function save(after) {
        if (!store) return
        if (store.filePath.length && !store.readOnly) {
            if (store.saveFile() && after) after()
        } else {
            saveDialog.after = after || null
            _prepare(saveDialog, store.saveFolder, _safeName())
            saveDialog.open()
        }
    }
    function openTemplate(kind) {
        templatePopup.kind = kind
        templatePopup.open()
    }
    function openQuick(dev) {
        quickPopup.dev = dev
        quickPopup.open()
    }
    function openPayloadSend(stepId) {
        payloadPopup.stepId = stepId
        var s = store ? Model.step(store.st, stepId) : null
        var ex = s ? Model.exchange(store.st, s.exchange) : null
        payloadPopup.payloadSpec = ex ? Model.clone(ex.request.data) : { type: "bytes", hex: "00" }
        payloadPopup.open()
    }

    Shortcut { sequences: [StandardKey.Undo]; enabled: root._keys; onActivated: root.store.undo() }
    Shortcut { sequences: [StandardKey.Redo]; enabled: root._keys; onActivated: root.store.redo() }
    Shortcut { sequences: [StandardKey.Save]; enabled: root._keys; onActivated: root.save() }
    Shortcut { sequence: "Ctrl+Shift+S"; enabled: root._keys; onActivated: { root._prepare(saveDialog, root.store.saveFolder, root._safeName()); saveDialog.open() } }
    Shortcut { sequences: [StandardKey.Open]; enabled: root._keys; onActivated: root.guardUnsaved(function () { root._prepare(openDialog, root.store.openFolder); openDialog.open() }) }
    Shortcut {
        sequences: [StandardKey.Delete]
        enabled: root._keys && !!root.store && root.store.selKind === "step"
        onActivated: root.store.removeStep(root.store.selId)
    }
    Shortcut {
        sequence: "Ctrl+D"
        enabled: root._keys && !!root.store && root.store.selKind === "step"
        onActivated: root.store.duplicateStep(root.store.selId)
    }

    // ── toolbar ──────────────────────────────────────────────────────────
    Flow {
        id: toolbar
        x: Tokens.spaceSm
        y: Tokens.spaceXs
        width: parent.width - 2 * Tokens.spaceSm
        spacing: Tokens.spaceSm

        Btn {
            id: fileBtn
            text: qsTr("File ▾")
            onClicked: {
                var p = fileBtn.mapToItem(null, 0, fileBtn.height + Tokens.spaceXxs)
                fileMenu.openAt(p.x, p.y)
            }
            NodesMenu {
                id: fileMenu
                function _template(text, kind) {
                    return { text: text, onTriggered: function () { root.openTemplate(kind) } }
                }
                groups: !visible || !root.store ? [] : [
                    [ { text: qsTr("New"),
                        items: function () {
                            return [ [ { text: qsTr("Empty network"),
                                         onTriggered: function () { root.guardUnsaved(function () { root.store.newNetwork("empty") }) } } ],
                                     [ fileMenu._template(qsTr("Ranging: head + beacons…"), "ranging"),
                                       fileMenu._template(qsTr("Telemetry from beacons…"), "telemetry"),
                                       fileMenu._template(qsTr("Command with acknowledge…"), "command"),
                                       fileMenu._template(qsTr("Position transfer…"), "position"),
                                       fileMenu._template(qsTr("Pinger and listeners…"), "pinger"),
                                       fileMenu._template(qsTr("Swarm: interrogators, one transponder…"), "swarm"),
                                       fileMenu._template(qsTr("Bench pair…"), "bench"),
                                       fileMenu._template(qsTr("Bench pair, delayed beacon…"), "benchDelayed") ],
                                     [ { text: qsTr("From the current command plan"),
                                         onTriggered: function () { root.guardUnsaved(function () { root.store.migrateFromPlan() }) } } ] ]
                        } },
                      { text: qsTr("Open…"),
                        onTriggered: function () { root.guardUnsaved(function () { root._prepare(openDialog, root.store.openFolder); openDialog.open() }) } },
                      { text: qsTr("Open recent"), enabled: root.store.recentFiles.length > 0,
                        items: function () {
                            return [ root.store.recentFiles.map(function (path) {
                                return { text: path, elideLeft: true,
                                         onTriggered: function () { root.guardUnsaved(function () { if (!root.store.openFile(path)) root.store.removeRecent(path) }) } }
                            }) ]
                        } } ],
                    [ { text: qsTr("Save"), onTriggered: function () { root.save() } },
                      { text: qsTr("Save as…"),
                        onTriggered: function () { root._prepare(saveDialog, root.store.saveFolder, root._safeName()); saveDialog.open() } },
                      { text: qsTr("Save a copy without bindings…"),
                        onTriggered: function () { root._prepare(copyDialog, root.store.saveFolder, root._safeName() + "_shared"); copyDialog.open() } },
                      { text: qsTr("Revert to saved"), enabled: root.store.filePath.length > 0,
                        onTriggered: function () { root.store.revert() } } ],
                    [ { text: qsTr("Export frames (JSON)…"),
                        onTriggered: function () { root._prepare(framesDialog, root.store.saveFolder, root._safeName() + "_frames"); framesDialog.open() } },
                      { text: qsTr("Export per-node files…"), onTriggered: function () { perNodeDialog.open() } } ]
                ]
            }
        }
        KCircleIconButton {
            glyph: "↶"; width: Tokens.controlHMd; height: Tokens.controlHMd
            glyphPixelSize: Math.round(14 * AppPalette.scale); borderWidth: Tokens.cardBorderWidth
            enabled: !!root.store && root.store.canUndo; toolTipText: qsTr("Undo (Ctrl+Z)")
            onClicked: root.store.undo()
        }
        KCircleIconButton {
            glyph: "↷"; width: Tokens.controlHMd; height: Tokens.controlHMd
            glyphPixelSize: Math.round(14 * AppPalette.scale); borderWidth: Tokens.cardBorderWidth
            enabled: !!root.store && root.store.canRedo; toolTipText: qsTr("Redo (Ctrl+Y)")
            onClicked: root.store.redo()
        }
        Divider {}
        KTabBar {
            buttonHeight: Tokens.controlHMd; verticalPadding: 0; horizontalPadding: 0; buttonSpacing: 0
            cornerRadius: Tokens.radiusMd; trackColor: AppPalette.controlRaised; fontPixelSize: Tokens.fontSm
            options: [ { label: qsTr("Design"), value: "design" }, { label: qsTr("Live"), value: "live", tip: qsTr("overlay what really happens on the diagram") } ]
            currentValue: root.mode
            onValueSelected: function (v) { root.mode = v }
        }
        Divider {}
        Btn {
            text: root.engine && root.engine.running ? qsTr("■ Stop") : qsTr("▶ Start")
            checked: !!root.engine && root.engine.running
            enabled: !!root.engine && (root.engine.running || root.engine.canRun)
            toolTipText: root.engine && root.engine.canRun ? qsTr("Run the whole cycle on the bound heads") : qsTr("No step can run: bind and connect a head first")
            onClicked: { if (root.engine.running) root.engine.stop(); else { root.engine.start(-1); root.mode = "live" } }
        }
        Btn {
            text: qsTr("Step")
            enabled: !!root.engine && root.engine.canRun
            onClicked: { root.engine.step(-1); root.mode = "live" }
        }
        Row {
            height: Tokens.controlHMd
            spacing: Tokens.spaceXs
            Text { text: qsTr("Max range"); color: AppPalette.textSecond; font.pixelSize: Tokens.fontSm; anchors.verticalCenter: parent.verticalCenter }
            UsblSpin {
                from: Model.MIN_RANGE_M; to: Model.MAX_RANGE_M; stepSize: 10
                devValue: root.store ? root.store.st.maxRangeM : 300
                implicitWidth: Math.round(90 * AppPalette.scale)
                fontPixelSize: Tokens.fontSm
                anchors.verticalCenter: parent.verticalCenter
                writeBack: function (v) { root.store.setNetworkField("maxRangeM", v) }
            }
            Text { text: qsTr("m"); color: AppPalette.textSecond; font.pixelSize: Tokens.fontSm; anchors.verticalCenter: parent.verticalCenter }
        }
        Row {
            height: Tokens.controlHMd
            spacing: Tokens.spaceXs
            Text { text: qsTr("Period"); color: AppPalette.textSecond; font.pixelSize: Tokens.fontSm; anchors.verticalCenter: parent.verticalCenter }
            KSpinBox {
                from: root.store ? root.store.periodFloorMs : Model.DEFAULT_PERIOD_MS
                to: Model.MAX_PERIOD_MS; stepSize: 100
                divisor: 1000; decimals: 1
                value: root.store ? Math.max(root.store.st.cycle.periodMs, from) : Model.DEFAULT_PERIOD_MS
                opacity: root.store && root.store.periodMode ? 1.0 : 0.55
                fontPixelSize: Tokens.fontSm
                implicitWidth: Math.round(90 * AppPalette.scale)
                anchors.verticalCenter: parent.verticalCenter
                toolTipText: qsTr("Interval between steps, as Dwell was in the device settings. At least %1 s, the longest step of the cycle. Setting it switches the cycle to this period.")
                             .arg(((root.store ? root.store.periodFloorMs : 0) / 1000).toFixed(2))
                onValueModified: function (v) { if (root.store) root.store.setPeriodMs(v) }
            }
            Text { text: qsTr("s"); color: AppPalette.textSecond; font.pixelSize: Tokens.fontSm; anchors.verticalCenter: parent.verticalCenter }
            Btn {
                text: qsTr("Auto")
                checked: !!root.store && !root.store.periodMode
                anchors.verticalCenter: parent.verticalCenter
                toolTipText: qsTr("Each step as long as its own exchange needs; the period is not used")
                onClicked: root.store.setNetworkField("stepMode", "auto")
            }
        }
        KTabBar {
            buttonHeight: Tokens.controlHMd; verticalPadding: 0; horizontalPadding: 0; buttonSpacing: 0
            cornerRadius: Tokens.radiusMd; trackColor: AppPalette.controlRaised; fontPixelSize: Tokens.fontSm
            options: [ { label: qsTr("Now"), value: "immediately", tip: qsTr("transmit on command") }, { label: qsTr("PPS"), value: "pps", tip: qsTr("transmit on the next PPS pulse") } ]
            currentValue: root.store ? root.store.st.transmit : "immediately"
            onValueSelected: function (v) { root.store.setNetworkField("transmit", v) }
        }
        Text {
            text: root.store ? qsTr("cycle %1 s").arg((root.store.compiled.totalMs / 1000).toFixed(2)) : ""
            color: AppPalette.textSecond; font.pixelSize: Tokens.fontSm
            height: Tokens.controlHMd; verticalAlignment: Text.AlignVCenter
        }
        Rectangle {
            readonly property var _c: root.store ? root.store.problemCounts : { error: 0, warning: 0 }
            visible: _c.error + _c.warning > 0
            height: Tokens.controlHMd
            width: _pc.implicitWidth + 2 * Tokens.spaceMd
            radius: height / 2
            color: _c.error ? AppPalette.dangerBg : AppPalette.linkIdleBg
            border.width: 1
            border.color: _c.error ? AppPalette.dangerBorder : AppPalette.linkIdleBorder
            Text {
                id: _pc
                anchors.centerIn: parent
                text: (parent._c.error ? "✕ " + parent._c.error : "") + (parent._c.error && parent._c.warning ? "  " : "") + (parent._c.warning ? "! " + parent._c.warning : "")
                color: parent._c.error ? AppPalette.dangerText : AppPalette.linkIdleText
                font.pixelSize: Tokens.fontSm; font.bold: true
            }
            KTapArea { onTapped: { root.bottomShown = true; bottom.tab = "problems" } }
        }
        Divider {}
        Btn {
            text: qsTr("Write changed")
            enabled: !!root.writer && !root.writer.busy
            toolTipText: qsTr("Write every connected device whose state differs from the network")
            onClicked: { root.writer.writeChanged(); root.bottomShown = true; bottom.tab = "write" }
        }
        Btn {
            text: qsTr("■ Silence all")
            danger: true
            toolTipText: qsTr("Stop the cycle and make every connected device stop answering")
            onClicked: { if (root.engine) root.engine.stop(); if (root.writer) root.writer.silenceAll() }
        }
        Divider {}
        KCircleIconButton {
            glyph: "▤"; width: Tokens.controlHMd; height: Tokens.controlHMd
            glyphPixelSize: Math.round(12 * AppPalette.scale); borderWidth: Tokens.cardBorderWidth
            highlightHold: root.devicesShown; toolTipText: qsTr("Devices panel")
            onClicked: root.devicesShown = !root.devicesShown
        }
        KCircleIconButton {
            glyph: "☰"; width: Tokens.controlHMd; height: Tokens.controlHMd
            glyphPixelSize: Math.round(12 * AppPalette.scale); borderWidth: Tokens.cardBorderWidth
            highlightHold: root.inspectorShown; toolTipText: qsTr("Inspector")
            onClicked: root.inspectorShown = !root.inspectorShown
        }
        KCircleIconButton {
            glyph: "▁"; width: Tokens.controlHMd; height: Tokens.controlHMd
            glyphPixelSize: Math.round(12 * AppPalette.scale); borderWidth: Tokens.cardBorderWidth
            highlightHold: root.bottomShown; toolTipText: qsTr("Problems and writing")
            onClicked: root.bottomShown = !root.bottomShown
        }
    }

    // ── banners ──────────────────────────────────────────────────────────
    Column {
        id: banners
        anchors.top: toolbar.bottom
        anchors.topMargin: Tokens.spaceXs
        x: Tokens.spaceSm
        width: parent.width - 2 * Tokens.spaceSm
        spacing: Tokens.spaceXxs

        StatusBanner {
            readonly property var _ids: root.store ? root.store.unwrittenLifelines : []
            visible: _ids.length > 0 && !!root.writer && !root.writer.busy
            width: parent.width
            sev: "warn"
            word: qsTr("Not written to the devices")
            sub: root.store ? qsTr("%1 still run their previous settings. Changes on the diagram — data, addresses, answers, slots — take effect only after a write.")
                                  .arg(_ids.map(function (l) { return l.label }).join(", ")) : ""
            aside: Component {
                Btn {
                    text: qsTr("Write changed")
                    onClicked: { root.writer.writeChanged(); root.bottomShown = true; bottom.tab = "write" }
                }
            }
        }

        Rectangle {
            visible: !!root.store && root.store.externalChange
            width: parent.width; height: Tokens.controlHMd
            radius: Tokens.radiusSm
            color: AppPalette.linkIdleBg
            Row {
                anchors.verticalCenter: parent.verticalCenter
                x: Tokens.spaceSm
                spacing: Tokens.spaceSm
                Text { text: qsTr("The file changed on disk."); color: AppPalette.linkIdleText; font.pixelSize: Tokens.fontXs; font.bold: true; anchors.verticalCenter: parent.verticalCenter }
                Btn { text: qsTr("Reload"); onClicked: root.store.revert() }
                Btn { text: qsTr("Keep mine"); onClicked: root.store.keepMine() }
            }
        }
        Rectangle {
            id: errBanner
            property string text: ""
            visible: text.length > 0
            width: parent.width; height: Math.max(Tokens.controlHMd, errText.implicitHeight + Tokens.spaceXs * 2)
            radius: Tokens.radiusSm
            color: AppPalette.dangerBg
            Text {
                id: errText
                anchors.left: parent.left; anchors.leftMargin: Tokens.spaceSm
                anchors.right: errClose.left; anchors.rightMargin: Tokens.spaceSm
                anchors.verticalCenter: parent.verticalCenter
                wrapMode: Text.WordWrap
                text: errBanner.text
                color: AppPalette.dangerText; font.pixelSize: Tokens.fontXs
            }
            Btn { id: errClose; anchors.right: parent.right; anchors.rightMargin: Tokens.spaceXs; anchors.verticalCenter: parent.verticalCenter; text: "×"; onClicked: errBanner.text = "" }
            Connections {
                target: root.store
                function onErrorRaised(text) { errBanner.text = text }
                function onNetworkReplaced() { errBanner.text = "" }
            }
        }
        Rectangle {
            visible: !!root.store && root.store.readOnly
            width: parent.width; height: Tokens.controlHMd
            radius: Tokens.radiusSm
            color: AppPalette.linkIdleBg
            Text {
                anchors.verticalCenter: parent.verticalCenter; x: Tokens.spaceSm
                text: qsTr("Open read-only: written by a newer version. Use Save as to keep a copy you can edit.")
                color: AppPalette.linkIdleText; font.pixelSize: Tokens.fontXs
            }
        }
        Rectangle {
            visible: !!root.store && root.store.lastNotes.length > 0
            width: parent.width; height: Tokens.controlHMd
            radius: Tokens.radiusSm
            color: AppPalette.card
            Row {
                anchors.verticalCenter: parent.verticalCenter; x: Tokens.spaceSm
                spacing: Tokens.spaceSm
                Text {
                    text: root.store ? qsTr("Loaded with notes: %1").arg(root.store.lastNotes.join(", ")) : ""
                    color: AppPalette.textSecond; font.pixelSize: Tokens.fontXs
                    anchors.verticalCenter: parent.verticalCenter
                }
                Btn { text: "×"; onClicked: root.store.lastNotes = [] }
            }
        }
    }

    // ── panes ────────────────────────────────────────────────────────────
    Item {
        id: panes
        anchors.top: banners.bottom
        anchors.topMargin: Tokens.spaceXs
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: bottomWrap.top

        NodesDevicesPanel {
            id: devicesPanel
            visible: root.devicesShown
            width: root.devicesW
            height: parent.height
            store: root.store
            writer: root.writer
            engine: root.engine
            workspaceStore: root.workspaceStore
            onQuickInterrogateRequested: function (dev) { root.openQuick(dev) }
        }
        Splitter {
            visible: root.devicesShown
            x: devicesPanel.width - width / 2
            width: root._splitHit
            height: parent.height
            property real _w0: 0
            onDragStarted: _w0 = root.devicesW
            onDragged: function (d) { layoutPrefs.devicesW = Math.max(root._minSideW, _w0 + d) }
            onReset: layoutPrefs.devicesW = 0
        }

        NodesSequenceCanvas {
            id: canvas
            x: devicesPanel.width + (root.devicesShown ? 1 : 0)
            width: parent.width - x - (root.inspectorShown ? inspector.width + 1 : 0)
            height: parent.height
            store: root.store
            engine: root.engine
            writer: root.writer
            live: root._live
            msPerPx: root.store ? root.store.st.view.msPerPx : 4
            onSendNowRequested: function (id) { if (root.engine) { root.engine.interrogateNow(id); root.mode = "live" } }
            onPayloadSendRequested: function (id) { root.openPayloadSend(id) }
            onQuickInterrogateRequested: function (dev) { root.openQuick(dev) }
        }

        Splitter {
            visible: root.inspectorShown
            x: inspector.x - width / 2
            width: root._splitHit
            height: parent.height
            property real _w0: 0
            onDragStarted: _w0 = root.inspectorW
            onDragged: function (d) { layoutPrefs.inspectorW = Math.max(root._minSideW, _w0 - d) }
            onReset: layoutPrefs.inspectorW = 0
        }
        NodesInspector {
            id: inspector
            visible: root.inspectorShown
            x: parent.width - width
            width: root.inspectorW
            height: parent.height
            store: root.store
            engine: root.engine
            writer: root.writer
            live: root._live
            onQuickInterrogateRequested: function (dev) { root.openQuick(dev) }
            onPayloadSendRequested: function (id) { root.openPayloadSend(id) }
        }
    }

    Item {
        id: bottomWrap
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: root.bottomH
        clip: true
        NodesBottomPanel {
            id: bottom
            anchors.fill: parent
            anchors.topMargin: 1
            store: root.store
            writer: root.writer
        }
    }

    Splitter {
        horizontalLine: true
        visible: root.bottomShown
        x: 0
        y: bottomWrap.y - height / 2
        width: root.width
        height: root._splitHit
        property real _h0: 0
        onDragStarted: _h0 = root.bottomH
        onDragged: function (d) { layoutPrefs.bottomH = Math.max(root._minBottomH, _h0 - d) }
        onReset: layoutPrefs.bottomH = 0
    }

    // ── dialogs ──────────────────────────────────────────────────────────
    FileDialog {
        id: openDialog
        title: qsTr("Open network")
        fileMode: FileDialog.OpenFile
        nameFilters: [qsTr("Nodes networks (*.knodes)"), qsTr("All files (*)")]
        onAccepted: root.store.openFile(String(selectedFile))
    }
    FileDialog {
        id: saveDialog
        property var after: null
        title: qsTr("Save network as")
        fileMode: FileDialog.SaveFile
        defaultSuffix: "knodes"
        nameFilters: [qsTr("Nodes networks (*.knodes)")]
        onAccepted: { if (root.store.saveFileAs(String(selectedFile)) && after) after(); after = null }
        onRejected: after = null
    }
    FileDialog {
        id: copyDialog
        title: qsTr("Save a copy without device bindings")
        fileMode: FileDialog.SaveFile
        defaultSuffix: "knodes"
        nameFilters: [qsTr("Nodes networks (*.knodes)")]
        onAccepted: root.store.saveCopyWithoutBindings(String(selectedFile))
    }
    FileDialog {
        id: framesDialog
        title: qsTr("Export frames")
        fileMode: FileDialog.SaveFile
        defaultSuffix: "json"
        nameFilters: [qsTr("JSON (*.json)")]
        onAccepted: root.store.exportFrames(String(selectedFile))
    }
    FolderDialog {
        id: perNodeDialog
        title: qsTr("Folder for per-node files")
        onAccepted: root.store.exportPerNode(String(selectedFolder))
    }

    component ModalCard: Popup {
        parent: Overlay.overlay
        anchors.centerIn: parent
        modal: true
        focus: true
        padding: Tokens.spaceLg
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        background: Rectangle {
            radius: Tokens.radiusLg
            color: AppPalette.bg
            border.width: 1
            border.color: AppPalette.border
        }
    }

    ModalCard {
        id: unsavedPopup
        property var pending: null
        contentItem: Column {
            spacing: Tokens.spaceMd
            width: Math.round(380 * AppPalette.scale)
            Text {
                width: parent.width; wrapMode: Text.WordWrap
                text: qsTr("%1 has unsaved changes.").arg(root.store ? root.store.displayName : "")
                color: AppPalette.textStrong; font.pixelSize: Tokens.fontMd; font.bold: true
            }
            Row {
                spacing: Tokens.spaceSm
                Btn { text: qsTr("Save"); onClicked: { var a = unsavedPopup.pending; unsavedPopup.close(); root.save(a) } }
                Btn { text: qsTr("Discard"); danger: true; onClicked: { var a = unsavedPopup.pending; unsavedPopup.close(); if (a) a() } }
                Btn { text: qsTr("Cancel"); onClicked: unsavedPopup.close() }
            }
        }
    }

    component AddressChips: Row {
        id: chips
        property var selected: [1, 2]
        spacing: Tokens.spaceXxs
        Repeater {
            model: 8
            delegate: Rectangle {
                required property int index
                readonly property bool _on: chips.selected.indexOf(index) >= 0
                width: Math.round(26 * AppPalette.scale); height: width
                radius: Tokens.radiusSm
                color: _on ? DataFieldCatalog.usblAddressColor(index) : AppPalette.card
                border.width: 1
                border.color: _on ? AppPalette.accentBorder : AppPalette.border
                Text { anchors.centerIn: parent; text: String(index); color: parent._on ? "#FFFFFF" : AppPalette.textStrong; font.pixelSize: Tokens.fontXs; font.bold: true }
                KTapArea {
                    onTapped: {
                        var s = chips.selected.slice(), i = s.indexOf(index)
                        if (i >= 0) s.splice(i, 1); else s.push(index)
                        chips.selected = s.sort(function (a, b) { return a - b })
                    }
                }
            }
        }
    }

    ModalCard {
        id: templatePopup
        property string kind: "ranging"
        contentItem: Column {
            spacing: Tokens.spaceSm
            width: Math.round(400 * AppPalette.scale)
            Text { text: qsTr("New network"); color: AppPalette.textStrong; font.pixelSize: Tokens.fontMd; font.bold: true }
            Text { text: templatePopup.kind === "swarm" ? qsTr("Interrogator addresses") : qsTr("Beacon addresses"); color: AppPalette.textMuted; font.pixelSize: Tokens.fontXs }
            AddressChips { id: tplAddrs; selected: [1, 2] }
            Row {
                spacing: Tokens.spaceSm
                visible: templatePopup.kind !== "bench"
                Text { text: templatePopup.kind === "benchDelayed" ? qsTr("Extra range of the delayed beacon") : qsTr("Max range"); color: AppPalette.textMuted; font.pixelSize: Tokens.fontXs; anchors.verticalCenter: parent.verticalCenter }
                UsblSpin { id: tplRange; from: 1; to: 2000; stepSize: 10; devValue: 300; implicitWidth: Math.round(90 * AppPalette.scale); writeBack: function (v) { devValue = v } }
                Text { text: qsTr("m"); color: AppPalette.textMuted; font.pixelSize: Tokens.fontXs; anchors.verticalCenter: parent.verticalCenter }
            }
            Row {
                spacing: Tokens.spaceSm
                Btn {
                    text: qsTr("Create")
                    enabled: tplAddrs.selected.length > 0
                    onClicked: {
                        var k = templatePopup.kind
                        var p = { addresses: tplAddrs.selected }
                        if (k === "benchDelayed") p.extraRangeM = tplRange.devValue
                        else p.maxRangeM = tplRange.devValue
                        templatePopup.close()
                        root.guardUnsaved(function () { root.store.newNetwork(k, p) })
                    }
                }
                Btn { text: qsTr("Cancel"); onClicked: templatePopup.close() }
            }
        }
    }

    ModalCard {
        id: quickPopup
        property var dev: null
        contentItem: Column {
            spacing: Tokens.spaceSm
            width: Math.round(400 * AppPalette.scale)
            Text {
                text: quickPopup.dev ? qsTr("Interrogate from %1 SN %2").arg(quickPopup.dev.devName).arg(quickPopup.dev.devSN) : ""
                color: AppPalette.textStrong; font.pixelSize: Tokens.fontMd; font.bold: true
            }
            Text {
                width: parent.width; wrapMode: Text.WordWrap
                text: qsTr("Plain ranging needs no write: each request carries the address, the command and Max range. The device becomes a head lifeline of the current network (or of a new quick network).")
                color: AppPalette.textMuted; font.pixelSize: Tokens.fontXs
            }
            Text { text: qsTr("Addresses"); color: AppPalette.textMuted; font.pixelSize: Tokens.fontXs }
            AddressChips { id: quickAddrs; selected: [1] }
            Row {
                spacing: Tokens.spaceSm
                Text { text: qsTr("Max range"); color: AppPalette.textMuted; font.pixelSize: Tokens.fontXs; anchors.verticalCenter: parent.verticalCenter }
                UsblSpin { id: quickRange; from: 1; to: 2000; stepSize: 10; devValue: root.store ? root.store.st.maxRangeM : 300; implicitWidth: Math.round(90 * AppPalette.scale); writeBack: function (v) { devValue = v } }
                Text { text: qsTr("m"); color: AppPalette.textMuted; font.pixelSize: Tokens.fontXs; anchors.verticalCenter: parent.verticalCenter }
            }
            Row {
                spacing: Tokens.spaceSm
                Btn {
                    text: qsTr("▶ Start")
                    enabled: !!quickPopup.dev && quickAddrs.selected.length > 0
                    onClicked: {
                        var head = root.store.quickInterrogate(quickPopup.dev, quickAddrs.selected, quickRange.devValue)
                        quickPopup.close()
                        if (head >= 0 && root.engine) { root.engine.start(head); root.mode = "live" }
                    }
                }
                Btn { text: qsTr("Cancel"); onClicked: quickPopup.close() }
            }
        }
    }

    ModalCard {
        id: payloadPopup
        property int stepId: -1
        property var payloadSpec: ({ type: "bytes", hex: "00" })
        contentItem: Column {
            spacing: Tokens.spaceSm
            width: Math.round(440 * AppPalette.scale)
            Text { text: qsTr("Send once with this request data"); color: AppPalette.textStrong; font.pixelSize: Tokens.fontMd; font.bold: true }
            NodesPayloadEditor {
                width: parent.width
                store: root.store
                title: qsTr("Data")
                spec: payloadPopup.payloadSpec
                maxBits: 162
                onEdited: function (d) { payloadPopup.payloadSpec = d }
            }
            Row {
                spacing: Tokens.spaceSm
                Btn {
                    text: qsTr("Send")
                    onClicked: {
                        var enc = Payload.encode(payloadPopup.payloadSpec, root.store.formats)
                        if (root.engine) root.engine.sendWithPayload(payloadPopup.stepId, enc.hex, enc.bits)
                        root.mode = "live"
                        payloadPopup.close()
                    }
                }
                Btn { text: qsTr("Cancel"); onClicked: payloadPopup.close() }
            }
        }
    }
}
