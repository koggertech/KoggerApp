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
    property real msPerPx: 4

    signal sendNowRequested(int stepId)
    signal payloadSendRequested(int stepId)
    signal quickInterrogateRequested(var dev)

    readonly property real axisW: Math.round(64 * AppPalette.scale)
    readonly property real headerH: Math.round(70 * AppPalette.scale)
    readonly property real colMinW: Math.round(170 * AppPalette.scale)
    readonly property var lls: store ? store.lifelines : []
    readonly property real colW: lls.length ? Math.max(colMinW, (width - axisW) / lls.length) : colMinW
    readonly property real contentW: axisW + colW * Math.max(1, lls.length)
    readonly property color reqColor: AppPalette.accentBorder
    readonly property color respColor: AppPalette.linkOkBorder
    readonly property color badColor: AppPalette.dangerBorder
    readonly property color lineColor: AppPalette.border

    function colOf(id) {
        for (var i = 0; i < lls.length; ++i) if (lls[i].id === id) return i
        return -1
    }
    function xOf(id) {
        var c = colOf(id)
        return c < 0 ? -1 : axisW + colW * c + colW / 2
    }
    function addrColor(addr) {
        return (typeof addr === "number" && addr >= 0) ? DataFieldCatalog.usblAddressColor(addr) : AppPalette.textMuted
    }

    readonly property var layout: {
        var out = [], y = Tokens.spaceMd
        if (!store) return out
        var steps = store.compiled.steps
        for (var i = 0; i < steps.length; ++i) {
            var s = steps[i]
            var ex = Model.exchange(store.st, s.exchangeId)
            var tgt = ex ? Model.targetLifeline(store.st, ex) : null
            var ini = ex ? Model.lifeline(store.st, ex.from) : null
            var spec = {}
            for (var k in s.spec) spec[k] = s.spec[k]
            spec.plannedRangeM = tgt && typeof tgt.plannedRangeM === "number" ? tgt.plannedRangeM : null
            spec.blindAfterRequestMs = ini ? ini.monitor.blindAfterRequestMs : 0
            spec.blindAfterResponseMs = tgt ? tgt.monitor.blindAfterResponseMs : 0
            var ev = Timing.stepEvents(spec)
            var h = s.enabled ? Math.max(Math.round(54 * AppPalette.scale), s.durationMs / root.msPerPx)
                              : Math.round(30 * AppPalette.scale)
            var listeners = []
            if (ex) for (var q = 0; q < ex.listeners.length; ++q) listeners.push(ex.listeners[q].lifeline)
            out.push({ step: s, ex: ex, ini: ini, tgt: tgt, ev: ev, y: y, h: h, listeners: listeners,
                       targetAddr: s.addr })
            y += h + Tokens.spaceXs
        }
        return out
    }
    readonly property real contentH: {
        var last = layout.length ? layout[layout.length - 1] : null
        return (last ? last.y + last.h : 0) + Math.round(140 * AppPalette.scale)
    }

    function _stepIndexAtY(y) {
        for (var i = 0; i < layout.length; ++i) if (y < layout[i].y + layout[i].h / 2) return i
        return layout.length
    }

    // ── sticky lifeline headers ──────────────────────────────────────────
    Item {
        id: headerBar
        width: parent.width
        height: root.headerH
        clip: true
        z: 5

        Rectangle { anchors.fill: parent; color: Qt.rgba(AppPalette.bg.r, AppPalette.bg.g, AppPalette.bg.b, 0.0) }
        TapHandler {
            onTapped: if (root.store) root.store.select("network", -1)
        }

        Item {
            x: -flick.contentX
            width: root.contentW
            height: parent.height

            Text {
                x: Tokens.spaceSm
                y: parent.height - height - Tokens.spaceXs
                text: qsTr("time")
                color: AppPalette.textMuted
                font.pixelSize: Tokens.fontXxs
            }

            Repeater {
                model: root.lls
                delegate: Rectangle {
                    id: hdr
                    required property var modelData
                    required property int index
                    readonly property bool _sel: root.store && root.store.selKind === "lifeline" && root.store.selId === modelData.id
                    readonly property var _dev: root.store ? root.store.deviceFor(modelData.id) : null
                    readonly property string _ws: root.store ? root.store.writeStateOf(modelData.id) : ""
                    x: root.axisW + root.colW * index + Tokens.spaceXs
                    y: Tokens.spaceXs
                    width: root.colW - 2 * Tokens.spaceXs
                    height: root.headerH - 2 * Tokens.spaceXs
                    radius: Tokens.radiusMd
                    color: AppPalette.rowRaised
                    border.width: _sel ? Math.max(1, Math.round(2 * AppPalette.scale)) : Math.max(1, Math.round(AppPalette.scale))
                    border.color: _sel ? AppPalette.accentBorder : AppPalette.border
                    opacity: modelData.active === false ? 0.6 : 1.0

                    function openMenu(scenePos) {
                        if (root.store) root.store.select("lifeline", hdr.modelData.id)
                        var p = scenePos ? scenePos : hdr.mapToItem(null, hdr.width / 2, hdr.height / 2)
                        llMenu.llId = hdr.modelData.id
                        llMenu.openAt(p.x, p.y)
                    }

                    Rectangle {
                        width: parent.width; height: Math.max(2, Math.round(3 * AppPalette.scale))
                        radius: height / 2
                        color: root.addrColor(modelData.address)
                    }

                    Column {
                        x: Tokens.spaceSm; y: Tokens.spaceSm
                        width: parent.width - 2 * Tokens.spaceSm - linkHandle.width
                        spacing: 1
                        Row {
                            spacing: Tokens.spaceXs
                            UsblAddressBadge {
                                visible: typeof hdr.modelData.address === "number"
                                address: typeof hdr.modelData.address === "number" ? hdr.modelData.address : 0
                                diameter: Math.round(18 * AppPalette.scale)
                                anchors.verticalCenter: parent.verticalCenter
                            }
                            Text {
                                text: hdr.modelData.label
                                color: AppPalette.textStrong
                                font.pixelSize: Tokens.fontSm; font.bold: true
                                elide: Text.ElideRight
                                width: Math.min(implicitWidth, hdr.width - Math.round(88 * AppPalette.scale))
                                anchors.verticalCenter: parent.verticalCenter
                            }
                        }
                        Text {
                            text: {
                                var k = hdr.modelData.kind === "head" ? qsTr("head") : (hdr.modelData.kind === "external" ? qsTr("external") : qsTr("beacon"))
                                var r = hdr.modelData.respond === "off" ? qsTr("silent") : qsTr("answers")
                                return k + " · " + r
                            }
                            color: AppPalette.textMuted
                            font.pixelSize: Tokens.fontXxs
                        }
                        Row {
                            spacing: Tokens.spaceXs
                            Rectangle {
                                width: Math.round(7 * AppPalette.scale); height: width; radius: width / 2
                                anchors.verticalCenter: parent.verticalCenter
                                color: hdr._dev ? AppPalette.linkOkBorder : "transparent"
                                border.width: hdr._dev ? 0 : 1
                                border.color: hdr.modelData.binding ? AppPalette.linkIdleBorder : AppPalette.textMuted
                            }
                            Text {
                                text: hdr._dev ? ("SN " + hdr._dev.devSN)
                                     : (hdr.modelData.binding ? qsTr("SN %1 · not connected").arg(hdr.modelData.binding.serial)
                                     : (hdr.modelData.kind === "external" ? qsTr("not bindable") : qsTr("unbound")))
                                color: AppPalette.textSecond
                                font.pixelSize: Tokens.fontXxs
                                elide: Text.ElideRight
                                width: Math.min(implicitWidth, hdr.width - Math.round(40 * AppPalette.scale))
                            }
                        }
                        Text {
                            visible: hdr.modelData.kind !== "external"
                            text: hdr._ws === "confirmed" ? qsTr("✓ written") : hdr._ws === "writing" ? qsTr("writing…")
                                : hdr._ws === "changed" || hdr._ws === "earlierChanged" ? qsTr("changed — write")
                                : hdr._ws === "earlier" ? qsTr("written earlier")
                                : hdr._ws === "rejected" || hdr._ws === "noAnswer" || hdr._ws === "partial" ? qsTr("write failed")
                                : hdr._ws === "baseline" ? qsTr("baseline") : ""
                            color: hdr._ws === "confirmed" ? AppPalette.linkOkText
                                 : (hdr._ws === "rejected" || hdr._ws === "noAnswer" || hdr._ws === "partial") ? AppPalette.dangerText
                                 : AppPalette.linkIdleText
                            font.pixelSize: Tokens.fontXxs; font.bold: true
                        }
                    }

                    KTapArea {
                        anchors.fill: parent
                        onTapped: if (root.store) root.store.select("lifeline", hdr.modelData.id)
                        onLongPressed: hdr.openMenu()
                    }
                    TapHandler {
                        acceptedButtons: Qt.RightButton
                        onTapped: function (eventPoint) { hdr.openMenu(eventPoint.scenePosition) }
                    }

                    KSwitch {
                        visible: hdr.modelData.kind !== "head" && typeof hdr.modelData.address === "number"
                        flat: true
                        anchors.right: parent.right
                        anchors.rightMargin: Tokens.spaceXs
                        y: Tokens.spaceXs
                        trackWidth: Math.round(30 * AppPalette.scale)
                        trackHeight: Math.round(16 * AppPalette.scale)
                        width: trackWidth
                        height: trackHeight + 2 * Tokens.spaceXxs
                        checked: hdr.modelData.active !== false
                        toolTipText: checked ? qsTr("In the cycle: interrogated. Click to skip it.") : qsTr("Skipped by the cycle. Click to interrogate it.")
                        onToggled: if (root.store) root.store.setLifelineField(hdr.modelData.id, "active", checked)
                    }

                    Rectangle {
                        id: linkHandle
                        anchors.right: parent.right
                        anchors.rightMargin: Tokens.spaceXs
                        anchors.verticalCenter: parent.verticalCenter
                        width: Math.round(22 * AppPalette.scale); height: width
                        radius: Tokens.radiusSm
                        color: linkDrag.active ? AppPalette.accentBg : AppPalette.controlRaised
                        border.width: 1
                        border.color: AppPalette.border
                        Text {
                            anchors.centerIn: parent
                            text: "⟶"
                            color: AppPalette.accentText
                            font.pixelSize: Tokens.fontSm
                        }
                        HoverHandler { cursorShape: Qt.DragLinkCursor }
                        KToolTip {
                            text: qsTr("Drag to another lifeline to add an exchange")
                            shown: linkHover.hovered && !linkDrag.active
                        }
                        HoverHandler { id: linkHover }
                        DragHandler {
                            id: linkDrag
                            target: null
                            onActiveChanged: {
                                if (active) {
                                    rubber.fromId = hdr.modelData.id
                                    rubber.x0 = root.xOf(hdr.modelData.id) - flick.contentX
                                    rubber.visible = true
                                } else {
                                    rubber.visible = false
                                    var p = linkHandle.mapToItem(root, centroid.position.x, centroid.position.y)
                                    var col = Math.floor((p.x + flick.contentX - root.axisW) / root.colW)
                                    if (col >= 0 && col < root.lls.length && root.lls[col].id !== hdr.modelData.id && root.store) {
                                        var idx = root._stepIndexAtY(p.y - root.headerH + flick.contentY)
                                        root.store.addExchange(hdr.modelData.id, root.lls[col].id, "ranging", idx)
                                    }
                                }
                            }
                            onCentroidChanged: {
                                if (!active) return
                                var p = linkHandle.mapToItem(root, centroid.position.x, centroid.position.y)
                                rubber.x1 = p.x
                                rubber.y1 = p.y
                                rubber.requestPaint()
                            }
                        }
                    }
                }
            }
        }
    }

    // ── body ─────────────────────────────────────────────────────────────
    Flickable {
        id: flick
        anchors.top: headerBar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        contentWidth: root.contentW
        contentHeight: root.contentH
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        interactive: !reorder.active

        WheelHandler {
            acceptedModifiers: Qt.ControlModifier
            onWheel: function (event) {
                var f = event.angleDelta.y > 0 ? 0.85 : 1.18
                root.msPerPx = Math.max(0.5, Math.min(40, root.msPerPx * f))
            }
        }

        Item {
            id: content
            width: root.contentW
            height: root.contentH

            TapHandler {
                onTapped: if (root.store) root.store.select("network", -1)
            }

            Repeater {
                model: root.lls
                delegate: Rectangle {
                    required property var modelData
                    required property int index
                    x: root.axisW + root.colW * index + root.colW / 2
                    width: Math.max(1, Math.round(AppPalette.scale))
                    height: content.height
                    color: root.addrColor(modelData.address)
                    opacity: 0.35
                }
            }

            Text {
                visible: root.layout.length === 0
                x: root.axisW + Tokens.spaceXl
                y: Tokens.spaceXl
                width: Math.max(0, root.width - root.axisW - 2 * Tokens.spaceXl)
                wrapMode: Text.WordWrap
                text: root.lls.length < 2
                      ? qsTr("Add a head and a beacon (toolbar or Devices panel), then drag the ⟶ handle of the head onto the beacon to add an exchange.")
                      : qsTr("Drag the ⟶ handle of an initiator's header onto another lifeline to add an exchange.")
                color: AppPalette.textMuted
                font.pixelSize: Tokens.fontSm
            }

            Repeater {
                model: root.layout
                delegate: Item {
                    id: band
                    required property var modelData
                    required property int index
                    readonly property var s: modelData.step
                    readonly property var ev: modelData.ev
                    readonly property bool _sel: root.store && root.store.selKind === "step" && root.store.selId === s.stepId
                    readonly property var _log: root.engine && root.engine.stepLog ? root.engine.stepLog[s.stepId] : null
                    readonly property bool _replied: root.live && !!root.store && !!_log && _log.state === "replied"
                    readonly property bool _cur: root.live && root.engine && root.engine.curStep && root.engine.curStep.stepId === s.stepId
                    readonly property real _xi: root.xOf(modelData.ini ? modelData.ini.id : -1)
                    readonly property real _xt: modelData.tgt ? root.xOf(modelData.tgt.id) : -1
                    readonly property real _overflow: Math.max(0, ev.windowTo / root.msPerPx - modelData.h,
                                                               (ev.followUp ? ev.followUp.windowTo : 0) / root.msPerPx - modelData.h)
                    y: modelData.y
                    width: content.width
                    height: modelData.h
                    z: _sel ? 3 : 1
                    opacity: s.enabled ? 1.0 : 0.45

                    Rectangle {
                        x: root.axisW
                        width: parent.width - root.axisW
                        height: parent.height
                        radius: Tokens.radiusSm
                        color: band.index % 2 ? Qt.rgba(AppPalette.rowRaised.r, AppPalette.rowRaised.g, AppPalette.rowRaised.b, 0.35)
                                              : Qt.rgba(AppPalette.card.r, AppPalette.card.g, AppPalette.card.b, 0.25)
                        border.width: band._sel || band._cur ? Math.max(1, Math.round(2 * AppPalette.scale)) : 0
                        border.color: band._cur ? AppPalette.accent : AppPalette.accentBorder
                    }

                    Canvas {
                        id: cv
                        x: 0; y: 0
                        width: parent.width
                        height: parent.height + band._overflow + Tokens.spaceSm
                        renderStrategy: Canvas.Cooperative
                        property var deps: [band.ev, band._xi, band._xt, root.msPerPx, band._log, root.live, band._sel]
                        onDepsChanged: requestPaint()
                        onWidthChanged: requestPaint()

                        function arrow(ctx, x1, y1, x2, y2, color, w, open) {
                            ctx.strokeStyle = color
                            ctx.fillStyle = color
                            ctx.lineWidth = w
                            ctx.beginPath(); ctx.moveTo(x1, y1); ctx.lineTo(x2, y2); ctx.stroke()
                            var a = Math.atan2(y2 - y1, x2 - x1), L = 9 * AppPalette.scale
                            ctx.beginPath()
                            ctx.moveTo(x2, y2)
                            ctx.lineTo(x2 - L * Math.cos(a - 0.4), y2 - L * Math.sin(a - 0.4))
                            ctx.lineTo(x2 - L * Math.cos(a + 0.4), y2 - L * Math.sin(a + 0.4))
                            ctx.closePath()
                            if (open) ctx.stroke(); else ctx.fill()
                        }
                        function dashed(ctx, x1, y1, x2, y2, color, w) {
                            var len = Math.sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1))
                            var n = Math.max(1, Math.floor(len / (8 * AppPalette.scale)))
                            ctx.strokeStyle = color; ctx.lineWidth = w
                            ctx.beginPath()
                            for (var i = 0; i < n; i += 2) {
                                ctx.moveTo(x1 + (x2 - x1) * i / n, y1 + (y2 - y1) * i / n)
                                ctx.lineTo(x1 + (x2 - x1) * (i + 1) / n, y1 + (y2 - y1) * (i + 1) / n)
                            }
                            ctx.stroke()
                        }

                        onPaint: {
                            var ctx = getContext("2d")
                            ctx.reset()
                            var e = band.ev, mp = root.msPerPx, xi = band._xi, xt = band._xt
                            if (xi < 0) return
                            var w = Math.max(1.5, 1.6 * AppPalette.scale)
                            var bw = 8 * AppPalette.scale
                            var h = band.height

                            if (!e.ping) {
                                var wy0 = e.windowFrom / mp, wy1 = e.windowTo / mp
                                ctx.fillStyle = Qt.rgba(root.reqColor.r, root.reqColor.g, root.reqColor.b, 0.22)
                                ctx.fillRect(xi - bw / 2, wy0, bw, Math.max(0, Math.min(wy1, h) - wy0))
                                if (wy1 > h && band.s.enabled) {
                                    ctx.fillStyle = Qt.rgba(root.badColor.r, root.badColor.g, root.badColor.b, 0.45)
                                    ctx.fillRect(xi - bw / 2, Math.max(wy0, h), bw, wy1 - Math.max(wy0, h))
                                }
                            }
                            if (e.blindHead && e.blindHead[1] > 0) {
                                ctx.fillStyle = Qt.rgba(AppPalette.textMuted.r, AppPalette.textMuted.g, AppPalette.textMuted.b, 0.25)
                                ctx.fillRect(xi + bw / 2 + 1, 0, 2 * AppPalette.scale, Math.min(h, e.blindHead[1] / mp))
                            }

                            if (xt >= 0) {
                                if (e.ping) dashed(ctx, xi, 0, xt, e.reqAtTarget / mp, root.reqColor, w)
                                arrow(ctx, xi, 0, xt, e.reqAtTarget / mp, root.reqColor, w, e.ping)
                                if (!e.ping) {
                                    ctx.fillStyle = Qt.rgba(root.respColor.r, root.respColor.g, root.respColor.b, 0.28)
                                    ctx.fillRect(xt - bw / 2, e.reqAtTarget / mp, bw, (e.replyTx - e.reqAtTarget) / mp)
                                    arrow(ctx, xt, e.replyTx / mp, xi, e.replyAtHead / mp, root.respColor, w, false)
                                    if (e.followUp) {
                                        var f = e.followUp
                                        arrow(ctx, xi, f.tx / mp, xt, f.atTarget / mp, root.reqColor, w, false)
                                        ctx.fillStyle = Qt.rgba(root.respColor.r, root.respColor.g, root.respColor.b, 0.28)
                                        ctx.fillRect(xt - bw / 2, f.atTarget / mp, bw, (f.replyTx - f.atTarget) / mp)
                                        arrow(ctx, xt, f.replyTx / mp, xi, f.replyAtHead / mp, root.respColor, w, false)
                                        ctx.fillStyle = Qt.rgba(root.reqColor.r, root.reqColor.g, root.reqColor.b, 0.18)
                                        ctx.fillRect(xi - bw / 2, f.windowFrom / mp, bw, (f.windowTo - f.windowFrom) / mp)
                                        ctx.strokeStyle = AppPalette.textMuted; ctx.lineWidth = 1
                                        ctx.beginPath()
                                        ctx.moveTo(xi - bw, e.replyAtHead / mp); ctx.lineTo(xi - bw - 5, e.replyAtHead / mp)
                                        ctx.lineTo(xi - bw - 5, f.tx / mp); ctx.lineTo(xi - bw, f.tx / mp)
                                        ctx.stroke()
                                    }
                                }
                            }
                            var ls = band.modelData.listeners
                            for (var i = 0; i < ls.length; ++i) {
                                var xl = root.xOf(ls[i])
                                if (xl >= 0) dashed(ctx, xi, 0, xl, e.reqAtTarget / mp, Qt.rgba(root.reqColor.r, root.reqColor.g, root.reqColor.b, 0.55), 1)
                            }

                            if (root.live && band._log) {
                                var L = band._log
                                if (L.state === "missed") {
                                    var mx = xi, my = Math.min(h - 6, e.windowTo / mp), r = 6 * AppPalette.scale
                                    ctx.strokeStyle = root.badColor; ctx.lineWidth = 2.2
                                    ctx.beginPath(); ctx.moveTo(mx - r, my - r); ctx.lineTo(mx + r, my + r)
                                    ctx.moveTo(mx + r, my - r); ctx.lineTo(mx - r, my + r); ctx.stroke()
                                } else if (L.state === "replied") {
                                    ctx.fillStyle = root.respColor
                                    ctx.beginPath(); ctx.arc(xi, e.replyAtHead / mp, 5 * AppPalette.scale, 0, 2 * Math.PI); ctx.fill()
                                } else if (L.state === "waiting") {
                                    ctx.strokeStyle = root.reqColor; ctx.lineWidth = 2
                                    ctx.beginPath(); ctx.arc(xi, 0, 7 * AppPalette.scale, 0, 2 * Math.PI); ctx.stroke()
                                }
                            }
                        }
                    }

                    Text {
                        x: Math.min(band._xi, band._xt >= 0 ? band._xt : band._xi) + Tokens.spaceLg
                        y: Tokens.spaceXxs
                        visible: band._xi >= 0
                        width: Math.max(0, Math.abs((band._xt >= 0 ? band._xt : band._xi) - band._xi) - 2 * Tokens.spaceLg)
                        horizontalAlignment: band._xt >= band._xi ? Text.AlignLeft : Text.AlignRight
                        elide: Text.ElideRight
                        text: {
                            var ex = band.modelData.ex
                            var d = ex ? root.store.dataSummary(ex.request.data) : ""
                            return "cmd " + band.s.cmd + (band.s.kind === "ping" ? " · " + qsTr("ping") : "")
                                 + (d.length ? " · " + d : "")
                                 + (band.s.kind !== "ping" ? " · ≤" + Math.round(band.s.replyMm / 1000) + " m" : "")
                        }
                        color: AppPalette.accentText
                        font.pixelSize: Tokens.fontXxs; font.bold: true
                    }
                    Text {
                        visible: band.s.kind !== "ping" && band._xt >= 0 && band._xi >= 0
                        x: Math.min(band._xi, band._xt) + Tokens.spaceLg
                        y: Math.min(band.height - height - 2, band.ev.replyAtHead / root.msPerPx - height - 2)
                        width: Math.max(0, Math.abs(band._xt - band._xi) - 2 * Tokens.spaceLg)
                        horizontalAlignment: band._xt >= band._xi ? Text.AlignRight : Text.AlignLeft
                        elide: Text.ElideRight
                        text: {
                            var ex = band.modelData.ex
                            var d = ex ? root.store.dataSummary(ex.response.data) : ""
                            var live = band._replied ? root.store.fixText(band._log) : ""
                            return qsTr("reply") + (d.length ? " · " + d : "") + (live.length ? " · " + live : "")
                        }
                        color: AppPalette.linkOkText
                        font.pixelSize: Tokens.fontXxs; font.bold: true
                    }
                    Text {
                        readonly property string _sync: band._replied ? root.store.syncText(band._log) : ""
                        visible: _sync.length > 0 && band._xt >= 0 && band._xi >= 0
                        x: Math.min(band._xi, band._xt) + Tokens.spaceLg
                        y: band.ev.replyAtHead / root.msPerPx + Tokens.spaceXxs
                        width: Math.max(0, Math.abs(band._xt - band._xi) - 2 * Tokens.spaceLg)
                        horizontalAlignment: band._xt >= band._xi ? Text.AlignRight : Text.AlignLeft
                        elide: Text.ElideRight
                        text: _sync
                        color: AppPalette.textSecond
                        font.pixelSize: Tokens.fontXxs
                    }

                    Column {
                        x: Tokens.spaceXs
                        y: Tokens.spaceXxs
                        width: root.axisW - Tokens.spaceSm
                        spacing: 0
                        Text {
                            text: "#" + (band.index + 1) + (band.s.on ? "" : " ⏸")
                            color: AppPalette.textStrong
                            font.pixelSize: Tokens.fontXs; font.bold: true
                        }
                        Text {
                            text: band.s.enabled ? (band.s.startMs / 1000).toFixed(2) + " s" : qsTr("off")
                            color: AppPalette.textMuted
                            font.pixelSize: Tokens.fontXxs
                        }
                        Text {
                            visible: band.s.enabled
                            text: (band.s.durationMs / 1000).toFixed(2) + " s"
                            color: band.s.durationMs < band.s.requiredMs ? AppPalette.dangerText : AppPalette.textSecond
                            font.pixelSize: Tokens.fontXxs; font.bold: band.s.durationMs < band.s.requiredMs
                        }
                        Text {
                            visible: root.live && !!band._log && band._log.count > 0
                            text: band._log ? (band._log.replies || 0) + "/" + band._log.count : ""
                            color: AppPalette.textMuted
                            font.pixelSize: Tokens.fontXxs
                        }
                    }

                    MouseArea {
                        x: root.axisW
                        width: parent.width - root.axisW
                        height: parent.height
                        acceptedButtons: Qt.LeftButton | Qt.RightButton
                        onClicked: function (mouse) {
                            if (!root.store) return
                            root.store.select("step", band.s.stepId)
                            if (mouse.button === Qt.RightButton) {
                                var p = mapToItem(null, mouse.x, mouse.y)
                                stepMenu.stepId = band.s.stepId
                                stepMenu.exchangeId = band.s.exchangeId
                                stepMenu.openAt(p.x, p.y)
                            }
                        }
                        onDoubleClicked: if (root.live) root.sendNowRequested(band.s.stepId)
                    }

                    Repeater {
                        model: root.live && root.engine ? root.engine.legsOf(band.s.exchangeId) : []
                        delegate: Text {
                            id: rxMark
                            required property var modelData
                            readonly property bool _reply: modelData.leg === "response" || modelData.leg === "followUpReply"
                            readonly property var _ds: root.engine.dataState(band.s.stepId, modelData.leg, modelData.lifelineId)
                            readonly property real _xr: root.xOf(modelData.lifelineId)
                            readonly property real _xs: _reply && band._xt >= 0 ? band._xt : band._xi
                            readonly property bool _right: _xr >= _xs
                            readonly property real _atMs: {
                                var e = band.ev, f = e.followUp
                                switch (modelData.leg) {
                                case "request": return e.reqAtTarget
                                case "response": return e.replyAtHead
                                case "followUp": return f ? f.atTarget : e.reqAtTarget
                                }
                                return f ? f.replyAtHead : e.replyAtHead
                            }
                            visible: _xr >= 0 && _ds.state.length > 0
                            x: _right ? _xr + Tokens.spaceSm : _xr - Tokens.spaceSm - width
                            y: _atMs / root.msPerPx + Tokens.spaceXxs
                            width: Math.max(0, root.colW / 2 - Tokens.spaceMd)
                            horizontalAlignment: _right ? Text.AlignLeft : Text.AlignRight
                            elide: Text.ElideRight
                            text: root.store ? root.store.rxMarkText(_ds, modelData) : ""
                            color: _ds.state === "match" ? AppPalette.linkOkText
                                 : _ds.state === "waiting" ? AppPalette.textMuted
                                 : _ds.state === "missing" ? AppPalette.linkIdleText
                                 : AppPalette.dangerText
                            font.pixelSize: Tokens.fontXxs; font.bold: true
                            HoverHandler { id: rxHover }
                            KToolTip {
                                text: root.store ? root.store.rxDetailText(rxMark._ds, rxMark.modelData) : ""
                                shown: rxHover.hovered && rxMark.visible
                            }
                        }
                    }

                    Rectangle {
                        id: grip
                        x: Tokens.spaceXxs
                        y: parent.height - height - Tokens.spaceXxs
                        width: root.axisW - Tokens.spaceSm
                        height: Math.round(12 * AppPalette.scale)
                        color: "transparent"
                        Text { anchors.centerIn: parent; text: "⋮⋮"; color: AppPalette.textMuted; font.pixelSize: Tokens.fontXs; rotation: 90 }
                        HoverHandler { cursorShape: Qt.SizeAllCursor }
                        DragHandler {
                            id: reorderOne
                            target: null
                            yAxis.enabled: true; xAxis.enabled: false
                            onActiveChanged: {
                                if (active) { reorder.dragStep = band.s.stepId; reorder.active = true }
                                else {
                                    reorder.active = false
                                    var p = grip.mapToItem(content, 0, centroid.position.y)
                                    var to = root._stepIndexAtY(p.y)
                                    if (to > band.index) to -= 1
                                    if (to !== band.index && root.store) root.store.moveStep(band.s.stepId, to)
                                    reorder.dropY = -1
                                }
                            }
                            onCentroidChanged: if (active) reorder.dropY = grip.mapToItem(content, 0, centroid.position.y).y
                        }
                    }

                    MouseArea {
                        id: resizer
                        x: root.axisW
                        y: parent.height - height / 2
                        width: parent.width - root.axisW
                        height: Math.round(8 * AppPalette.scale)
                        visible: band.s.enabled
                        cursorShape: Qt.SizeVerCursor
                        preventStealing: true
                        property real _h0: 0
                        property real _y0: 0
                        property real _preview: -1
                        onPressed: function (mouse) { _h0 = band.height; _y0 = mapToItem(content, mouse.x, mouse.y).y }
                        onPositionChanged: function (mouse) {
                            if (!pressed) return
                            var dy = mapToItem(content, mouse.x, mouse.y).y - _y0
                            _preview = Math.max(100, Math.round((_h0 + dy) * root.msPerPx / 10) * 10)
                        }
                        onReleased: {
                            if (_preview > 0 && root.store) root.store.setStepField(band.s.stepId, "durationMs", _preview)
                            _preview = -1
                        }
                        onDoubleClicked: if (root.store) root.store.setStepField(band.s.stepId, "durationMs", "auto")
                        Rectangle {
                            visible: resizer.containsMouse || resizer.pressed
                            anchors.verticalCenter: parent.verticalCenter
                            width: parent.width; height: 2
                            color: AppPalette.accentBorder
                        }
                        Text {
                            visible: resizer.pressed && resizer._preview > 0
                            anchors.right: parent.right
                            anchors.bottom: parent.top
                            text: (resizer._preview / 1000).toFixed(2) + " s" + (resizer._preview < band.s.requiredMs ? "  < " + (band.s.requiredMs / 1000).toFixed(2) + " s" : "")
                            color: resizer._preview < band.s.requiredMs ? AppPalette.dangerText : AppPalette.textStrong
                            font.pixelSize: Tokens.fontXs; font.bold: true
                        }
                        hoverEnabled: true
                    }

                }
            }

            Rectangle {
                visible: reorder.active && reorder.dropY >= 0
                x: 0
                y: {
                    var i = root._stepIndexAtY(reorder.dropY)
                    if (i < root.layout.length) return root.layout[i].y - 2
                    var last = root.layout.length ? root.layout[root.layout.length - 1] : null
                    return last ? last.y + last.h + 1 : 0
                }
                width: content.width
                height: Math.max(2, Math.round(3 * AppPalette.scale))
                color: AppPalette.accent
            }

            Text {
                visible: root.layout.length > 0
                x: root.axisW + Tokens.spaceSm
                y: {
                    var last = root.layout[root.layout.length - 1]
                    return last ? last.y + last.h + Tokens.spaceMd : 0
                }
                text: root.store ? qsTr("loop · cycle %1 s · %2").arg((root.store.compiled.totalMs / 1000).toFixed(2))
                                   .arg(root.store.st.cycle.stepMode === "uniform" ? qsTr("uniform steps")
                                        : root.store.st.cycle.stepMode === "period" ? qsTr("period %1 s").arg((root.store.st.cycle.periodMs / 1000).toFixed(1))
                                        : qsTr("auto steps")) : ""
                color: AppPalette.textSecond
                font.pixelSize: Tokens.fontXs; font.bold: true
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

    QtObject {
        id: reorder
        property bool active: false
        property int dragStep: -1
        property real dropY: -1
    }

    NodesMenu {
        id: stepMenu
        property int stepId: -1
        property int exchangeId: -1
        readonly property var _st: visible && root.store ? Model.step(root.store.st, stepId) : null
        groups: !_st ? [] : [
            [ { text: qsTr("Send now"), enabled: !!root.engine,
                onTriggered: function () { root.sendNowRequested(stepMenu.stepId) } },
              { text: qsTr("Send with payload…"), enabled: !!root.engine,
                onTriggered: function () { root.payloadSendRequested(stepMenu.stepId) } } ],
            [ { text: qsTr("In the cycle"), checked: _st.on !== false,
                onTriggered: function () {
                    var s = Model.step(root.store.st, stepMenu.stepId)
                    if (s) root.store.setStepField(stepMenu.stepId, "on", s.on === false)
                } },
              { text: qsTr("Duplicate step"), onTriggered: function () { root.store.duplicateStep(stepMenu.stepId) } },
              { text: qsTr("Automatic duration"), onTriggered: function () { root.store.setStepField(stepMenu.stepId, "durationMs", "auto") } } ],
            [ { text: qsTr("Delete step"), danger: true, onTriggered: function () { root.store.removeStep(stepMenu.stepId) } },
              { text: qsTr("Delete exchange"), danger: true, onTriggered: function () { root.store.removeExchange(stepMenu.exchangeId) } } ]
        ]
    }

    NodesMenu {
        id: llMenu
        property int llId: -1
        readonly property var _ll: visible && root.store ? Model.lifeline(root.store.st, llId) : null
        readonly property int _col: root.colOf(llId)
        readonly property var _dev: _ll ? root.store.deviceFor(llId) : null
        readonly property bool _ext: !!_ll && _ll.kind === "external"
        readonly property bool _runs: !!_ll && !!root.engine && root.engine.isRunningFor(llId)
        readonly property bool _canRun: !!_ll && !!root.engine && root.engine.runnableFor(llId).length > 0
        readonly property bool _canWrite: !_ext && !!_dev && !!root.writer && !root.writer.busy
        readonly property var _cands: _ll ? root.store.devices.filter(function (d) {
            var o = root.store.lifelineForDevice(d)
            return !o || o.id === llMenu.llId
        }) : []

        function _respondItem(text, value, isCurrent) {
            return { text: text, checked: isCurrent,
                     onTriggered: function () { root.store.setLifelineField(llMenu.llId, "respond", value) } }
        }

        groups: !_ll ? [] : [
            [ { text: _runs ? qsTr("■ Stop") : qsTr("▶ Run this head"), enabled: _runs || _canRun,
                onTriggered: function () { if (llMenu._runs) root.engine.stop(); else root.engine.start(llMenu.llId) } },
              { text: qsTr("Step"), enabled: _canRun, onTriggered: function () { root.engine.step(llMenu.llId) } },
              { text: qsTr("Add exchange to"), enabled: root.lls.length > 1,
                items: function () {
                    return [ root.lls.filter(function (x) { return x.id !== llMenu.llId }).map(function (x) {
                        return { text: x.label + (typeof x.address === "number" ? " · " + x.address : ""),
                                 onTriggered: function () { root.store.addExchange(llMenu.llId, x.id, "ranging") } }
                    }) ]
                } } ],
            [ { text: qsTr("Write"), visible: !_ext, enabled: _canWrite,
                onTriggered: function () { root.writer.writeLifeline(llMenu.llId) } },
              { text: qsTr("Reset to baseline"), visible: !_ext, enabled: _canWrite,
                onTriggered: function () { root.writer.resetToBaseline(llMenu.llId) } },
              { text: qsTr("Quick interrogate…"), visible: !_ext, enabled: !!_dev,
                onTriggered: function () { root.quickInterrogateRequested(root.store.deviceFor(llMenu.llId)) } },
              { text: qsTr("Bind to"), visible: !_ext, enabled: _cands.length > 0,
                items: function () {
                    return [ llMenu._cands.map(function (d) {
                        return { text: d.devName + " SN " + d.devSN + " · " + root.store.linkLabelOf(d),
                                 onTriggered: function () { root.store.bindLifeline(llMenu.llId, d) } }
                    }) ]
                } },
              { text: qsTr("Unbind"), visible: !_ext && !!_ll.binding,
                onTriggered: function () { root.store.unbindLifeline(llMenu.llId) } } ],
            [ { text: qsTr("Answers"), visible: !_ext,
                items: function () {
                    var r = llMenu._ll ? llMenu._ll.respond : "off"
                    return [ [ llMenu._respondItem(qsTr("Off"), "off", r === "off"),
                               llMenu._respondItem(qsTr("Always"), "always", r === "always"),
                               llMenu._respondItem(qsTr("Window"), typeof r === "object" ? r : { windowS: 600 }, typeof r === "object") ] ]
                } },
              { text: qsTr("In the cycle"), checked: _ll.active !== false,
                onTriggered: function () {
                    var l = Model.lifeline(root.store.st, llMenu.llId)
                    if (l) root.store.setLifelineField(llMenu.llId, "active", l.active === false)
                } },
              { text: qsTr("Move left"), enabled: _col > 0,
                onTriggered: function () { root.store.moveLifeline(llMenu.llId, llMenu._col - 1) } },
              { text: qsTr("Move right"), enabled: _col >= 0 && _col < root.lls.length - 1,
                onTriggered: function () { root.store.moveLifeline(llMenu.llId, llMenu._col + 1) } } ],
            [ { text: qsTr("Delete lifeline"), danger: true, onTriggered: function () { root.store.removeLifeline(llMenu.llId) } } ]
        ]
    }

    Canvas {
        id: rubber
        anchors.fill: parent
        visible: false
        z: 10
        property int fromId: -1
        property real x0: 0
        property real x1: 0
        property real y1: 0
        onPaint: {
            var ctx = getContext("2d")
            ctx.reset()
            if (!visible) return
            ctx.strokeStyle = AppPalette.accent
            ctx.lineWidth = 2
            ctx.beginPath()
            ctx.moveTo(x0, root.headerH)
            ctx.lineTo(x1, y1)
            ctx.stroke()
        }
        onVisibleChanged: requestPaint()
    }
}
