import QtQuick 2.15
import QtQuick.Controls 2.15
import kqml_types 1.0
import "NodesModel.js" as Model
import "NodesCompile.js" as Compile

Item {
    id: root

    property var store: null
    property var writer: null
    property string tab: "problems"

    component Btn: UsblButton {
        implicitHeight: Tokens.controlHMd
        fontPixelSize: Tokens.fontSm
    }

    Row {
        id: tabs
        x: Tokens.spaceSm
        height: Tokens.controlHMd
        spacing: Tokens.spaceSm
        KTabBar {
            buttonHeight: Tokens.controlHSm; fontPixelSize: Tokens.fontXs
            anchors.verticalCenter: parent.verticalCenter
            options: [
                { label: root.store ? qsTr("Problems %1").arg(root.store.problemCounts.error + root.store.problemCounts.warning) : qsTr("Problems"), value: "problems" },
                { label: qsTr("Write"), value: "write" },
                { label: qsTr("Frames"), value: "frames" },
                { label: qsTr("Slots"), value: "slots" }
            ]
            currentValue: root.tab
            onValueSelected: function (v) { root.tab = v }
        }
        Text {
            visible: root.tab === "write" && !!root.writer && root.writer.busy
            text: root.writer ? qsTr("writing %1/%2").arg(root.writer.progressDone).arg(root.writer.progressTotal) : ""
            color: AppPalette.accentText; font.pixelSize: Tokens.fontXs; font.bold: true
            anchors.verticalCenter: parent.verticalCenter
        }
    }

    Flickable {
        id: flick
        anchors.top: tabs.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: Tokens.spaceXs
        contentWidth: Math.max(width, body.implicitWidth)
        contentHeight: body.implicitHeight
        clip: true
        boundsBehavior: Flickable.StopAtBounds

        Column {
            id: body
            width: flick.width
            spacing: Tokens.spaceXxs

            // problems
            Repeater {
                model: root.tab === "problems" && root.store ? root.store.allProblems : []
                delegate: Rectangle {
                    id: prow
                    required property var modelData
                    width: body.width
                    implicitHeight: Math.max(Tokens.controlHMd, ptext.implicitHeight + Tokens.spaceXs * 2)
                    radius: Tokens.radiusSm
                    color: modelData.sev === "error" ? AppPalette.dangerBg : modelData.sev === "warning" ? AppPalette.linkIdleBg : "transparent"
                    Text {
                        id: picon
                        x: Tokens.spaceSm
                        anchors.verticalCenter: parent.verticalCenter
                        text: prow.modelData.sev === "error" ? "✕" : prow.modelData.sev === "warning" ? "!" : "i"
                        color: prow.modelData.sev === "error" ? AppPalette.dangerText : prow.modelData.sev === "warning" ? AppPalette.linkIdleText : AppPalette.textMuted
                        font.pixelSize: Tokens.fontSm; font.bold: true
                        width: Math.round(16 * AppPalette.scale)
                    }
                    Text {
                        id: ptext
                        anchors.left: picon.right
                        anchors.right: fixBtn.left
                        anchors.rightMargin: Tokens.spaceSm
                        anchors.verticalCenter: parent.verticalCenter
                        wrapMode: Text.WordWrap
                        text: root.store.problemText(prow.modelData)
                        color: AppPalette.text
                        font.pixelSize: Tokens.fontXs
                    }
                    MouseArea {
                        anchors.left: parent.left
                        anchors.right: fixBtn.left
                        height: parent.height
                        onClicked: {
                            var p = prow.modelData
                            if (p.step >= 0) root.store.select("step", p.step)
                            else if (p.exchange >= 0) {
                                var s = root.store.st.cycle.steps.filter(function (x) { return x.exchange === p.exchange })
                                if (s.length) root.store.select("step", s[0].id); else root.store.select("exchange", p.exchange)
                            }
                            else if (p.lifeline >= 0) root.store.select("lifeline", p.lifeline)
                            else root.store.select("network", -1)
                        }
                    }
                    Btn {
                        id: fixBtn
                        anchors.right: parent.right
                        anchors.rightMargin: Tokens.spaceXs
                        anchors.verticalCenter: parent.verticalCenter
                        visible: text.length > 0
                        width: visible ? implicitWidth : 0
                        text: root.store.fixLabel(prow.modelData)
                        onClicked: root.store.applyFix(prow.modelData)
                    }
                }
            }
            Text {
                visible: root.tab === "problems" && !!root.store && !root.store.allProblems.length
                text: qsTr("No problems: every device will understand the others.")
                color: AppPalette.linkOkText; font.pixelSize: Tokens.fontXs
                padding: Tokens.spaceSm
            }

            // write
            Row {
                visible: root.tab === "write"
                spacing: Tokens.spaceSm
                Btn { text: qsTr("Write changed"); enabled: !!root.writer && !root.writer.busy; onClicked: root.writer.writeChanged() }
                Btn { text: qsTr("Write all connected"); enabled: !!root.writer && !root.writer.busy; onClicked: root.writer.writeAllConnected() }
                Btn { text: qsTr("Cancel"); visible: !!root.writer && root.writer.busy; onClicked: root.writer.cancel() }
            }
            Repeater {
                model: root.tab === "write" && root.store ? root.store.lifelines.filter(function (l) { return l.kind !== "external" }) : []
                delegate: Row {
                    id: wrow
                    required property var modelData
                    readonly property var _dev: root.store.deviceFor(modelData.id)
                    readonly property string _ws: root.store.writeStateOf(modelData.id)
                    readonly property var _d: root.store.compiled.devices[modelData.id]
                    spacing: Tokens.spaceSm
                    height: Tokens.controlHMd
                    Text {
                        text: wrow.modelData.label
                        width: Math.round(120 * AppPalette.scale); elide: Text.ElideRight
                        color: AppPalette.textStrong; font.pixelSize: Tokens.fontXs; font.bold: true
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Text {
                        text: wrow._d ? wrow._d.fingerprint.substring(0, 4) + " · " + qsTr("%1 slots configured").arg(wrow._d.configuredSlots) : ""
                        width: Math.round(150 * AppPalette.scale); elide: Text.ElideRight
                        color: AppPalette.textMuted; font.pixelSize: Tokens.fontXs; font.family: "monospace"
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Text {
                        text: {
                            var f = root.store.writeFailureText(wrow.modelData.id)
                            return root.store.writeStateText(wrow._ws) + (f.length ? " — " + f : "")
                        }
                        width: Math.max(Math.round(160 * AppPalette.scale), body.width - Math.round(520 * AppPalette.scale))
                        elide: Text.ElideRight
                        color: wrow._ws === "confirmed" ? AppPalette.linkOkText
                             : (wrow._ws === "rejected" || wrow._ws === "noAnswer" || wrow._ws === "partial") ? AppPalette.dangerText
                             : AppPalette.textSecond
                        font.pixelSize: Tokens.fontXs
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Btn { text: qsTr("Write"); enabled: !!wrow._dev && !!root.writer && !root.writer.busy; anchors.verticalCenter: parent.verticalCenter; onClicked: root.writer.writeLifeline(wrow.modelData.id) }
                    Btn { text: qsTr("Baseline"); enabled: !!wrow._dev && !!root.writer && !root.writer.busy; anchors.verticalCenter: parent.verticalCenter; onClicked: root.writer.resetToBaseline(wrow.modelData.id) }
                }
            }
            Text {
                visible: root.tab === "write" && !!root.writer && root.writer.lastLog.length > 0
                width: body.width
                wrapMode: Text.WordWrap
                text: root.writer ? qsTr("Last write: ") + root.writer.lastLog.map(function (e) {
                    return "#" + (e.index + 1) + " " + (e.kind === "v6" ? "v6 cmd " + e.cmd + (e.event === 2 ? " resp" : " req") : e.kind)
                         + (e.attempt > 1 ? " (try " + e.attempt + ")" : "")
                         + (e.code === 1 ? " ok" : (e.code === 0 ? " no answer" : " code " + e.code))
                         + " " + e.ms + "ms"
                }).join(" · ") : ""
                color: AppPalette.textMuted; font.pixelSize: Tokens.fontXxs; font.family: "monospace"
            }

            // frames
            Text {
                visible: root.tab === "frames"
                text: root.store && root.store.selLifeline ? qsTr("Frames written to %1, in order:").arg(root.store.selLifeline.label)
                                                           : qsTr("Select a lifeline to see the exact frames it is written with.")
                color: AppPalette.textMuted; font.pixelSize: Tokens.fontXs
            }
            Repeater {
                model: root.tab === "frames" && root.store && root.store.selLifeline
                       ? Compile.framesView(root.store.compiled.devices[root.store.selLifeline.id]) : []
                delegate: Text {
                    required property var modelData
                    text: (modelData.index + 1) + ". V" + modelData.ver + "  " + modelData.text
                    color: modelData.configured ? AppPalette.text : AppPalette.textMuted
                    font.pixelSize: Tokens.fontXxs; font.family: "monospace"
                }
            }

            // slots
            Grid {
                visible: root.tab === "slots"
                columns: 17
                columnSpacing: Tokens.spaceXxs
                rowSpacing: Tokens.spaceXxs
                Text { text: ""; width: Math.round(110 * AppPalette.scale) }
                Repeater {
                    model: 16
                    delegate: Text {
                        required property int index
                        text: (index < 8 ? "req " : "resp ") + (index % 8)
                        width: Math.round(64 * AppPalette.scale)
                        color: AppPalette.textMuted; font.pixelSize: Tokens.fontXxs; font.bold: true
                    }
                }
                Repeater {
                    model: root.tab === "slots" && root.store ? root.store.lifelines.filter(function (l) { return l.kind !== "external" }) : []
                    delegate: Repeater {
                        id: srow
                        required property var modelData
                        readonly property var _d: root.store.compiled.devices[modelData.id]
                        model: 17
                        delegate: Rectangle {
                            required property int index
                            readonly property var _slot: index > 0 && srow._d ? srow._d.slots[index - 1] : null
                            width: index === 0 ? Math.round(110 * AppPalette.scale) : Math.round(64 * AppPalette.scale)
                            height: Math.round(30 * AppPalette.scale)
                            radius: Tokens.radiusSm
                            color: index === 0 ? "transparent" : (_slot && _slot.configured ? AppPalette.accentBg : AppPalette.card)
                            border.width: index === 0 ? 0 : 1
                            border.color: AppPalette.border
                            Text {
                                anchors.fill: parent
                                anchors.margins: 2
                                verticalAlignment: Text.AlignVCenter
                                horizontalAlignment: index === 0 ? Text.AlignLeft : Text.AlignHCenter
                                elide: Text.ElideRight
                                wrapMode: Text.NoWrap
                                text: {
                                    if (index === 0) return srow.modelData.label
                                    var s = parent._slot
                                    if (!s || !s.configured) return "·"
                                    var t = []
                                    if (s.recvFn) t.push("r" + (s.recvFn === 2 ? "LL" : s.recvBits))
                                    if (s.sendFn) t.push("s" + (s.sendFn === 2 ? "LL" : s.sendBits))
                                    if (s.cmdIdAction) t.push("→" + s.cmdIdRepl)
                                    if (s.addrAction) t.push("@" + s.addrRepl)
                                    return t.length ? t.join(" ") : "✓"
                                }
                                color: index === 0 ? AppPalette.textStrong : AppPalette.text
                                font.pixelSize: Tokens.fontXxs; font.bold: index === 0
                            }
                        }
                    }
                }
            }
            Text {
                visible: root.tab === "slots"
                text: qsTr("r = receives bits, s = sends bits, LL = position + azimuth, → = reply cmd, @ = reply address. Empty slots are written with defaults.")
                color: AppPalette.textMuted; font.pixelSize: Tokens.fontXxs
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
