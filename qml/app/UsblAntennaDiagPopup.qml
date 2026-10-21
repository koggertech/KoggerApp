import QtQuick 2.15
import QtQuick.Controls 2.15
import kqml_types 1.0

Popup {
    id: root

    property var store: null
    property int address: -1
    property var history: []
    readonly property int historyLength: 120

    readonly property var _diags: (visible && typeof dataset !== "undefined" && dataset && dataset.usblDiagnostics)
                                  ? dataset.usblDiagnostics : ({})
    readonly property var diag: address >= 0 ? (_diags[String(address)] || null) : null
    readonly property var _channels: (diag && diag.channels) ? diag.channels : []
    readonly property string _code: DataFieldCatalog.usblAntennaCode(diag, store)
    readonly property var _thr: (store && store.usblDiagThresholds) ? store.usblDiagThresholds
                                                                    : DataFieldCatalog.usblDiagDefaults()

    readonly property real _s: AppPalette.scale
    readonly property int _titleFont: Tokens.fontLg
    readonly property int _font: Tokens.fontLg
    readonly property int _sectionFont: Tokens.fontBase
    readonly property int _fontSmall: Tokens.fontSm
    readonly property real _rowH: Math.round(_font * 1.7)
    readonly property real _sketchSize: Math.round(160 * _s)
    readonly property var _cols: [
        { key: "index",      title: qsTr("Ch"),          w: 52 },
        { key: "used",       title: qsTr("Dir"),         w: 58 },
        { key: "levelDb",    title: qsTr("Level, dB"),   w: 100 },
        { key: "inDb",       title: qsTr("Input, dB"),   w: 100 },
        { key: "snrDb",      title: qsTr("SNR, dB"),     w: 90 },
        { key: "phaseDeg",   title: qsTr("Phase, °"),    w: 96 },
        { key: "residDeg",   title: qsTr("Resid., °"),   w: 96 },
        { key: "peakOffset", title: qsTr("Offset, smp"), w: 116 }
    ]
    readonly property real _tableW: {
        var w = 0
        for (var i = 0; i < _cols.length; ++i)
            w += Math.round(_cols[i].w * _s)
        return w
    }
    readonly property real _contentW: Math.max(_tableW, metricsRow.implicitWidth)
    readonly property real _maxBodyW: Math.max(Math.round(320 * _s),
                                               (parent ? parent.width : 1200) - 2 * padding - 2 * Tokens.spaceLg)
    readonly property real _maxBodyH: Math.max(Math.round(240 * _s),
                                               (parent ? parent.height : 900) - 2 * padding - 2 * Tokens.spaceLg)

    function openFor(addr) {
        address = addr
        refreshHistory()
        open()
    }

    function refreshHistory() {
        history = (address >= 0 && typeof dataset !== "undefined" && dataset)
                  ? dataset.usblDiagHistory(address, historyLength) : []
    }

    function _fmt(v, digits, suffix) {
        if (v === undefined || v === null || typeof v !== "number" || !isFinite(v))
            return "—"
        return v.toFixed(digits) + (suffix ? suffix : "")
    }

    function _signed(v, digits, suffix) {
        var s = _fmt(v, digits, suffix)
        return (s !== "—" && v > 0) ? "+" + s : s
    }

    function _ink(sev) {
        return DataFieldCatalog.usblSeverityInk(sev)
    }

    function _codeInk(code) {
        return DataFieldCatalog.usblAntennaInk(code)
    }

    function _geometryText(g) {
        switch (g) {
        case "crossA": return qsTr("cross A")
        case "crossB": return qsTr("cross B")
        case "tetra4": return qsTr("tetrahedron")
        }
        return qsTr("unknown layout")
    }

    function _stateText(s) {
        switch (s) {
        case "valid":        return qsTr("direction valid")
        case "solverFailed": return qsTr("solver failed")
        case "channelsOnly": return qsTr("channels only")
        }
        return qsTr("no analysis")
    }

    function _facts() {
        if (!diag)
            return qsTr("No diagnostics from this node yet")
        var parts = [_geometryText(diag.geometry), _stateText(diag.state),
                     qsTr("ping %1").arg(diag.pingCounter)]
        if (store && store.nowMs && diag.epochMs > 0)
            parts.push(DataFieldCatalog.formatAgeMs(Math.max(0, store.nowMs - diag.epochMs)))
        if (!diag.joined)
            parts.push(qsTr("no matching solution"))
        return parts.join(" · ")
    }

    function _cell(ch, key) {
        switch (key) {
        case "index":      return String(ch.index)
        case "used":       return ch.used ? qsTr("yes") : qsTr("no")
        case "levelDb":    return _fmt(ch.levelDb, 1)
        case "inDb":       return _fmt(ch.inDb, 1)
        case "snrDb":      return _fmt(ch.snrDb, 0)
        case "phaseDeg":   return _fmt(ch.phaseDeg, 1)
        case "residDeg":   return _fmt(ch.residDeg, 1)
        case "peakOffset": return _fmt(ch.peakOffset, 3)
        }
        return ""
    }

    function _cellInk(ch, key) {
        if (key === "levelDb" || key === "inDb")
            return _ink(DataFieldCatalog.usblLevelSeverity(ch[key], store))
        if (key === "used" && !ch.used)
            return AppPalette.textMuted
        return AppPalette.text
    }

    function _series(key) {
        var out = []
        var h = history || []
        for (var i = 0; i < h.length; ++i)
            out.push(h[i][key])
        return out
    }

    readonly property bool _absolute: !!(store && store.usblDiagLevelAbsolute)
    readonly property string _levelKey: _absolute ? "peaksDb" : "levels"

    function _levelSeries(ch) {
        var out = []
        var h = history || []
        for (var i = 0; i < h.length; ++i) {
            var l = h[i][_levelKey]
            out.push((l && ch < l.length) ? l[ch] : NaN)
        }
        return out
    }

    readonly property int _levelChannels: {
        var n = _channels.length
        var h = history || []
        for (var i = 0; i < h.length; ++i)
            if (h[i].levels && h[i].levels.length > n)
                n = h[i].levels.length
        return n
    }

    readonly property real _levelDead: _thr.levelDb - DataFieldCatalog.usblDeadBelowDb()

    readonly property var _levelRange: {
        var lo = _absolute ? Infinity : _levelDead - 2
        var hi = _absolute ? -Infinity : 0
        var h = history || []
        for (var i = 0; i < h.length; ++i) {
            var l = h[i][_levelKey] || []
            for (var c = 0; c < l.length; ++c) {
                if (typeof l[c] === "number" && isFinite(l[c])) {
                    lo = Math.min(lo, l[c])
                    hi = Math.max(hi, l[c])
                }
            }
        }
        if (!isFinite(lo) || !isFinite(hi))
            return { lo: NaN, hi: NaN }
        return { lo: Math.floor(lo - 1), hi: Math.ceil(hi + 1) }
    }

    parent: Overlay.overlay
    x: Math.round(((parent ? parent.width : width) - width) / 2)
    y: Math.round(((parent ? parent.height : height) - height) / 2)
    modal: false
    focus: true
    closePolicy: Popup.CloseOnEscape
    padding: Tokens.spaceMd

    onAddressChanged: if (visible) refreshHistory()

    Timer {
        id: historyTimer
        interval: 250
        repeat: false
        onTriggered: root.refreshHistory()
    }

    Connections {
        target: (typeof dataset !== "undefined") ? dataset : null
        enabled: root.visible
        function onUsblDiagChanged() {
            if (!historyTimer.running)
                historyTimer.start()
        }
    }

    Connections {
        target: root.store
        enabled: !!root.store
        function onUsblAntennaDiagActiveChanged() {
            if (!root.store.usblAntennaDiagActive)
                root.close()
        }
    }

    background: Rectangle {
        color: AppPalette.bg
        radius: Tokens.radiusLg
        border.width: 1
        border.color: AppPalette.border
    }

    component Spark: Item {
        id: spark
        property string title: ""
        property var values: []
        property real threshold: NaN
        property bool badAbove: true
        property int digits: 3
        property string suffix: ""
        property int fontPixelSize: Tokens.fontSm
        property bool compact: false
        property real titleWidth: 0
        property real valueWidth: 0
        property real deadThreshold: NaN
        property real rangeMin: NaN
        property real rangeMax: NaN
        readonly property real _gap: Math.round(spark.fontPixelSize * 0.5)
        readonly property real _last: {
            var v = spark.values || []
            for (var i = v.length - 1; i >= 0; --i)
                if (typeof v[i] === "number" && isFinite(v[i]))
                    return v[i]
            return NaN
        }
        readonly property bool _bad: isFinite(_last) && isFinite(threshold)
                                     && (badAbove ? _last > threshold : _last < threshold)
        readonly property bool _dead: isFinite(_last) && isFinite(deadThreshold) && _last < deadThreshold

        onValuesChanged: plot.requestPaint()
        onThresholdChanged: plot.requestPaint()
        onDeadThresholdChanged: plot.requestPaint()
        onRangeMinChanged: plot.requestPaint()
        onRangeMaxChanged: plot.requestPaint()

        Text {
            id: sparkTitle
            anchors.left: parent.left
            anchors.top: spark.compact ? undefined : parent.top
            anchors.verticalCenter: spark.compact ? parent.verticalCenter : undefined
            width: spark.compact ? spark.titleWidth : implicitWidth
            elide: Text.ElideRight
            text: spark.title
            color: AppPalette.textMuted
            font.pixelSize: spark.fontPixelSize
        }
        Text {
            id: sparkValue
            anchors.right: parent.right
            anchors.top: spark.compact ? undefined : parent.top
            anchors.verticalCenter: spark.compact ? parent.verticalCenter : undefined
            width: spark.compact ? spark.valueWidth : implicitWidth
            horizontalAlignment: Text.AlignRight
            text: isFinite(spark._last) ? spark._last.toFixed(spark.digits) + spark.suffix : "—"
            color: DataFieldCatalog.usblSeverityInk(spark._dead ? 2 : (spark._bad ? 1 : 0))
            font.pixelSize: spark.fontPixelSize
            font.bold: true
        }
        Rectangle {
            anchors.fill: plot
            color: AppPalette.bgDeep
            radius: Tokens.radiusSm
        }
        Canvas {
            id: plot
            anchors.left: spark.compact ? sparkTitle.right : parent.left
            anchors.leftMargin: spark.compact ? spark._gap : 0
            anchors.right: spark.compact ? sparkValue.left : parent.right
            anchors.rightMargin: spark.compact ? spark._gap : 0
            anchors.top: spark.compact ? parent.top : sparkTitle.bottom
            anchors.topMargin: spark.compact ? 0 : Math.round(spark.fontPixelSize * 0.25)
            anchors.bottom: parent.bottom
            antialiasing: true
            property color ink: AppPalette.text
            property color warn: AppPalette.linkIdleText
            property color fault: AppPalette.linkDownText
            property color thrInk: AppPalette.linkIdleBorder
            property color deadInk: AppPalette.linkDownBorder
            onInkChanged: requestPaint()
            onWidthChanged: requestPaint()
            onHeightChanged: requestPaint()
            onPaint: {
                var ctx = getContext("2d")
                if (!ctx)
                    return
                ctx.clearRect(0, 0, width, height)
                var v = spark.values || []
                var n = v.length
                var lo = Infinity, hi = -Infinity
                for (var i = 0; i < n; ++i) {
                    if (typeof v[i] === "number" && isFinite(v[i])) {
                        lo = Math.min(lo, v[i])
                        hi = Math.max(hi, v[i])
                    }
                }
                if (isFinite(spark.threshold)) {
                    lo = Math.min(lo, spark.threshold)
                    hi = Math.max(hi, spark.threshold)
                }
                var fixed = isFinite(spark.rangeMin) && isFinite(spark.rangeMax) && spark.rangeMax > spark.rangeMin
                if (fixed) {
                    lo = spark.rangeMin
                    hi = spark.rangeMax
                } else {
                    if (!isFinite(lo) || !isFinite(hi))
                        return
                    if (hi - lo < 1e-9) {
                        var d0 = Math.max(Math.abs(hi) * 0.5, 1e-3)
                        hi += d0
                        lo -= d0
                    }
                    var pad = (hi - lo) * 0.12
                    lo -= pad
                    hi += pad
                }
                var inset = 2
                var h = height - 2 * inset
                function yOf(x) { return inset + h - (Math.max(lo, Math.min(hi, x)) - lo) / (hi - lo) * h }
                function hline(value, ink) {
                    var ly = Math.round(yOf(value)) + 0.5
                    ctx.strokeStyle = ink
                    ctx.lineWidth = 1
                    ctx.beginPath()
                    ctx.moveTo(0, ly)
                    ctx.lineTo(width, ly)
                    ctx.stroke()
                }
                if (isFinite(spark.threshold))
                    hline(spark.threshold, plot.thrInk)
                if (isFinite(spark.deadThreshold))
                    hline(spark.deadThreshold, plot.deadInk)
                var dx = n > 1 ? (width - 2 * inset) / (n - 1) : 0
                ctx.strokeStyle = plot.ink
                ctx.lineWidth = 1.5
                ctx.beginPath()
                var started = false
                for (i = 0; i < n; ++i) {
                    if (!(typeof v[i] === "number" && isFinite(v[i]))) {
                        started = false
                        continue
                    }
                    var x = n > 1 ? inset + i * dx : width / 2
                    var y = yOf(v[i])
                    if (!started) {
                        ctx.moveTo(x, y)
                        started = true
                    } else {
                        ctx.lineTo(x, y)
                    }
                }
                ctx.stroke()
                for (i = 0; i < n; ++i) {
                    if (!(typeof v[i] === "number" && isFinite(v[i])) || !isFinite(spark.threshold))
                        continue
                    var bad = spark.badAbove ? v[i] > spark.threshold : v[i] < spark.threshold
                    if (!bad && n > 1)
                        continue
                    ctx.fillStyle = (isFinite(spark.deadThreshold) && v[i] < spark.deadThreshold) ? plot.fault : plot.warn
                    ctx.beginPath()
                    ctx.arc(n > 1 ? inset + i * dx : width / 2, yOf(v[i]), 2, 0, 2 * Math.PI)
                    ctx.fill()
                }
            }
        }
    }

    contentItem: Item {
        id: bodyBox
        readonly property bool needV: body.implicitHeight > root._maxBodyH
        readonly property real vReserve: needV ? bodyScroll.width + Tokens.spaceSm : 0
        readonly property bool needH: body.implicitWidth + vReserve > root._maxBodyW
        implicitWidth: Math.min(body.implicitWidth + vReserve, root._maxBodyW)
        implicitHeight: Math.min(body.implicitHeight + (needH ? hScroll.implicitHeight : 0), root._maxBodyH)

        Flickable {
            id: bodyFlick
            anchors.fill: parent
            anchors.rightMargin: bodyBox.vReserve
            contentWidth: body.implicitWidth
            contentHeight: body.implicitHeight
            bottomMargin: bodyBox.needH ? hScroll.implicitHeight : 0
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            interactive: bodyBox.needV || bodyBox.needH

            ScrollBar.horizontal: ScrollBar {
                id: hScroll
                policy: bodyBox.needH ? ScrollBar.AlwaysOn : ScrollBar.AlwaysOff
            }

            Column {
                id: body
                spacing: Tokens.spaceSm

                Item {
                    width: root._contentW
                    height: Math.max(Tokens.chipH, closeButton.height)

                    Row {
                        anchors.left: parent.left
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: Tokens.spaceSm

                        UsblAddressBadge {
                            address: Math.max(0, root.address)
                            diameter: Tokens.chipH
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        Text {
                            text: qsTr("Antenna diagnostics")
                            color: AppPalette.text
                            font.pixelSize: root._titleFont
                            font.bold: true
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        Text {
                            text: DataFieldCatalog.usblAntennaText(root._code)
                            color: root._codeInk(root._code)
                            font.pixelSize: root._titleFont
                            font.bold: true
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }

                    KButton {
                        id: closeButton
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        height: Tokens.controlHMd
                        fontPixelSize: Tokens.fontBase
                        text: qsTr("Close")
                        onClicked: root.close()
                    }
                }

                Text {
                    width: root._contentW
                    text: root._facts()
                    color: AppPalette.textMuted
                    font.pixelSize: root._fontSmall
                    elide: Text.ElideRight
                }

                Row {
                    id: metricsRow
                    spacing: Tokens.spaceLg

                    Canvas {
                        id: sketch
                        width: root._sketchSize
                        height: root._sketchSize
                        antialiasing: true
                        property var d: root.diag
                        property var thr: root._thr
                        property color ink: AppPalette.text
                        property color muted: AppPalette.textMuted
                        property color sep: AppPalette.separator
                        property color arrow: AppPalette.accent
                        property color face: AppPalette.bg
                        onDChanged: requestPaint()
                        onThrChanged: requestPaint()
                        onInkChanged: requestPaint()
                        Component.onCompleted: requestPaint()

                        function sensorLayout(geometry, n) {
                            if (geometry === "crossA")
                                return [[-1, 0], [0, -1], [0, 1], [1, 0], [0, 0]]
                            if (geometry === "crossB")
                                return [[0, 1], [0, -1], [-1, 0], [1, 0], [0, 0]]
                            var out = []
                            for (var i = 0; i < n; ++i) {
                                var a = Math.PI / 2 - i * 2 * Math.PI / Math.max(1, n)
                                out.push([Math.cos(a), Math.sin(a)])
                            }
                            return out
                        }

                        onPaint: {
                            var ctx = getContext("2d")
                            if (!ctx)
                                return
                            ctx.clearRect(0, 0, width, height)
                            var cx = width / 2, cy = height / 2
                            var unit = Math.min(width, height) * 0.3
                            var r = Math.max(7, unit * 0.34)
                            var reach = unit * 1.55

                            ctx.strokeStyle = sketch.sep
                            ctx.lineWidth = 1
                            ctx.beginPath()
                            ctx.moveTo(cx - reach, cy)
                            ctx.lineTo(cx + reach, cy)
                            ctx.moveTo(cx, cy - reach)
                            ctx.lineTo(cx, cy + reach)
                            ctx.stroke()
                            ctx.fillStyle = sketch.muted
                            ctx.font = Math.round(r * 0.75) + "px sans-serif"
                            ctx.textAlign = "center"
                            ctx.textBaseline = "middle"
                            ctx.fillText("x", cx + reach, cy - r * 0.7)
                            ctx.fillText("y", cx + r * 0.6, cy - reach)

                            var d = sketch.d
                            var ch = (d && d.channels) ? d.channels : []
                            var pos = sensorLayout(d ? d.geometry : "unknown", ch.length)

                            if (d && d.dirValid && isFinite(d.dirAz)) {
                                var el = isFinite(d.dirEl) ? d.dirEl : 0
                                var len = unit * 1.45 * Math.max(0.15, Math.cos(el * Math.PI / 180))
                                var az = d.dirAz * Math.PI / 180
                                var ex = cx + len * Math.cos(az), ey = cy - len * Math.sin(az)
                                var head = r * 0.75, b = Math.atan2(ey - cy, ex - cx)
                                ctx.strokeStyle = sketch.arrow
                                ctx.fillStyle = sketch.arrow
                                ctx.lineWidth = Math.max(1.5, r * 0.16)
                                ctx.beginPath()
                                ctx.moveTo(cx, cy)
                                ctx.lineTo(ex, ey)
                                ctx.stroke()
                                ctx.beginPath()
                                ctx.moveTo(ex, ey)
                                ctx.lineTo(ex - head * Math.cos(b - 0.45), ey - head * Math.sin(b - 0.45))
                                ctx.lineTo(ex - head * Math.cos(b + 0.45), ey - head * Math.sin(b + 0.45))
                                ctx.closePath()
                                ctx.fill()
                            }

                            for (var i = 0; i < ch.length && i < pos.length; ++i) {
                                var px = cx + pos[i][0] * unit, py = cy - pos[i][1] * unit
                                var sev = DataFieldCatalog.usblLevelSeverity(ch[i].levelDb, root.store)
                                var c = DataFieldCatalog.usblSeverityInk(sev, sketch.ink, sketch.muted)
                                ctx.beginPath()
                                ctx.arc(px, py, r, 0, 2 * Math.PI)
                                ctx.fillStyle = sketch.face
                                ctx.fill()
                                ctx.lineWidth = ch[i].used ? Math.max(2, r * 0.16) : 1
                                ctx.strokeStyle = c
                                ctx.stroke()
                                ctx.fillStyle = c
                                ctx.font = "bold " + Math.round(r * 0.9) + "px sans-serif"
                                ctx.fillText(String(ch[i].index), px, py)
                                if (isFinite(ch[i].levelDb) && ch[i].levelDb < -0.05) {
                                    ctx.font = Math.round(r * 0.62) + "px sans-serif"
                                    ctx.fillText(ch[i].levelDb.toFixed(1), px, py + r * 1.55)
                                }
                            }
                        }
                    }

                    Grid {
                        columns: 2
                        columnSpacing: Tokens.spaceMd
                        rowSpacing: Math.round(Tokens.spaceXs)
                        anchors.verticalCenter: parent.verticalCenter

                        Text { text: qsTr("Fit misfit"); color: AppPalette.textStrong; font.pixelSize: root._font }
                        Text {
                            text: root._fmt(root.diag ? root.diag.misfit : NaN, 3)
                            color: root._ink(DataFieldCatalog.usblMetricSeverity("misfit", root.diag ? root.diag.misfit : NaN, root.store))
                            font.pixelSize: root._font
                            font.bold: true
                        }
                        Text { text: qsTr("Amplitude part"); color: AppPalette.textStrong; font.pixelSize: root._font }
                        Text {
                            text: root._fmt(root.diag ? root.diag.amp : NaN, 4)
                            color: root._ink(DataFieldCatalog.usblMetricSeverity("amp", root.diag ? root.diag.amp : NaN, root.store))
                            font.pixelSize: root._font
                            font.bold: true
                        }
                        Text { text: qsTr("Phase part"); color: AppPalette.textStrong; font.pixelSize: root._font }
                        Text {
                            text: root._fmt(root.diag ? root.diag.phase : NaN, 4)
                            color: root._ink(DataFieldCatalog.usblMetricSeverity("phase", root.diag ? root.diag.phase : NaN, root.store))
                            font.pixelSize: root._font
                            font.bold: true
                        }
                        Text { text: qsTr("Direction width"); color: AppPalette.textStrong; font.pixelSize: root._font }
                        Text {
                            text: (root.diag && isFinite(root.diag.widthAz) && isFinite(root.diag.widthEl))
                                  ? root.diag.widthAz.toFixed(1) + "° × " + root.diag.widthEl.toFixed(1) + "°" : "—"
                            color: AppPalette.text
                            font.pixelSize: root._font
                            font.bold: true
                        }
                        Text { text: qsTr("Alt. direction margin"); color: AppPalette.textStrong; font.pixelSize: root._font }
                        Text {
                            text: root._fmt(root.diag ? root.diag.altDb : NaN, 2, " " + qsTr("dB"))
                            color: root._ink(DataFieldCatalog.usblMetricSeverity("altDb", root.diag ? root.diag.altDb : NaN, root.store))
                            font.pixelSize: root._font
                            font.bold: true
                        }
                    }

                    Grid {
                        columns: 2
                        columnSpacing: Tokens.spaceMd
                        rowSpacing: Math.round(Tokens.spaceXs)
                        anchors.verticalCenter: parent.verticalCenter

                        Text { text: qsTr("Array coherence"); color: AppPalette.textStrong; font.pixelSize: root._font }
                        Text {
                            text: root._fmt(root.diag ? root.diag.coherence : NaN, 3)
                            color: AppPalette.text
                            font.pixelSize: root._font
                            font.bold: true
                        }
                        Text { text: qsTr("Direction, array frame"); color: AppPalette.textStrong; font.pixelSize: root._font }
                        Text {
                            text: root._fmt(root.diag ? root.diag.dirAz : NaN, 1, "°") + " / "
                                  + root._fmt(root.diag ? root.diag.dirEl : NaN, 1, "°")
                            color: AppPalette.text
                            font.pixelSize: root._font
                            font.bold: true
                        }
                        Text { text: qsTr("Lowest channel SNR"); color: AppPalette.textStrong; font.pixelSize: root._font }
                        Text {
                            text: root._fmt(root.diag ? root.diag.minSnrDb : NaN, 1, " " + qsTr("dB"))
                            color: AppPalette.text
                            font.pixelSize: root._font
                            font.bold: true
                        }
                        Text { text: qsTr("Largest phase residual"); color: AppPalette.textStrong; font.pixelSize: root._font }
                        Text {
                            text: root._fmt(root.diag ? root.diag.residMaxDeg : NaN, 1, "°")
                            color: AppPalette.text
                            font.pixelSize: root._font
                            font.bold: true
                        }
                        Text { text: qsTr("Centre channel / sync"); color: AppPalette.textStrong; font.pixelSize: root._font }
                        Text {
                            text: root._signed(root.diag ? root.diag.centreSyncDb : NaN, 2, " " + qsTr("dB"))
                            color: AppPalette.text
                            font.pixelSize: root._font
                            font.bold: true
                        }
                    }
                }

                Column {
                    width: root._tableW

                    Rectangle {
                        width: parent.width
                        height: root._rowH
                        color: AppPalette.headerBg
                        radius: Tokens.radiusSm

                        Row {
                            anchors.verticalCenter: parent.verticalCenter
                            Repeater {
                                model: root._cols
                                delegate: Text {
                                    required property var modelData
                                    width: Math.round(modelData.w * root._s)
                                    rightPadding: Tokens.spaceSm
                                    horizontalAlignment: modelData.key === "index" ? Text.AlignHCenter : Text.AlignRight
                                    text: modelData.title
                                    color: AppPalette.textMuted
                                    font.pixelSize: root._sectionFont
                                    elide: Text.ElideRight
                                }
                            }
                        }
                    }

                    Repeater {
                        model: root._channels.length
                        delegate: Rectangle {
                            id: chRow
                            required property int index
                            readonly property var _ch: root._channels[index]
                            width: parent ? parent.width : root._tableW
                            height: root._rowH
                            color: index % 2 ? AppPalette.rowRaised : "transparent"

                            Row {
                                anchors.verticalCenter: parent.verticalCenter
                                Repeater {
                                    model: root._cols
                                    delegate: Text {
                                        required property var modelData
                                        width: Math.round(modelData.w * root._s)
                                        rightPadding: Tokens.spaceSm
                                        horizontalAlignment: modelData.key === "index" ? Text.AlignHCenter : Text.AlignRight
                                        text: chRow._ch ? root._cell(chRow._ch, modelData.key) : ""
                                        color: chRow._ch ? root._cellInk(chRow._ch, modelData.key) : AppPalette.text
                                        font.pixelSize: root._font
                                        font.bold: modelData.key === "levelDb"
                                    }
                                }
                            }
                        }
                    }

                    Text {
                        visible: root._channels.length === 0
                        width: parent.width
                        height: root._rowH
                        verticalAlignment: Text.AlignVCenter
                        horizontalAlignment: Text.AlignHCenter
                        text: qsTr("No channel data in the last report")
                        color: AppPalette.textMuted
                        font.pixelSize: root._font
                    }
                }

                Text {
                    width: root._contentW
                    text: qsTr("Last %1 reports").arg(root.history.length)
                    color: AppPalette.textStrong
                    font.pixelSize: root._sectionFont
                }

                Row {
                    id: sparkRow
                    spacing: Tokens.spaceMd
                    readonly property real cellW: Math.floor((root._contentW - Tokens.spaceMd) / 2)
                    readonly property real cellH: Math.round(66 * root._s)

                    Spark {
                        width: sparkRow.cellW; height: sparkRow.cellH
                        title: qsTr("Fit misfit")
                        values: root._series("misfit")
                        threshold: root._thr.misfit
                        digits: 3
                    }
                    Spark {
                        width: sparkRow.cellW; height: sparkRow.cellH
                        title: qsTr("Width az, °")
                        values: root._series("widthAz")
                        digits: 1
                    }
                }

                Item {
                    visible: root._levelChannels > 0
                    width: root._contentW
                    height: levelModeBar.height

                    Text {
                        anchors.left: parent.left
                        anchors.right: levelModeBar.left
                        anchors.rightMargin: Tokens.spaceSm
                        anchors.verticalCenter: parent.verticalCenter
                        elide: Text.ElideRight
                        text: root._absolute ? qsTr("Channel peak level, dB re one matched-filter unit")
                                             : qsTr("Channel level re the strongest, dB")
                        color: AppPalette.textStrong
                        font.pixelSize: root._sectionFont
                    }

                    KTabBar {
                        id: levelModeBar
                        anchors.right: parent.right
                        width: Math.round(200 * root._s)
                        buttonHeight: Tokens.controlHSm
                        fontPixelSize: Tokens.fontSm
                        trackColor: AppPalette.bgDeep
                        options: [
                            { label: qsTr("Relative"), value: false },
                            { label: qsTr("Absolute"), value: true }
                        ]
                        currentValue: root._absolute
                        onValueSelected: function(v) { if (root.store) root.store.usblDiagLevelAbsolute = v }
                    }
                }

                Column {
                    spacing: Math.round(3 * root._s)

                    Repeater {
                        model: root._levelChannels
                        delegate: Spark {
                            required property int index
                            width: root._contentW
                            height: Math.round(34 * root._s)
                            compact: true
                            titleWidth: Math.round(52 * root._s)
                            valueWidth: Math.round(84 * root._s)
                            title: qsTr("Ch %1").arg(index)
                            values: root._levelSeries(index)
                            threshold: root._absolute ? NaN : root._thr.levelDb
                            deadThreshold: root._absolute ? NaN : root._levelDead
                            badAbove: false
                            digits: 1
                            suffix: " " + qsTr("dB")
                            rangeMin: root._levelRange.lo
                            rangeMax: root._levelRange.hi
                        }
                    }
                }
            }
        }

        KScrollBar {
            id: bodyScroll
            flickable: bodyFlick
            visible: bodyBox.needV
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.right: parent.right
        }
    }
}
