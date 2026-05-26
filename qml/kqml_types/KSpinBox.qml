import QtQuick 2.15
import QtQuick.Controls 2.15
import kqml_types 1.0

Item {
    id: root

    property int value: 0
    property int from: 0
    property int to: 100
    property int stepSize: 1
    property real divisor: 1.0
    property int decimals: 0
    property bool editable: true

    signal valueModified(int val)

    implicitWidth: Math.round(115 * AppPalette.scale)
    implicitHeight: Tokens.controlHMd

    readonly property int _btnW: Math.round(26 * AppPalette.scale)
    readonly property bool _focused: input.activeFocus

    // Guard: while we programmatically set input.text we must NOT re-parse it
    // (would re-enter onTextChanged and cause feedback loops).
    property bool _settingText: false

    function clamp(v) { return Math.max(root.from, Math.min(root.to, v)) }

    function applyValue(v) {
        var c = clamp(Math.round(v))
        if (c !== root.value) {
            root.value = c
            root.valueModified(c)
        }
    }

    function displayedText() { return (root.value / root.divisor).toFixed(root.decimals) }

    function _setText(t) {
        _settingText = true
        input.text = t
        _settingText = false
    }

    // Parse what's currently typed and push it to root.value (no text rewrite).
    function _parseAndApply() {
        var raw = input.text.replace(",", ".").trim()
        if (raw === "" || raw === "-" || raw === ".") return
        var parsed = parseFloat(raw)
        if (isNaN(parsed)) return
        applyValue(parsed * root.divisor)
    }

    // Final commit: parse, apply, and re-format display to canonical form.
    function commitTyped() {
        _parseAndApply()
        _setText(displayedText())
    }

    function increment() {
        if (input.activeFocus) _parseAndApply()
        applyValue(root.value + root.stepSize)
        if (input.activeFocus) {
            _setText(displayedText())
            input.cursorPosition = input.text.length
        }
    }

    function decrement() {
        if (input.activeFocus) _parseAndApply()
        applyValue(root.value - root.stepSize)
        if (input.activeFocus) {
            _setText(displayedText())
            input.cursorPosition = input.text.length
        }
    }

    // External value changes (writes from device / from sibling controls) —
    // refresh the display only when user is not actively editing.
    onValueChanged:    if (!_focused) _setText(displayedText())
    onDivisorChanged:  if (!_focused) _setText(displayedText())
    onDecimalsChanged: if (!_focused) _setText(displayedText())

    Component.onCompleted: _setText(displayedText())

    Rectangle {
        id: frame
        anchors.fill: parent
        radius: Tokens.radiusMd
        color: AppPalette.bg
        border.width: 1
        border.color: input.activeFocus
                      ? AppPalette.accentBorder
                      : ((minusMouse.containsMouse || plusMouse.containsMouse) ? AppPalette.borderHover : AppPalette.border)

        // ── minus button background (inset to stay inside rounded border) ──
        Rectangle {
            id: minusBg
            x: 1; y: 1
            width: root._btnW - 1
            height: parent.height - 2
            radius: Math.max(0, Tokens.radiusMd - 1)
            color: minusMouse.pressed
                   ? AppPalette.bgDeep
                   : (minusMouse.containsMouse ? AppPalette.card : "transparent")
            Behavior on color { ColorAnimation { duration: 60 } }
        }

        // Минус — горизонтальная палочка. Толщина и пропорции подобраны
        // под Tabler-stroke (~3 px при scale=1), концы скруглены.
        Rectangle {
            anchors.centerIn: minusBg
            width: Math.round(14 * AppPalette.scale)
            height: Math.max(3, Math.round(3 * AppPalette.scale))
            radius: height / 2
            color: AppPalette.text
            opacity: root.value > root.from ? 0.85 : 0.3
        }

        // ── plus button background ──
        Rectangle {
            id: plusBg
            x: parent.width - root._btnW
            y: 1
            width: root._btnW - 1
            height: parent.height - 2
            radius: Math.max(0, Tokens.radiusMd - 1)
            color: plusMouse.pressed
                   ? AppPalette.bgDeep
                   : (plusMouse.containsMouse ? AppPalette.card : "transparent")
            Behavior on color { ColorAnimation { duration: 60 } }
        }

        // Плюс — две палочки крест-накрест с такой же толщиной/округлением.
        Item {
            anchors.centerIn: plusBg
            width: Math.round(14 * AppPalette.scale)
            height: Math.round(14 * AppPalette.scale)
            opacity: root.value < root.to ? 0.85 : 0.3

            Rectangle {
                anchors.centerIn: parent
                width: parent.width
                height: Math.max(3, Math.round(3 * AppPalette.scale))
                radius: height / 2
                color: AppPalette.text
            }
            Rectangle {
                anchors.centerIn: parent
                width: Math.max(3, Math.round(3 * AppPalette.scale))
                height: parent.height
                radius: width / 2
                color: AppPalette.text
            }
        }

        // ── editable value ──
        TextInput {
            id: input
            x: root._btnW
            y: 0
            width: parent.width - 2 * root._btnW
            height: parent.height
            horizontalAlignment: TextInput.AlignHCenter
            verticalAlignment: TextInput.AlignVCenter
            color: AppPalette.text
            font.pixelSize: Tokens.fontSm
            selectByMouse: true
            selectionColor: AppPalette.accentBg
            readOnly: !root.editable
            clip: true
            inputMethodHints: Qt.ImhFormattedNumbersOnly
            validator: DoubleValidator {
                bottom: root.from / root.divisor
                top:    root.to   / root.divisor
                decimals: root.decimals
                notation: DoubleValidator.StandardNotation
                locale: "C"
            }

            // Apply value live as user types (no need to press Enter).
            onTextChanged: {
                if (root._settingText) return     // ignore our own programmatic writes
                if (!activeFocus) return          // ignore external rewrites
                root._parseAndApply()
            }

            // No selectAll() on focus — cursor lands wherever the user clicked.
            onActiveFocusChanged: {
                if (!activeFocus) root.commitTyped()
            }

            Keys.onReturnPressed:  { root.commitTyped(); focus = false }
            Keys.onEnterPressed:   { root.commitTyped(); focus = false }
            Keys.onEscapePressed:  { root._setText(root.displayedText()); focus = false }
            Keys.onUpPressed:      root.increment()
            Keys.onDownPressed:    root.decrement()
        }

        // ── interactive overlays on top of bg rectangles ──
        MouseArea {
            id: minusMouse
            x: 0; y: 0
            width: root._btnW; height: parent.height
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            enabled: root.value > root.from
            onClicked: root.decrement()
            onPressAndHold: holdDec.start()
            onReleased: holdDec.stop()
            onCanceled: holdDec.stop()
        }
        Timer { id: holdDec; interval: 80; repeat: true; onTriggered: root.decrement() }

        MouseArea {
            id: plusMouse
            x: parent.width - root._btnW
            y: 0
            width: root._btnW; height: parent.height
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            enabled: root.value < root.to
            onClicked: root.increment()
            onPressAndHold: holdInc.start()
            onReleased: holdInc.stop()
            onCanceled: holdInc.stop()
        }
        Timer { id: holdInc; interval: 80; repeat: true; onTriggered: root.increment() }
    }
}
