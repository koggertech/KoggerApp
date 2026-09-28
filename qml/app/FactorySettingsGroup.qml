import QtQuick 2.15
import QtQuick.Layouts 1.15
import kqml_types 1.0

// Dedicated factory panel (visible only in factory mode).
// UX: enter the access key once -> the device list fills -> pick a device + Start ->
// the stage line shows the main steps / high-level errors (core.flasherTextInfo). This is an
// operator tool: no low-level detail — the flashing logic lives in the private factory module.
// Backed by core (FLASHER build): flasherProducts / refreshFlasherProducts / setFlasherData /
// connectOpenedLinkAsFlasher / flasherTextInfo / flasherIdInfo.
SettingsGroup {
    id: factoryGroup

    property var store: null

    // Live device list (factory mode only).
    readonly property var _products: (typeof core !== "undefined" && core && core.isFactoryMode && core.flasherProducts)
                                      ? core.flasherProducts : []
    readonly property int _idx: deviceCombo.currentIndex
    readonly property string _selPn: (_idx >= 0 && _idx < _products.length) ? _products[_idx].pn : ""
    readonly property bool _selReady: (_idx >= 0 && _idx < _products.length) ? !!_products[_idx].ready : false
    readonly property bool _hasToken: (typeof core !== "undefined" && core && core.isFactoryMode) ? core.flasherHasToken : false

    // ── Progress rail state ───────────────────────────────────────────────
    // The step codes identify a step, they do NOT order one: 10 arrives near the end and 190
    // in the middle. _stageFor() is the single place that knows the running order, so the mapping
    // lives here rather than being inferred anywhere else. A code mapping to -1 leaves the current
    // stage alone: 115 (erase) legitimately occurs inside two different stages, and 45 is the
    // device-list refresh, which is not part of a flash at all.
    readonly property var _stageNames: [qsTr("Connect"), qsTr("Identify"), qsTr("Unlock"),
                                        qsTr("Test boot"), qsTr("Provision"), qsTr("Install firmware")]
    readonly property int _lastStage: _stageNames.length - 1

    property int stepCode: (typeof core !== "undefined" && core && core.isFactoryMode) ? core.flasherIdInfo : 0
    property string stepText: (typeof core !== "undefined" && core && core.isFactoryMode) ? core.flasherTextInfo : ""

    property bool _started: false
    property int  _stage: 0                 // index of the stage in progress
    property bool _failed: false
    property bool _complete: false
    property var  _elapsed: [0,0,0,0,0,0]   // ms per stage, frozen when that stage ends
    // A stage the run never entered is NOT a stage that succeeded. Unlock is skipped whenever the
    // board is already unlocked, and ticking it green would claim something that did not happen.
    property var  _entered: [true,false,false,false,false,false]
    property double _stageAt: 0             // Date.now() when the current stage opened
    property int  _tick: 0                  // drives the live clock on the active stage
    property int  _pct: -1                  // install %, -1 while indeterminate
    property double _lastMsgAt: 0           // Date.now() of the last state change from the flasher

    // Backstop, not the fix: a stalled run should be ended by the flashing layer reporting an
    // error code, and this only covers the case where none arrives -- which is what happened when
    // a board was unplugged mid-provision. The threshold sits well above the longest legitimate
    // silence in a run (a full-chip erase), so a slow erase never trips it.
    readonly property int _stallMs: 100000
    readonly property bool _stalled: _stalledNow()

    function _stalledNow() {
        if (!_started || _complete || _failed || _lastMsgAt <= 0)
            return false
        var unused = _tick                     // read so the binding re-evaluates on each tick
        return (Date.now() - _lastMsgAt) > _stallMs
    }

    function _stageFor(code) {
        switch (code) {
        case 80:  case 85:                                 return 0
        case 130:                                          return 1
        case 190: case 160:                                return 2
        case 120: case 170: case 150: case 10:             return 3
        // 40/50 are NOT listed: the same two fetch codes arrive in the test-boot phase and in
        // the product phase, so they cannot mark the boundary between them. Mapping them to
        // Provision left Test boot milliseconds after entering it and billed its erase, write and
        // UID read to Provision. 110 is emitted only in the product phase, so it is unambiguous.
        case 110: case 180: case 140:                      return 4
        case 105: case 100: case 15:                       return 5
        }
        return -1
    }

    function _resetRail() {
        _started = true; _failed = false; _complete = false
        _stage = 0; _pct = -1
        _elapsed = [0,0,0,0,0,0]
        _entered = [true,false,false,false,false,false]
        _stageAt = Date.now()
        _lastMsgAt = _stageAt
    }

    function _fmt(ms) {
        if (ms <= 0) return ""
        var sec = ms / 1000
        if (sec < 0.1) return qsTr("<0.1s")     // a real step, just a fast one -- never "0.0s"
        return (sec < 10 ? sec.toFixed(1) : Math.round(sec)) + qsTr("s")
    }

    function _liveMs(i) {
        if (i !== _stage || _complete || _failed)
            return _elapsed[i]
        var unused = _tick                         // read so the binding re-evaluates on each tick
        return Date.now() - _stageAt
    }

    // Freeze the running stage's duration. Copies the array rather than mutating in place:
    // assigning a var property its own reference back does not emit changed, so the rail would
    // only refresh when some *other* property happened to change alongside it.
    function _closeStage(at) {
        var e = _elapsed.slice()
        e[_stage] = at - _stageAt
        _elapsed = e
    }

    onStepCodeChanged: {
        if (!_started || stepCode === 0)
            return
        _lastMsgAt = Date.now()            // any state change proves the board is still there
        if (stepCode >= 500) {                     // any failure stops the rail where it stands
            _closeStage(Date.now())
            _failed = true
            return
        }
        if (stepCode === 100) {                    // "Installing firmware… 47%" is the only
            var m = /(\d+)\s*%/.exec(stepText)     // percentage the flasher reports at all
            if (m) _pct = parseInt(m[1])
        }
        var next = _stageFor(stepCode)
        if (next >= 0 && next > _stage) {          // never walk backwards
            var now = Date.now()
            _closeStage(now)
            var en = _entered.slice()
            en[next] = true
            _entered = en
            _stage = next
            _stageAt = now
        }
        if (stepCode === 15) {                     // terminal success
            _closeStage(Date.now())
            _complete = true
        }
    }

    // The only reason for a timer: a locked chip can sit silent in a full erase for around a
    // minute, with no step code in between. Without a moving clock the operator reasonably
    // concludes it has hung, and unplugs the board mid-erase.
    Timer {
        interval: 100; repeat: true
        running: factoryGroup._started && !factoryGroup._complete && !factoryGroup._failed
        onTriggered: factoryGroup._tick++
    }

    preferredWidth: width
    title: qsTr("Factory")
    description: qsTr("Factory setup for new devices.")
    stateStore: store
    stateKey: "app.factory"
    collapsedByDefault: false
    contentSpacing: Tokens.spaceMd

    Component.onCompleted: if (typeof core !== "undefined" && core && core.isFactoryMode) core.refreshFlasherProducts()

    // ── Access key ────────────────────────────────────────────────────────
    Row {
        spacing: Tokens.spaceSm
        Text { text: qsTr("Access key:"); color: AppPalette.textSecond; font.pixelSize: Tokens.fontBase }
        Text {
            visible: factoryGroup._hasToken
            anchors.verticalCenter: parent.verticalCenter
            text: qsTr("✓ saved"); color: "#10B981"; font.pixelSize: Tokens.fontXs
        }
    }

    Row {
        width: parent.width; height: Tokens.controlHMd; spacing: Tokens.spaceSm

        Rectangle {
            width: parent.width - setTokenBtn.width - parent.spacing
            height: Tokens.controlHMd; radius: Tokens.radiusMd; color: AppPalette.bg
            border.width: tokenField.activeFocus ? 1 : Tokens.cardBorderWidth
            border.color: tokenField.activeFocus ? AppPalette.accentBorder : AppPalette.border

            TextInput {
                id: tokenField
                activeFocusOnTab: true
                anchors.fill: parent; anchors.leftMargin: Tokens.spaceMd; anchors.rightMargin: Tokens.spaceMd
                verticalAlignment: TextInput.AlignVCenter
                color: AppPalette.text; font.pixelSize: Tokens.fontBase
                echoMode: TextInput.Password; clip: true
                TapHandler { acceptedButtons: Qt.LeftButton; onDoubleTapped: tokenField.selectAll() }

                Text {
                    visible: !tokenField.text.length
                    text: factoryGroup._hasToken ? qsTr("Key saved — paste to replace") : qsTr("Paste key…")
                    color: AppPalette.textMuted
                    font.pixelSize: Tokens.fontBase; anchors.verticalCenter: parent.verticalCenter
                }
            }
        }

        KButton {
            id: setTokenBtn
            width: Math.round(64 * AppPalette.scale); height: Tokens.controlHMd
            text: qsTr("Set")
            enabled: tokenField.text.length > 0
            onClicked: {
                core.setFlasherData(tokenField.text)
                tokenField.text = ""
                core.refreshFlasherProducts()
            }
        }
    }

    // ── Device selection ──────────────────────────────────────────────────
    Text { text: qsTr("Device:"); color: AppPalette.textSecond; font.pixelSize: Tokens.fontBase }

    RowLayout {
        width: parent.width; spacing: Tokens.spaceSm

        KCombo {
            id: deviceCombo
            Layout.fillWidth: true
            Layout.preferredHeight: Tokens.controlHMd
            model: factoryGroup._products.map(function(p) { return p.pn })
        }

        KCircleIconButton {
            width: Tokens.controlHMd; height: Tokens.controlHMd
            cornerRadius: Tokens.radiusMd; borderWidth: 0; scaleOnHover: false
            iconSource: "qrc:/icons/ui/refresh.svg"
            iconPixelSize: Math.round(Tokens.controlHMd * 0.5)
            iconTintColor: AppPalette.text
            toolTipText: qsTr("Refresh device list")
            fillColor: AppPalette.chipRaised; fillHoverColor: AppPalette.chipRaisedHover
            onClicked: if (typeof core !== "undefined" && core) core.refreshFlasherProducts()
        }
    }

    Text {
        visible: factoryGroup._selPn.length > 0 && !factoryGroup._selReady
        width: parent.width; wrapMode: Text.WordWrap
        text: qsTr("Selected device is not ready.")
        color: "#F59E0B"; font.pixelSize: Tokens.fontXs
    }

    // ── Start ─────────────────────────────────────────────────────────────
    // Deliberately ONE label, always. There is no "retry": the operator may have swapped the
    // board, power-cycled it, or changed the PN between attempts, and every run re-reads the chip
    // UID from scratch anyway. A button that says "Retry" invites the belief that the previous
    // unit is being resumed. Every press starts from zero.
    KButton {
        width: parent.width; height: Tokens.controlHMd
        text: qsTr("Start flashing")
        enabled: factoryGroup._selPn.length > 0 && factoryGroup._selReady
        onClicked: {
            if (typeof core === "undefined" || !core) return
            factoryGroup._resetRail()
            core.connectOpenedLinkAsFlasher(factoryGroup._selPn)
        }
    }

    // ── Progress rail ─────────────────────────────────────────────────────
    // Six stages, always all six. Completed ones collapse to a tick and their duration; the
    // active one carries the live message; a failure reddens its own row and leaves the rest
    // visible, so "which part broke" is answerable without reading the sentence.
    Column {
        id: rail
        width: parent.width
        spacing: 0
        visible: factoryGroup._started

        Repeater {
            model: factoryGroup._stageNames

            delegate: Item {
                id: row
                width: rail.width

                readonly property bool wasEntered: !!factoryGroup._entered[index]
                readonly property bool isPast:   factoryGroup._complete || index < factoryGroup._stage
                readonly property bool isDone:   isPast && wasEntered
                readonly property bool isSkipped: isPast && !wasEntered
                readonly property bool isActive: index === factoryGroup._stage
                                                 && !factoryGroup._complete && !factoryGroup._failed
                readonly property bool isFailed: index === factoryGroup._stage && factoryGroup._failed
                readonly property color tone: isFailed ? "#EF4444"
                                            : (isDone ? "#10B981"
                                            : (isActive ? AppPalette.accent : AppPalette.border))

                implicitHeight: head.height + (sub.visible ? sub.implicitHeight + Tokens.spaceXxs : 0)
                                + Tokens.spaceSm

                // Connector: drawn from this dot to the next row's dot.
                Rectangle {
                    x: dot.x + dot.width / 2 - width / 2
                    y: dot.y + dot.height
                    width: Math.max(1, Math.round(AppPalette.scale))
                    height: row.height - y
                    visible: index < factoryGroup._lastStage
                    color: row.isDone ? "#10B981" : AppPalette.border
                    opacity: row.isDone ? 0.45 : 1.0
                }

                Item {
                    id: head
                    width: parent.width
                    height: Math.max(nameText.implicitHeight, timeText.implicitHeight)

                    Rectangle {
                        id: dot
                        width: Math.round(10 * AppPalette.scale); height: width; radius: width / 2
                        anchors.verticalCenter: parent.verticalCenter
                        color: (row.isDone || row.isActive || row.isFailed) ? row.tone : "transparent"
                        border.width: Math.max(1, Math.round(1.5 * AppPalette.scale))
                        border.color: row.tone
                        opacity: row.isSkipped ? 0.4 : 1.0
                    }

                    Text {
                        id: nameText
                        anchors.left: dot.right; anchors.leftMargin: Tokens.spaceMd
                        anchors.right: timeText.left; anchors.rightMargin: Tokens.spaceSm
                        anchors.verticalCenter: parent.verticalCenter
                        elide: Text.ElideRight
                        text: modelData
                        font.pixelSize: Tokens.fontBase
                        font.bold: row.isActive || row.isFailed
                        color: row.isFailed ? "#EF4444"
                             : (row.isActive ? AppPalette.text : AppPalette.textMuted)
                        opacity: row.isSkipped ? 0.45 : ((row.isDone || row.isActive || row.isFailed) ? 1.0 : 0.45)
                    }

                    Text {
                        id: timeText
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        text: {
                            if (row.isSkipped) return qsTr("skipped")
                            if (row.isActive && index === factoryGroup._lastStage && factoryGroup._pct >= 0)
                                return factoryGroup._pct + "%"
                            return factoryGroup._fmt(factoryGroup._liveMs(index))
                        }
                        font.pixelSize: Tokens.fontSm
                        color: AppPalette.textMuted
                        opacity: 0.75
                    }
                }

                // Live message, only under the stage it belongs to.
                Text {
                    id: sub
                    anchors.top: head.bottom; anchors.topMargin: Tokens.spaceXxs
                    anchors.left: parent.left
                    anchors.leftMargin: dot.width + Tokens.spaceMd
                    anchors.right: parent.right
                    visible: (row.isActive || row.isFailed) && text.length > 0
                    wrapMode: Text.WordWrap
                    text: factoryGroup.stepText
                    font.pixelSize: Tokens.fontSm
                    lineHeight: 1.25
                    color: row.isFailed ? "#EF4444" : AppPalette.textMuted
                }
            }
        }
    }

    // Determinate only where a real percentage exists -- the install stage. Everything earlier is
    // indeterminate, and a bar that interpolated across stages would be inventing five-sixths.
    Rectangle {
        width: parent.width
        height: Math.round(4 * AppPalette.scale)
        radius: height / 2
        color: AppPalette.border
        visible: factoryGroup._started && factoryGroup._pct >= 0 && !factoryGroup._failed

        Rectangle {
            width: parent.width * Math.min(1, Math.max(0, factoryGroup._pct / 100))
            height: parent.height; radius: parent.radius
            color: factoryGroup._complete ? "#10B981" : AppPalette.accent
            Behavior on width { NumberAnimation { duration: 180 } }
        }
    }

    // Silence long enough that something is wrong, but no error arrived. Says what to check
    // rather than just reporting the silence.
    Text {
        width: parent.width; wrapMode: Text.WordWrap
        visible: factoryGroup._stalled
        text: qsTr("No response for over %1s — the board may have been disconnected. "
                   + "Press Start to begin again.").arg(Math.round(factoryGroup._stallMs / 1000))
        color: "#F59E0B"; font.pixelSize: Tokens.fontSm
    }

    // Terminal states get an outcome line: the run is over, say what it produced.
    Text {
        width: parent.width; wrapMode: Text.WordWrap
        visible: factoryGroup._complete
        text: qsTr("✓ Provisioned and installed — %1").arg(factoryGroup._selPn)
        color: "#10B981"; font.pixelSize: Tokens.fontSm
    }
}
