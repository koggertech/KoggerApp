import QtQuick 2.15
import kqml_types 1.0
import "StandLogic.js" as Stand

// The stand panel: the only surface that configures and runs a calibration stand. There is no
// stand group in the device settings page, so everything the operator needs is here.
//
// TWO REGIMES, ONE PINNED HEAD. Compact is the head — name, device, what was last sent, the
// commands — plus the badge row. Extended adds the form beneath it. The head never moves, so a
// run can be commanded while the form is open.
//
// EDIT → COMMIT, unlike every other device control in this app. The stand accepts its whole
// configuration only as part of Start; there is no field-level write and a running scan cannot
// be re-shaped. So the form is a draft, the badge row reports what the device was actually
// given, and marks that differ are struck through until Start. That strike is the only place
// the panel says the two disagree — there is no dirty banner.
//
// NO PROGRESS, AND NOT BY OVERSIGHT. The stand acknowledges commands and publishes nothing
// else: no position, no point index, no completion. A counter derived from the configuration we
// sent would look like telemetry and be a guess.
//
// SCALE IS FIXED AT 1 and the content is laid out in AppPalette.scale space, not the widget
// system's appScale space. This is the first panel kind that takes input: a spin box scaled by
// a corner grip stops being usable long before a text card stops being readable, and matching
// the settings pane's control sizing is what keeps it usable.
BasePanePopup {
    id: root

    required property var store
    required property var def
    property var dev: null

    readonly property real widgetScale: 1.0

    readonly property real _bgAlpha: {
        var t = (def && typeof def.transparency === "number") ? def.transparency : 0
        return Math.max(0.15, Math.min(1, 1 - t / 100))
    }

    readonly property bool _hasStand: !!(dev && dev.isStandSupport === true)
    readonly property bool _expanded: !!(def && def.expanded)

    property var _cfg: Stand.normalizeConfig(def ? def.config : null)
    property var _sent: null
    property string _sentCmd: ""
    property string _sentTime: ""

    readonly property string _invalid: Stand.validate(_cfg)
    readonly property bool _canStart: _hasStand && _invalid === ""

    readonly property real _s: AppPalette.scale
    readonly property real _barH: headerHeight
    readonly property real _barPad: contentPadding
    readonly property real _barBtn: Math.max(Math.round(18 * AppPalette.scale), _barH - _barPad * 2)
    readonly property real _pad: Math.round(9 * _s)
    readonly property real _gap: Math.round(7 * _s)
    readonly property real _contentW: Math.round(292 * _s)
    readonly property real _contentH: contentCol.implicitHeight

    function _writeCfg(patch) {
        var next = Stand.copyConfig(_cfg)
        for (var k in patch) next[k] = patch[k]
        _cfg = Stand.normalizeConfig(next)
        if (store && def && def.id)
            store.setWidgetStandConfig(def.id, _cfg)
    }

    function _toggleExpanded() {
        if (store && def && def.id)
            store.setWidgetStandExpanded(def.id, !_expanded)
    }

    // The device API is reached by name and only if it is there. The panel ships ahead of the
    // command layer; without this it would throw on every press rather than doing nothing.
    function _call(name, arg) {
        if (!dev || typeof dev[name] !== "function")
            return false
        if (arg === undefined) dev[name]()
        else                   dev[name](arg)
        return true
    }

    function _stamp(cmd) {
        _sentCmd = cmd
        _sentTime = Qt.formatTime(new Date(), "hh:mm:ss")
    }

    // Always sends, including while a scan is running: this app cannot observe the run state,
    // so a guard here would be a guess wearing the shape of a safeguard. The stand decides what
    // a Start mid-scan means.
    function start() {
        if (!_canStart) return
        if (_call("standStart", _cfg)) {
            _sent = Stand.copyConfig(_cfg)
            _stamp("start")
        }
    }
    function stop()   { if (_call("standStop"))   _stamp("stop") }
    function pause()  { if (_call("standPause"))  _stamp("pause") }
    function resume() { if (_call("standResume")) _stamp("resume") }
    function home()   { if (_call("standHome"))   _stamp("home") }

    popupVisible: true
    dragEnabled: true
    dragAnywhere: false
    headerReserved: true
    dragHandleOpacity: 0
    resizeEnabled: false
    collapseButtonVisible: false
    fullscreenMode: false
    panelColor: Qt.rgba(AppPalette.bg.r, AppPalette.bg.g, AppPalette.bg.b, _bgAlpha)
    panelBorderColor: AppPalette.border
    panelRadius: Tokens.radiusLg
    ghostFollowsContent: true
    ghostRadius: Tokens.radiusLg
    snapEdgeCenters: true

    // Imperative, as in the other panel kinds: BasePanePopup writes into expandedWidth/Height
    // itself, so a binding here is broken the first time it does.
    function _applyScale() {
        expandedWidth  = Math.round(_contentW + _pad * 2 + contentPadding * 2)
        expandedHeight = Math.round(_contentH + _pad * 2 + _contentTopMargin + contentPadding)
    }

    property bool _synced: false

    function syncFromStore() {
        if (!def || !def.id)
            return
        _cfg = Stand.normalizeConfig(def.config)
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

    on_ContentHChanged: _applyScale()
    on_ContentWChanged: _applyScale()
    onDefChanged: { _cfg = Stand.normalizeConfig(def ? def.config : null); _applyScale(); Qt.callLater(syncFromStore) }

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

    component Island: Rectangle {
        default property alias body: islandCol.data
        property alias caption: islandLabel.text
        property alias captionSuffix: islandSuffix.text
        width: root._contentW
        implicitHeight: islandCol.implicitHeight + Math.round(14 * root._s)
        radius: Tokens.radiusLg
        color: Qt.rgba(AppPalette.card.r, AppPalette.card.g, AppPalette.card.b, root._bgAlpha)

        Column {
            id: islandCol
            x: Math.round(8 * root._s)
            y: Math.round(7 * root._s)
            width: parent.width - 2 * x
            spacing: Math.round(5 * root._s)

            Row {
                spacing: Math.round(5 * root._s)
                visible: islandLabel.text.length > 0
                Text {
                    id: islandLabel
                    font.pixelSize: Tokens.fontXxs
                    font.capitalization: Font.AllUppercase
                    font.letterSpacing: 0.6
                    color: AppPalette.textMuted
                }
                Text {
                    id: islandSuffix
                    font.pixelSize: Tokens.fontXxs
                    font.capitalization: Font.AllUppercase
                    font.letterSpacing: 0.6
                    color: AppPalette.accent
                }
            }
        }
    }

    // KSpinBox that pushes edits out and takes external updates without echoing them back —
    // the same guard UsblSpin uses, which is not visible outside that file.
    component ScanSpin: Item {
        id: spinRoot
        property string field: ""
        property int from: -2000000000
        property int to: 2000000000
        property int stepSize: 10
        property bool editable: true

        implicitHeight: Tokens.controlHMd
        opacity: editable ? 1 : 0.4

        readonly property int devValue: root._cfg ? root._cfg[field] : 0
        property bool _in: false

        onDevValueChanged: if (spin.value !== devValue) { _in = true; spin.value = devValue; _in = false }
        Component.onCompleted: { _in = true; spin.value = devValue; _in = false }

        KSpinBox {
            id: spin
            anchors.fill: parent
            enabled: spinRoot.editable
            from: spinRoot.from; to: spinRoot.to; stepSize: spinRoot.stepSize
            fontPixelSize: Tokens.fontSm
            onValueModified: function (v) {
                if (spinRoot._in) return
                var patch = {}
                patch[spinRoot.field] = v
                root._writeCfg(patch)
            }
        }
    }

    // The checked state is the configuration's, not the control's: a checkable KButton toggles
    // itself on click, which severs the binding that says what the scan actually holds.
    component MotionButton: KButton {
        property bool on: false
        height: Tokens.controlHLg
        fontPixelSize: Tokens.fontXs
        normalBg: on ? AppPalette.accentBg : AppPalette.bgDeep
        hoverBg: on ? AppPalette.accentBg : AppPalette.bgHover
        normalBorder: on ? AppPalette.accentBorder : AppPalette.border
        textColor: on ? AppPalette.textStrong : AppPalette.textSecond
    }

    component ShapeTriple: Row {
        id: triple
        property string prefix: ""
        width: root._contentW - Math.round(16 * root._s)
        spacing: Math.round(5 * root._s)

        Repeater {
            model: [{ suffix: "Start", label: qsTr("Start", "angle range") },
                    { suffix: "End",   label: qsTr("End",   "angle range") },
                    { suffix: "Step",  label: qsTr("Step",  "angle range") }]

            Column {
                required property var modelData
                width: (parent.width - 2 * parent.spacing) / 3
                spacing: Math.round(3 * root._s)

                Text {
                    text: modelData.label
                    font.pixelSize: Tokens.fontXxs
                    color: AppPalette.textMuted
                }
                ScanSpin {
                    width: parent.width
                    field: triple.prefix + modelData.suffix
                }
            }
        }
    }

    Item {
        id: titleBar
        width: parent.width
        height: root._barH
        y: -root._barH
        z: 10

        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.AllButtons
            cursorShape: Qt.OpenHandCursor
            onPressed: function(mouse) { mouse.accepted = true }
        }

        DragHandler {
            target: null
            enabled: root.dragEnabled && !root.collapsed && !root.fullscreenMode
            xAxis.enabled: true
            yAxis.enabled: true
            onActiveChanged: active ? root._beginDrag() : root._endDrag()
            onTranslationChanged: if (active) root._updateDrag(translation.x, translation.y)
        }

        Text {
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            text: qsTr("Stand")
            color: AppPalette.textStrong
            font.pixelSize: Tokens.fontBase
        }

        KDragBar {
            anchors.centerIn: parent
            orientation: "horizontal"
            barColor: AppPalette.controlRaised
        }

        KCircleIconButton {
            id: closeBtn
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.topMargin: root._barPad
            width: root._barBtn
            height: root._barBtn
            rounded: false
            cornerRadius: Tokens.radiusLg
            z: 1
            iconSource: ""
            glyph: "×"
            showGlyphWithIcon: true
            glyphPixelSize: Tokens.iconSm
            glyphColor: AppPalette.textSecond
            fillColor: AppPalette.controlRaised
            fillHoverColor: Qt.lighter(AppPalette.controlRaised, 1.2)
            fillPressedColor: AppPalette.bgDeep
            borderColor: AppPalette.border
            borderHoverColor: AppPalette.borderHover
            toolTipText: qsTr("Hide panel")
            onClicked: root.store.setStandPanelShown(false)
        }

        KCircleIconButton {
            anchors.right: closeBtn.left
            anchors.rightMargin: root._barPad
            anchors.top: closeBtn.top
            width: root._barBtn
            height: root._barBtn
            rounded: false
            cornerRadius: Tokens.radiusLg
            z: 1
            iconSource: "qrc:/icons/ui/pencil.svg"
            iconTintColor: AppPalette.textSecond
            fillColor: AppPalette.controlRaised
            fillHoverColor: Qt.lighter(AppPalette.controlRaised, 1.2)
            fillPressedColor: AppPalette.bgDeep
            borderColor: AppPalette.border
            borderHoverColor: AppPalette.borderHover
            toolTipText: qsTr("Panel settings")
            onClicked: root.store.openStandPanelSettings()
        }
    }

    Item {
        x: root.contentPadding + root._pad
        y: root.contentPadding + root._pad
        width: root._contentW
        height: root._contentH

        Column {
            id: contentCol
            width: parent.width
            spacing: root._gap

            // ── head ──────────────────────────────────────────────────────
            Row {
                width: parent.width
                spacing: Math.round(7 * root._s)

                Rectangle {
                    anchors.verticalCenter: parent.verticalCenter
                    visible: root._hasStand
                    height: Math.round(Tokens.chipH * 0.76)
                    width: devName.implicitWidth + Math.round(14 * root._s)
                    radius: height / 2
                    color: Qt.rgba(AppPalette.rowRaised.r, AppPalette.rowRaised.g,
                                   AppPalette.rowRaised.b, root._bgAlpha)
                    border.width: 1
                    border.color: AppPalette.border
                    // The serial number appears only when more than one stand answered: the app
                    // drives the first, and with two connected the operator has to be able to
                    // tell which one that is. With one there is nothing to disambiguate.
                    Text {
                        id: devName
                        anchors.centerIn: parent
                        text: {
                            if (!root.dev) return qsTr("device")
                            var n = root.dev.devName ? root.dev.devName : qsTr("device")
                            var many = !!(root.store && root.store.standDeviceCount > 1)
                            return (many && root.dev.devSN) ? (n + " · " + root.dev.devSN) : n
                        }
                        font.pixelSize: Tokens.fontXxs
                        color: AppPalette.textMuted
                    }
                }

                // What the stand was last told, and that it took it. Not a line of its own: it
                // belongs with the other two facts of identity, and a row costs height the
                // resting panel does not have to spend.
                Rectangle {
                    anchors.verticalCenter: parent.verticalCenter
                    visible: root._hasStand
                    height: Math.round(Tokens.chipH * 0.76)
                    width: sentText.implicitWidth + Math.round(16 * root._s)
                    radius: height / 2
                    readonly property bool _any: root._sentCmd.length > 0
                    color: _any ? Qt.rgba(AppPalette.linkOkBg.r, AppPalette.linkOkBg.g,
                                          AppPalette.linkOkBg.b, root._bgAlpha)
                                : Qt.rgba(AppPalette.rowRaised.r, AppPalette.rowRaised.g,
                                          AppPalette.rowRaised.b, root._bgAlpha)
                    border.width: 1
                    border.color: _any ? AppPalette.linkOkBorder : AppPalette.border
                    Text {
                        id: sentText
                        anchors.centerIn: parent
                        text: parent._any ? (root._glyph(root._sentCmd) + " " + root._sentTime)
                                          : qsTr("not sent")
                        font.pixelSize: Tokens.fontXxs
                        color: parent._any ? AppPalette.linkOkText : AppPalette.textMuted
                    }
                }

                KCircleIconButton {
                    anchors.verticalCenter: parent.verticalCenter
                    visible: root._hasStand
                    width: Math.round(Tokens.controlH * 0.9); height: width
                    glyph: root._expanded ? "▴" : "▾"
                    glyphPixelSize: Math.round(10 * root._s)
                    borderWidth: 1
                    toolTipText: root._expanded ? qsTr("Hide configuration") : qsTr("Configuration")
                    onClicked: root._toggleExpanded()
                }
            }

            // ── commands ──────────────────────────────────────────────────
            Row {
                width: parent.width
                spacing: Math.round(6 * root._s)
                visible: root._hasStand

                KButton {
                    width: parent.width - 4 * (Tokens.controlHLg + parent.spacing)
                    height: Tokens.controlHLg
                    text: qsTr("Start")
                    enabled: root._canStart
                    normalBg: AppPalette.accentBg
                    hoverBg: AppPalette.accentBg
                    normalBorder: AppPalette.accentBorder
                    onClicked: root.start()
                }
                KCircleIconButton {
                    width: Tokens.controlHLg; height: Tokens.controlHLg
                    glyph: "❚❚"; glyphPixelSize: Math.round(11 * root._s)
                    borderWidth: 1
                    toolTipText: qsTr("Pause — takes effect at the end of the motion already commanded")
                    onClicked: root.pause()
                }
                // Its own control rather than a Pause that turns into Resume. The panel does
                // not know whether the stand is running — a button that changed face would be
                // asserting a device state from the last command this app happened to send.
                KCircleIconButton {
                    width: Tokens.controlHLg; height: Tokens.controlHLg
                    glyph: "▶"; glyphPixelSize: Math.round(10 * root._s)
                    borderWidth: 1
                    toolTipText: qsTr("Resume")
                    onClicked: root.resume()
                }
                KCircleIconButton {
                    width: Tokens.controlHLg; height: Tokens.controlHLg
                    glyph: "■"; glyphPixelSize: Math.round(12 * root._s)
                    glyphColor: AppPalette.dangerText
                    borderWidth: 1
                    borderColor: AppPalette.dangerBorder
                    toolTipText: qsTr("Stop")
                    onClicked: root.stop()
                }
                KCircleIconButton {
                    width: Tokens.controlHLg; height: Tokens.controlHLg
                    glyph: "⌂"; glyphPixelSize: Math.round(13 * root._s)
                    borderWidth: 1
                    toolTipText: qsTr("Home")
                    onClicked: root.home()
                }
            }

            // ── the scan, as marks ────────────────────────────────────────
            Island {
                visible: root._hasStand
                StandBadgeRow {
                    width: parent.width
                    config: root._cfg
                    sentConfig: root._sent
                    transparencyAlpha: root._bgAlpha
                }
            }

            // ── empty state ───────────────────────────────────────────────
            Island {
                visible: !root._hasStand
                Text {
                    width: parent.width
                    wrapMode: Text.WordWrap
                    horizontalAlignment: Text.AlignHCenter
                    text: qsTr("No stand-capable device")
                    font.pixelSize: Tokens.fontSm
                    color: AppPalette.textSecond
                }
            }

            // ── the form ──────────────────────────────────────────────────
            Island {
                visible: root._hasStand && root._expanded
                caption: qsTr("Scan order")
                KTabBar {
                    width: parent.width
                    buttonHeight: Tokens.controlH
                    fontPixelSize: Tokens.fontXs
                    options: [{ label: qsTr("Az → El"), value: Stand.ORDER_AZ_EL },
                              { label: qsTr("El → Az"), value: Stand.ORDER_EL_AZ }]
                    property string orderModel: root._cfg ? root._cfg.order : Stand.ORDER_AZ_EL
                    property bool _g: false
                    onOrderModelChanged: if (currentValue !== orderModel) { _g = true; currentValue = orderModel; _g = false }
                    Component.onCompleted: { _g = true; currentValue = orderModel; _g = false }
                    onValueSelected: function(v) { if (!_g) root._writeCfg({ order: v }) }
                }
            }

            Island {
                visible: root._hasStand && root._expanded
                caption: qsTr("Inner")
                captionSuffix: "· " + root._axisName(Stand.innerAxis(root._cfg))
                ShapeTriple { prefix: "inner" }
            }

            Island {
                visible: root._hasStand && root._expanded
                caption: qsTr("Outer")
                captionSuffix: "· " + root._axisName(Stand.outerAxis(root._cfg))
                ShapeTriple { prefix: "outer" }
            }

            // Independent, not exclusive: both may be checked, and the pair is one property of
            // the scan rather than two settings four rows apart.
            Island {
                visible: root._hasStand && root._expanded
                caption: qsTr("Motion")
                Row {
                    width: parent.width
                    spacing: Math.round(6 * root._s)

                    MotionButton {
                        width: (parent.width - parent.spacing) / 2
                        text: qsTr("Reverse inner")
                        on: root._cfg ? root._cfg.reverse : false
                        toolTipText: qsTr("The inner axis sweeps forward then back for each outer step")
                        onClicked: root._writeCfg({ reverse: !root._cfg.reverse })
                    }
                    MotionButton {
                        width: (parent.width - parent.spacing) / 2
                        text: qsTr("Continuous inner")
                        on: root._cfg ? root._cfg.continuous : false
                        toolTipText: qsTr("The inner axis sweeps each leg without stopping, firing at every step boundary. "
                                        + "At most one fire per point; post-fire wait does not apply.")
                        onClicked: root._writeCfg({ continuous: !root._cfg.continuous })
                    }
                }
            }

            Island {
                visible: root._hasStand && root._expanded
                caption: qsTr("Firing")
                Row {
                    width: parent.width
                    spacing: Math.round(8 * root._s)
                    Column {
                        width: (parent.width - parent.spacing) / 2
                        spacing: Math.round(3 * root._s)
                        Text { text: qsTr("Fires"); font.pixelSize: Tokens.fontXxs; color: AppPalette.textMuted }
                        ScanSpin { width: parent.width; field: "fires"; from: 0; to: root._cfg && root._cfg.continuous ? 1 : 9999; stepSize: 1 }
                    }
                    Column {
                        width: (parent.width - parent.spacing) / 2
                        spacing: Math.round(3 * root._s)
                        Text { text: qsTr("Cycles"); font.pixelSize: Tokens.fontXxs; color: AppPalette.textMuted }
                        ScanSpin { width: parent.width; field: "cycles"; from: 1; to: 9999; stepSize: 1 }
                    }
                }
            }

            Island {
                visible: root._hasStand && root._expanded
                caption: qsTr("Timing")
                Row {
                    width: parent.width
                    spacing: Math.round(8 * root._s)
                    Column {
                        width: (parent.width - parent.spacing) / 2
                        spacing: Math.round(3 * root._s)
                        Text { text: qsTr("Settle, ms"); font.pixelSize: Tokens.fontXxs; color: AppPalette.textMuted }
                        ScanSpin { width: parent.width; field: "settleMs"; from: 0; to: 60000 }
                    }
                    Column {
                        width: (parent.width - parent.spacing) / 2
                        spacing: Math.round(3 * root._s)
                        Text {
                            text: qsTr("Post-fire, ms"); font.pixelSize: Tokens.fontXxs
                            color: AppPalette.textMuted
                            opacity: root._cfg && root._cfg.continuous ? 0.4 : 1
                        }
                        ScanSpin {
                            width: parent.width; field: "postFireMs"; from: 0; to: 60000
                            editable: !(root._cfg && root._cfg.continuous)
                        }
                    }
                }
            }
        }
    }

    function _glyph(cmd) {
        switch (cmd) {
        case "start":  return "▶"
        case "resume": return "▶"
        case "pause":  return "❚❚"
        case "stop":   return "■"
        case "home":   return "⌂"
        }
        return ""
    }

    function _axisName(a) { return a === "el" ? qsTr("elevation") : qsTr("azimuth") }
}
