import QtQuick 2.15
import kqml_types 1.0
import "NodesModel.js" as Model

DeviceSettingsGroup {
    id: card

    property var dev: null
    property var store: null
    property var network: null
    property var engine: null

    title: qsTr("Acoustic network")
    titlePixelSize: 13
    stateKey: "dev.usblNetwork"
    collapsedByDefault: false
    visible: !!(dev && (dev.isUSBL || dev.isUSBLBeacon) && network)

    readonly property var _ll: network && dev ? network.lifelineForDevice(dev) : null
    readonly property bool _isHead: !!_ll && !!network && Model.isInitiator(network.st, _ll.id)
    readonly property int _head: network ? network.headFor(_ll ? _ll.id : -1) : -1

    function _stepTo(lifelineId) {
        var rs = engine ? engine.runnable : []
        for (var i = 0; i < rs.length; ++i)
            if (rs[i].targetId === lifelineId && (_head < 0 || rs[i].initiatorId === _head)) return rs[i]
        return null
    }

    component Btn: UsblButton {
        implicitHeight: Tokens.controlHMd
        fontPixelSize: Tokens.fontSm
    }
    component Tool: KToolIconButton {
        toolTipOnHold: true
    }

    headerActions: Row {
        spacing: Tokens.spaceSm
        rightPadding: Tokens.spaceSm
        Text {
            visible: !!card.engine && card.engine.running
            text: qsTr("running")
            color: AppPalette.accentText
            font.pixelSize: Tokens.fontSm; font.bold: true
            anchors.verticalCenter: parent.verticalCenter
        }
    }

    Text {
        width: parent.width
        wrapMode: Text.WordWrap
        text: card.network ? qsTr("%1%2 · %3 lifelines · cycle %4 s")
                             .arg(card.network.displayName).arg(card.network.dirty ? " ●" : "")
                             .arg(card.network.lifelines.length)
                             .arg((card.network.compiled.totalMs / 1000).toFixed(2)) : ""
        color: AppPalette.textStrong
        font.pixelSize: Tokens.fontSm; font.bold: true
    }
    Text {
        width: parent.width
        wrapMode: Text.WordWrap
        text: card._ll ? qsTr("This device is “%1” — %2").arg(card._ll.label).arg(card.network.writeStateText(card.network.writeStateOf(card._ll.id)))
                       : qsTr("This device is not part of the network.")
        color: AppPalette.textSecond
        font.pixelSize: Tokens.fontSm
    }

    Row {
        spacing: Tokens.spaceSm
        Tool {
            readonly property bool _on: !!card.engine && card.engine.running
            iconSource: _on ? "qrc:/icons/ui/player-stop.svg" : "qrc:/icons/ui/player-play.svg"
            armed: _on
            enabled: !!card.engine && (card.engine.running || card.engine.canRun)
            toolTipText: _on ? qsTr("Stop the cycle") : qsTr("Start the cycle")
            anchors.verticalCenter: parent.verticalCenter
            onClicked: card.engine.running ? card.engine.stop() : card.engine.start(card._isHead ? card._ll.id : -1)
        }
        Tool {
            iconSource: "qrc:/icons/ui/step-into.svg"
            enabled: !!card.engine && card.engine.canRun
            toolTipText: qsTr("Step: send the next interrogation and stop there")
            anchors.verticalCenter: parent.verticalCenter
            onClicked: card.engine.step(card._isHead ? card._ll.id : -1)
        }
        KIcon {
            source: "qrc:/icons/ui/ruler_measure.svg"
            toolTipText: qsTr("Max range, m")
            anchors.verticalCenter: parent.verticalCenter
        }
        UsblSpin {
            from: Model.MIN_RANGE_M; to: Model.MAX_RANGE_M; stepSize: 10
            devValue: card.network ? card.network.st.maxRangeM : 300
            implicitWidth: Math.round(90 * AppPalette.scale)
            fontPixelSize: Tokens.fontSm
            anchors.verticalCenter: parent.verticalCenter
            writeBack: function (v) { card.network.setNetworkField("maxRangeM", v) }
        }
        Text { text: qsTr("m"); color: AppPalette.textSecond; font.pixelSize: Tokens.fontSm; anchors.verticalCenter: parent.verticalCenter }
    }
    Row {
        spacing: Tokens.spaceSm
        KIcon {
            source: "qrc:/icons/ui/repeat.svg"
            toolTipText: qsTr("Period between steps, s")
            anchors.verticalCenter: parent.verticalCenter
        }
        KSpinBox {
            from: card.network ? card.network.periodFloorMs : Model.DEFAULT_PERIOD_MS
            to: Model.MAX_PERIOD_MS; stepSize: 100
            divisor: 1000; decimals: 1
            value: card.network ? Math.max(card.network.st.cycle.periodMs, from) : Model.DEFAULT_PERIOD_MS
            opacity: card.network && card.network.periodMode ? 1.0 : 0.55
            implicitWidth: Math.round(96 * AppPalette.scale)
            anchors.verticalCenter: parent.verticalCenter
            toolTipText: qsTr("Interval between steps, as Dwell was. It cannot go below %1 s: the longest step of the cycle needs that long. Setting it switches the cycle to this period.")
                         .arg(((card.network ? card.network.periodFloorMs : 0) / 1000).toFixed(2))
            onValueModified: function (v) { if (card.network) card.network.setPeriodMs(v) }
        }
        Text { text: qsTr("s between steps"); color: AppPalette.textSecond; font.pixelSize: Tokens.fontSm; anchors.verticalCenter: parent.verticalCenter }
        Tool {
            iconSource: "qrc:/icons/ui/wand.svg"
            armed: !!card.network && !card.network.periodMode
            toolTipText: qsTr("Auto: each step as long as its own exchange needs; the period is not used")
            anchors.verticalCenter: parent.verticalCenter
            onClicked: card.network.setNetworkField("stepMode", "auto")
        }
    }

    Repeater {
        model: card.network ? card.network.answeringTargets : []
        delegate: Row {
            id: trow
            required property var modelData
            readonly property bool _on: modelData.active !== false
            readonly property var _step: card._stepTo(modelData.id)
            readonly property real _m: card.network ? card.network.measuredRangeOf(modelData.id) : NaN
            spacing: Tokens.spaceSm
            opacity: _on ? 1.0 : 0.6
            UsblAddressBadge {
                address: trow.modelData.address
                diameter: Math.round(18 * AppPalette.scale)
                anchors.verticalCenter: parent.verticalCenter
            }
            Text {
                text: trow.modelData.label
                width: Math.round(110 * AppPalette.scale)
                elide: Text.ElideRight
                color: AppPalette.textStrong
                font.pixelSize: Tokens.fontSm; font.bold: true
                anchors.verticalCenter: parent.verticalCenter
            }
            KSwitch {
                flat: true
                checked: trow._on
                anchors.verticalCenter: parent.verticalCenter
                onToggled: {
                    card.network.setLifelineField(trow.modelData.id, "active", checked)
                    checked = Qt.binding(function () { return trow._on })
                }
            }
            Text {
                text: !trow._on ? qsTr("skipped")
                     : isNaN(trow._m) ? "—" : qsTr("%1 m").arg(trow._m.toFixed(1))
                width: Math.round(64 * AppPalette.scale)
                color: AppPalette.textSecond
                font.pixelSize: Tokens.fontSm
                anchors.verticalCenter: parent.verticalCenter
            }
            Tool {
                iconSource: "qrc:/icons/ui/speakerphone.svg"
                enabled: !!trow._step
                anchors.verticalCenter: parent.verticalCenter
                toolTipText: trow._step ? qsTr("Interrogate this beacon once") : qsTr("No connected head asks this beacon: add an exchange to it in the Nodes-Configurator")
                onClicked: card.engine.interrogateNow(trow._step.stepId)
            }
        }
    }

    Row {
        spacing: Tokens.spaceSm
        Btn {
            text: qsTr("Open Nodes-Configurator")
            iconSource: "qrc:/icons/ui/topology-ring-3.svg"
            onClicked: if (card.store) card.store.nodesConfiguratorOpen = true
        }
        Btn {
            visible: !card._ll
            text: qsTr("Interrogate from this device")
            iconSource: "qrc:/icons/ui/speakerphone.svg"
            toolTipText: qsTr("Adds this device as a head ranging address 1 and starts it")
            onClicked: {
                var head = card.network.quickInterrogate(card.dev, [1], card.network.st.maxRangeM)
                if (head >= 0) card.engine.start(head)
                if (card.store) card.store.nodesConfiguratorOpen = true
            }
        }
    }
}
