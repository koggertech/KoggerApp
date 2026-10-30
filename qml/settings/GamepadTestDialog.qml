import QtQuick 2.15
import QtQuick.Controls 2.15
import kqml_types 1.0

Popup {
    id: dialog

    property var store: null

    readonly property bool hasMonitor: typeof gamepadMonitor !== "undefined" && gamepadMonitor !== null
    readonly property bool supported: hasMonitor && gamepadMonitor.supported
    readonly property bool connected: hasMonitor && gamepadMonitor.connected
    readonly property string family: connected ? gamepadMonitor.family : ""
    readonly property var faceLabels: connected ? gamepadMonitor.faceLabels : []
    readonly property bool playStation: family === "ps4" || family === "ps5"
    readonly property int guideButton: 5
    readonly property int buttonCount: 21
    readonly property int buttons: hasMonitor ? (gamepadMonitor.buttons & ~(1 << guideButton)) : 0
    readonly property int availableButtons: hasMonitor ? gamepadMonitor.availableButtons : 0

    readonly property var pressedItems: {
        var items = []
        for (var i = 0; i < buttonCount; ++i) {
            if ((buttons & (1 << i)) !== 0)
                items.push({ symbol: buttonSymbol(i), text: buttonName(i) })
        }
        return items
    }

    readonly property string familyName: {
        switch (family) {
            case "xbox":  return "Xbox"
            case "ps4":   return "DualShock 4"
            case "ps5":   return "DualSense"
            case "steam": return "Steam"
            default:      return qsTr("Other gamepad")
        }
    }

    readonly property string emptyTitle: {
        if (!supported)
            return qsTr("Gamepad support is not built into this version")
        if (hasMonitor && gamepadMonitor.error.length)
            return qsTr("Gamepad subsystem failed: %1").arg(gamepadMonitor.error)
        return qsTr("No gamepad connected")
    }

    readonly property string emptyHint: {
        if (!supported || (hasMonitor && gamepadMonitor.error.length))
            return ""
        return Qt.platform.os === "android"
            ? qsTr("Connect an Xbox or PlayStation gamepad over USB or Bluetooth.")
            : qsTr("Connect an Xbox, PlayStation or Steam gamepad over USB or Bluetooth.")
    }

    function glyphName(label) {
        switch (label) {
            case "cross":    return "✕"
            case "circle":   return "○"
            case "square":   return "□"
            case "triangle": return "△"
            default:         return label
        }
    }

    function buttonSymbol(bit) {
        var label = bit <= 3 && faceLabels.length > bit ? faceLabels[bit] : ""
        return label === "cross" || label === "circle" || label === "square" || label === "triangle" ? label : ""
    }

    function faceName(index) {
        var label = faceLabels.length > index ? faceLabels[index] : ""
        if (label.length)
            return glyphName(label)
        switch (index) {
            case 0:  return qsTr("Bottom button")
            case 1:  return qsTr("Right button")
            case 2:  return qsTr("Left button")
            default: return qsTr("Top button")
        }
    }

    function buttonName(bit) {
        if (bit <= 3)
            return faceName(bit)
        switch (bit) {
            case 4:  return family === "ps5" ? "Create" : family === "ps4" ? "Share" : family === "generic" ? "Back" : "View"
            case 5:  return playStation ? "PS" : family === "steam" ? "Steam" : family === "xbox" ? "Xbox" : "Guide"
            case 6:  return playStation ? "Options" : family === "generic" ? "Start" : "Menu"
            case 7:  return playStation ? "L3" : "LS"
            case 8:  return playStation ? "R3" : "RS"
            case 9:  return playStation ? "L1" : "LB"
            case 10: return playStation ? "R1" : "RB"
            case 11: return qsTr("D-pad up")
            case 12: return qsTr("D-pad down")
            case 13: return qsTr("D-pad left")
            case 14: return qsTr("D-pad right")
            case 15: return family === "ps5" ? qsTr("Mic") : family === "steam" ? "…" : "Share"
            case 16: return family === "steam" ? "R4" : "P1"
            case 17: return family === "steam" ? "L4" : "P3"
            case 18: return family === "steam" ? "R5" : "P2"
            case 19: return family === "steam" ? "L5" : "P4"
            default: return qsTr("Touchpad")
        }
    }

    function isDown(bit) {
        return (buttons & (1 << bit)) !== 0
    }

    function isAvailable(bit) {
        return bit !== guideButton && (!connected || (availableButtons & (1 << bit)) !== 0)
    }

    function axisText(label, value) {
        return label + " " + (value >= 0 ? "+" : "") + value.toFixed(2)
    }

    function setMonitoring(on) {
        if (hasMonitor)
            gamepadMonitor.active = on
        if (hasHelm)
            helm.configuring = on
    }

    readonly property bool hasHelm: typeof helm !== "undefined" && helm !== null
    readonly property var bindings: hasHelm ? helm.bindings : []
    readonly property var drivingBindings: bindings.filter(function(b) { return b.kind !== "button" })
    readonly property var buttonBindings: bindings.filter(function(b) { return b.kind === "button" })
    readonly property bool editable: hasHelm && connected && helm.layoutEditable
    readonly property int stickFirst: 100
    readonly property int leftTriggerInput: 110
    readonly property int rightTriggerInput: 111
    readonly property var axisInputs: [100, 101, 102, 103, 104, 105, 106, 107, 110, 111]
    readonly property int colInput: Math.round(150 * AppPalette.scale)

    property int listeningAction: -1
    property string message: ""
    property bool messageWarn: false
    property int prevButtons: 0
    property var armed: ({})

    function actionName(key) {
        switch (key) {
            case "throttleForward": return qsTr("Throttle forward")
            case "throttleReverse": return qsTr("Throttle reverse")
            case "steering":        return qsTr("Steering")
            case "manual":          return "Manual"
            case "hold":            return "Hold"
            case "loiter":          return "Loiter"
            case "auto":            return "Auto"
            case "courseHold":      return qsTr("Course hold")
            case "rtl":             return "RTL"
            default:                return key
        }
    }

    function axisName(axis) {
        switch (axis) {
            case 0:  return qsTr("Left stick X")
            case 1:  return qsTr("Left stick Y")
            case 2:  return qsTr("Right stick X")
            default: return qsTr("Right stick Y")
        }
    }

    function stickHalfName(input) {
        switch (input - stickFirst) {
            case 0:  return qsTr("Left stick →")
            case 1:  return qsTr("Left stick ←")
            case 2:  return qsTr("Left stick ↓")
            case 3:  return qsTr("Left stick ↑")
            case 4:  return qsTr("Right stick →")
            case 5:  return qsTr("Right stick ←")
            case 6:  return qsTr("Right stick ↓")
            default: return qsTr("Right stick ↑")
        }
    }

    function inputName(kind, input) {
        if (input < 0)
            return "—"
        if (kind === "button")
            return buttonName(input)
        if (input === leftTriggerInput)
            return playStation ? "L2" : "LT"
        if (input === rightTriggerInput)
            return playStation ? "R2" : "RT"
        if (kind === "steering") {
            var name = axisName(Math.floor((input - stickFirst) / 2))
            return (input - stickFirst) % 2 ? qsTr("%1, inverted").arg(name) : name
        }
        return stickHalfName(input)
    }

    function axisValue(axis) {
        if (!hasMonitor)
            return 0
        switch (axis) {
            case 0:  return gamepadMonitor.leftX
            case 1:  return gamepadMonitor.leftY
            case 2:  return gamepadMonitor.rightX
            default: return gamepadMonitor.rightY
        }
    }

    function halfRaw(input) {
        if (!hasMonitor)
            return 0
        if (input === leftTriggerInput)
            return gamepadMonitor.leftTrigger
        if (input === rightTriggerInput)
            return gamepadMonitor.rightTrigger
        var offset = input - stickFirst
        if (offset < 0 || offset > 7)
            return 0
        var v = axisValue(Math.floor(offset / 2))
        return offset % 2 ? -v : v
    }

    readonly property real triggerZone: hasHelm ? helm.triggerDeadzone : 0
    readonly property real stickZone: hasHelm ? helm.stickDeadzone : 0
    readonly property real liveThreshold: 0.01

    function stickComponent(axis) {
        var x = axisValue(axis)
        var magnitude = Math.sqrt(x * x + Math.pow(axisValue(axis ^ 1), 2))
        if (magnitude <= stickZone)
            return 0
        var scaled = Math.min(1, (magnitude - stickZone) / (1 - stickZone))
        return x / magnitude * scaled
    }

    function shapedHalf(input) {
        if (input === leftTriggerInput || input === rightTriggerInput) {
            var raw = halfRaw(input)
            return raw <= triggerZone ? 0 : Math.min(1, (raw - triggerZone) / (1 - triggerZone))
        }
        var offset = input - stickFirst
        if (offset < 0 || offset > 7)
            return 0
        var value = stickComponent(Math.floor(offset / 2))
        return offset % 2 ? -value : value
    }

    function inputActive(kind, input) {
        if (!connected || input < 0)
            return false
        if (kind === "button")
            return isDown(input)
        if (kind === "steering")
            return Math.abs(shapedHalf(input)) > liveThreshold
        return shapedHalf(input) > liveThreshold
    }

    function kindOf(action) {
        return action >= 0 && action < bindings.length ? bindings[action].kind : ""
    }

    function namesOf(actions) {
        var names = []
        for (var i = 0; i < actions.length; ++i) {
            var a = actions[i]
            if (a >= 0 && a < bindings.length)
                names.push(actionName(bindings[a].key))
        }
        return names.join(", ")
    }

    function toggleListening(action) {
        message = ""
        if (listeningAction === action || !editable) {
            listeningAction = -1
            return
        }
        prevButtons = buttons
        var state = {}
        for (var i = 0; i < axisInputs.length; ++i)
            state[axisInputs[i]] = halfRaw(axisInputs[i]) < 0.3
        armed = state
        listeningAction = action
    }

    function processSample() {
        if (listeningAction < 0)
            return
        var kind = kindOf(listeningAction)
        if (kind === "button") {
            var fresh = buttons & ~prevButtons
            prevButtons = buttons
            for (var b = 0; b < buttonCount; ++b) {
                if ((fresh & (1 << b)) !== 0) {
                    commit(b)
                    return
                }
            }
            return
        }
        for (var i = 0; i < axisInputs.length; ++i) {
            var input = axisInputs[i]
            if (kind === "steering" && (input === leftTriggerInput || input === rightTriggerInput))
                continue
            var v = halfRaw(input)
            if (v < 0.3) {
                armed[input] = true
            } else if (armed[input] && v > 0.6) {
                commit(input)
                return
            }
        }
    }

    function commit(input) {
        var action = listeningAction
        listeningAction = -1
        var result = helm.assignBinding(action, input)
        if (!result.ok) {
            message = qsTr("This input does not fit the action")
            messageWarn = true
            return
        }
        var parts = []
        if (result.swapped.length)
            parts.push(qsTr("Swapped with: %1").arg(namesOf(result.swapped)))
        if (result.cleared.length)
            parts.push(qsTr("Left without input: %1").arg(namesOf(result.cleared)))
        message = parts.join("  •  ")
        messageWarn = result.cleared.length > 0
    }

    readonly property string footerText: {
        if (!hasHelm || !connected)
            return ""
        if (!helm.layoutEditable)
            return qsTr("Stop gamepad control to change the layout")
        if (listeningAction >= 0) {
            var kind = kindOf(listeningAction)
            if (kind === "button")
                return qsTr("Press a gamepad button. Tap the row again to cancel.")
            if (kind === "steering")
                return qsTr("Push a stick to the right. Tap the row again to cancel.")
            return qsTr("Pull a trigger or push a stick the way this action should work. Tap the row again to cancel.")
        }
        if (message.length)
            return message
        return qsTr("Tap an action, then press the gamepad input for it")
    }

    Connections {
        target: dialog.hasMonitor ? gamepadMonitor : null
        function onStateChanged() { dialog.processSample() }
    }

    onEditableChanged: if (!editable) listeningAction = -1
    onFamilyChanged: message = ""

    parent: Overlay.overlay
    anchors.centerIn: parent
    modal: true
    focus: true
    padding: Math.round(14 * AppPalette.scale)
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    width: Math.min(Math.round((hasHelm ? 1000 : 640) * AppPalette.scale), parent ? parent.width * 0.95 : 1000)
    height: Math.min(Math.round(600 * AppPalette.scale), parent ? parent.height * 0.95 : 600)

    onOpened: {
        setMonitoring(true)
        if (store)
            store.activeHotkeysDialog = dialog
    }

    onClosed: {
        listeningAction = -1
        message = ""
        setMonitoring(false)
        if (store && store.activeHotkeysDialog === dialog)
            store.activeHotkeysDialog = null
    }

    Component.onDestruction: {
        setMonitoring(false)
        if (store && store.activeHotkeysDialog === dialog)
            store.activeHotkeysDialog = null
    }

    background: Rectangle {
        color: AppPalette.bgDeep
        border.color: AppPalette.border
        border.width: 1
        radius: 10
    }

    component PressedText: Text {
        color: AppPalette.text
        font.pixelSize: Tokens.fontBase
    }

    component SectionHeader: Item {
        id: sectionHeader
        property string text: ""
        width: parent ? parent.width : 0
        height: Math.round(26 * AppPalette.scale)

        Rectangle {
            anchors.left: parent.left
            anchors.right: sectionLabel.left
            anchors.rightMargin: Tokens.spaceSm
            anchors.verticalCenter: parent.verticalCenter
            height: 1
            color: AppPalette.border
        }
        Text {
            id: sectionLabel
            anchors.centerIn: parent
            text: sectionHeader.text
            color: AppPalette.textSecond
            font.pixelSize: Tokens.fontSm
            font.bold: true
        }
        Rectangle {
            anchors.left: sectionLabel.right
            anchors.leftMargin: Tokens.spaceSm
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            height: 1
            color: AppPalette.border
        }
    }

    component BindingRow: Item {
        id: bindingRow
        required property var modelData
        readonly property int action: modelData.action
        readonly property string kind: modelData.kind
        readonly property int input: modelData.input
        readonly property bool listening: dialog.listeningAction === action
        readonly property bool live: dialog.inputActive(kind, input)

        width: parent ? parent.width : 0
        height: Math.max(Tokens.controlHMd + Tokens.spaceMd, actionText.implicitHeight + Tokens.spaceMd)

        Rectangle {
            anchors.fill: parent
            radius: Tokens.radiusMd
            color: bindingRow.live ? Qt.rgba(AppPalette.accent.r, AppPalette.accent.g, AppPalette.accent.b, 0.18) : "transparent"
        }

        Rectangle {
            x: Tokens.spaceSm
            anchors.verticalCenter: parent.verticalCenter
            width: dialog.colInput - 2 * Tokens.spaceSm
            height: Tokens.controlHMd
            radius: Tokens.radiusMd
            color: bindingRow.listening ? AppPalette.accentBg : AppPalette.card
            border.color: bindingRow.listening ? AppPalette.accentBorder : AppPalette.border
            border.width: bindingRow.listening ? 1 : Tokens.cardBorderWidth

            Text {
                id: inputText
                anchors.centerIn: parent
                width: parent.width - 2 * Tokens.spaceSm
                visible: !inputSymbol.visible
                horizontalAlignment: Text.AlignHCenter
                elide: Text.ElideRight
                text: bindingRow.listening ? "…" : dialog.inputName(bindingRow.kind, bindingRow.input)
                color: bindingRow.listening ? AppPalette.accentText
                     : bindingRow.input < 0 ? AppPalette.dangerText
                     : dialog.editable ? AppPalette.text : AppPalette.textSecond
                font.pixelSize: Tokens.fontMd
                font.bold: true
            }
            GamepadSymbol {
                id: inputSymbol
                anchors.centerIn: parent
                width: Math.round(Tokens.fontMd * 0.9)
                visible: !bindingRow.listening && bindingRow.kind === "button" && label.length > 0
                label: bindingRow.input >= 0 ? dialog.buttonSymbol(bindingRow.input) : ""
                color: inputText.color
            }
        }

        Text {
            id: actionText
            x: dialog.colInput + Tokens.spaceSm
            width: parent.width - x - Tokens.spaceSm
            anchors.verticalCenter: parent.verticalCenter
            text: dialog.actionName(bindingRow.modelData.key)
            color: AppPalette.text
            font.pixelSize: Tokens.fontBase
            wrapMode: Text.WordWrap
        }

        MouseArea {
            anchors.fill: parent
            enabled: dialog.editable
            cursorShape: dialog.editable ? Qt.PointingHandCursor : Qt.ArrowCursor
            onClicked: dialog.toggleListening(bindingRow.action)
        }
    }

    component FixedRow: Item {
        id: fixedRow
        property string badge: ""
        property string label: ""
        property int mask: 0
        property bool all: false
        readonly property bool live: dialog.connected && (all ? (dialog.buttons & mask) === mask : (dialog.buttons & mask) !== 0)

        width: parent ? parent.width : 0
        height: Math.max(Tokens.controlHMd + Tokens.spaceMd, fixedText.implicitHeight + Tokens.spaceMd)

        Rectangle {
            anchors.fill: parent
            radius: Tokens.radiusMd
            color: fixedRow.live ? Qt.rgba(AppPalette.accent.r, AppPalette.accent.g, AppPalette.accent.b, 0.18) : "transparent"
        }

        Rectangle {
            x: Tokens.spaceSm
            anchors.verticalCenter: parent.verticalCenter
            width: dialog.colInput - 2 * Tokens.spaceSm
            height: Tokens.controlHMd
            radius: Tokens.radiusMd
            color: "transparent"
            border.color: AppPalette.border
            border.width: Tokens.cardBorderWidth

            Text {
                anchors.centerIn: parent
                width: parent.width - 2 * Tokens.spaceSm
                horizontalAlignment: Text.AlignHCenter
                elide: Text.ElideRight
                text: fixedRow.badge
                color: AppPalette.textSecond
                font.pixelSize: Tokens.fontMd
                font.bold: true
            }
        }

        Text {
            id: fixedText
            x: dialog.colInput + Tokens.spaceSm
            width: parent.width - x - Tokens.spaceSm
            anchors.verticalCenter: parent.verticalCenter
            text: fixedRow.label
            color: AppPalette.textSecond
            font.pixelSize: Tokens.fontBase
            wrapMode: Text.WordWrap
        }
    }

    component DeadzoneRow: Item {
        id: deadzoneRow
        property string label: ""
        property real value: 0
        signal moved(real value)

        width: parent ? parent.width : 0
        height: Tokens.controlHMd + Tokens.spaceMd

        Text {
            x: Tokens.spaceSm
            width: dialog.colInput - 2 * Tokens.spaceSm
            anchors.verticalCenter: parent.verticalCenter
            text: deadzoneRow.label
            color: AppPalette.text
            font.pixelSize: Tokens.fontBase
            wrapMode: Text.WordWrap
        }

        KSlider {
            x: dialog.colInput + Tokens.spaceSm
            width: parent.width - x - valueText.width - 2 * Tokens.spaceSm
            anchors.verticalCenter: parent.verticalCenter
            from: 0
            to: 0.5
            stepSize: 0.01
            enabled: dialog.hasHelm && helm.layoutEditable
            value: deadzoneRow.value
            onValueModified: function(val) { deadzoneRow.moved(val) }
        }

        Text {
            id: valueText
            anchors.right: parent.right
            anchors.rightMargin: Tokens.spaceSm
            anchors.verticalCenter: parent.verticalCenter
            width: Math.round(44 * AppPalette.scale)
            horizontalAlignment: Text.AlignRight
            text: Math.round(deadzoneRow.value * 100) + "%"
            color: AppPalette.text
            font.pixelSize: Tokens.fontBase
        }
    }

    Item {
        id: content
        anchors.fill: parent

        Text {
            id: titleText
            width: parent.width
            text: qsTr("Gamepad")
            color: AppPalette.textStrong
            font.pixelSize: Tokens.fontXl
            font.bold: true
        }

        KGamepadIcon {
            id: padIcon
            anchors.verticalCenter: padText.verticalCenter
            width: Math.round(22 * AppPalette.scale)
            height: width
            visible: dialog.connected
            family: dialog.family
            color: AppPalette.text
        }

        Item {
            id: padText
            readonly property var pads: dialog.hasMonitor ? gamepadMonitor.pads : []
            readonly property bool locked: pads.length > 1 && dialog.hasHelm && helm.active
            readonly property bool picking: pads.length > 1 && !locked
            readonly property int padIndex: {
                var id = dialog.hasMonitor ? gamepadMonitor.padId : 0
                for (var i = 0; i < pads.length; ++i)
                    if (pads[i].id === id) return i
                return -1
            }

            anchors.top: titleText.bottom
            anchors.topMargin: Tokens.spaceSm
            anchors.left: padIcon.right
            anchors.leftMargin: Tokens.spaceSm
            anchors.right: parent.right
            height: picking ? padPicker.height : padName.implicitHeight
            visible: dialog.connected

            KCombo {
                id: padPicker
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                width: Math.min(implicitWidth, parent.width - padFamilyText.implicitWidth - Tokens.spaceSm)
                visible: padText.picking
                fontPixelSize: Tokens.fontBase
                fillColor: AppPalette.card
                indicatorColor: AppPalette.textStrong
                model: padText.pads.map(function(pad) { return pad.label })
                toolTipText: qsTr("Gamepad to test and set up")
                onActivated: function(index) {
                    if (dialog.hasMonitor && index >= 0 && index < padText.pads.length)
                        gamepadMonitor.selectPad(padText.pads[index].id)
                }

                Binding {
                    target: padPicker
                    property: "currentIndex"
                    value: Math.max(0, padText.padIndex)
                }
            }
            Text {
                id: padName
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                visible: !padText.picking
                text: dialog.connected ? gamepadMonitor.name + "  ·  " + dialog.familyName : ""
                color: AppPalette.text
                font.pixelSize: Tokens.fontBase
                elide: Text.ElideRight
            }
            Text {
                id: padFamilyText
                anchors.left: padPicker.right
                anchors.leftMargin: Tokens.spaceSm
                anchors.verticalCenter: parent.verticalCenter
                visible: padText.picking
                text: dialog.familyName
                color: AppPalette.textSecond
                font.pixelSize: Tokens.fontBase
            }
        }

        Text {
            id: padLockNote
            anchors.top: padText.bottom
            anchors.topMargin: Tokens.spaceXs
            anchors.left: padText.left
            anchors.right: parent.right
            visible: dialog.connected && padText.locked
            text: qsTr("Switch gamepads after gamepad control ends")
            color: AppPalette.linkIdleText
            font.pixelSize: Tokens.fontSm
            wrapMode: Text.WordWrap
        }

        Item {
            id: body
            anchors.top: padLockNote.visible ? padLockNote.bottom : dialog.connected ? padText.bottom : titleText.bottom
            anchors.topMargin: Tokens.spaceLg
            anchors.bottom: footerRule.top
            anchors.bottomMargin: Tokens.spaceLg
            width: parent.width

            Column {
                anchors.centerIn: parent
                width: Math.min(parent.width, Math.round(460 * AppPalette.scale))
                visible: !dialog.connected
                spacing: Tokens.spaceMd

                Text {
                    width: parent.width
                    horizontalAlignment: Text.AlignHCenter
                    text: dialog.emptyTitle
                    color: AppPalette.textStrong
                    font.pixelSize: Tokens.fontXl
                    font.bold: true
                    wrapMode: Text.WordWrap
                }

                Text {
                    width: parent.width
                    visible: text.length > 0
                    horizontalAlignment: Text.AlignHCenter
                    text: dialog.emptyHint
                    color: AppPalette.textSecond
                    font.pixelSize: Tokens.fontBase
                    wrapMode: Text.WordWrap
                }
            }

            Item {
                id: padColumn
                visible: dialog.connected
                width: dialog.hasHelm ? Math.round((body.width - 2 * Tokens.spaceLg - 1) * 0.55) : body.width
                height: body.height

                Column {
                    id: readout
                    anchors.bottom: parent.bottom
                    width: parent.width
                    spacing: Tokens.spaceSm

                    KIsland {
                        width: parent.width
                        visible: dialog.hasHelm

                        KIslandRow {
                            label: qsTr("Vibration")
                            interactive: true
                            onClicked: rumbleSwitch.click()

                            KSwitch {
                                id: rumbleSwitch
                                flat: true
                                checked: dialog.hasHelm ? helm.rumble : false
                                onToggled: {
                                    if (dialog.hasHelm)
                                        helm.rumble = checked
                                    checked = Qt.binding(function() { return dialog.hasHelm ? helm.rumble : false })
                                }
                            }
                        }

                        KIslandRow {
                            id: strengthRow
                            label: qsTr("Strength")
                            slotWidth: Math.round(220 * AppPalette.scale)
                            open: dialog.hasHelm && helm.rumble

                            Item {
                                width: strengthRow.slotWidth
                                height: Tokens.controlHMd

                                KSlider {
                                    id: strengthSlider
                                    anchors.left: parent.left
                                    anchors.right: strengthValue.left
                                    anchors.rightMargin: Tokens.spaceSm
                                    anchors.verticalCenter: parent.verticalCenter
                                    from: 0.1
                                    to: 1
                                    stepSize: 0.05
                                    enabled: dialog.hasHelm && helm.rumble
                                    onValueModified: function(val) { if (dialog.hasHelm) helm.rumbleStrength = val }

                                    Binding {
                                        target: strengthSlider
                                        property: "value"
                                        value: dialog.hasHelm ? helm.rumbleStrength : 0.5
                                    }
                                }

                                Text {
                                    id: strengthValue
                                    anchors.right: parent.right
                                    anchors.verticalCenter: parent.verticalCenter
                                    width: Math.round(48 * AppPalette.scale)
                                    horizontalAlignment: Text.AlignRight
                                    text: Math.round(strengthSlider.value * 100) + "%"
                                    color: AppPalette.text
                                    font.pixelSize: Tokens.fontBase
                                }
                            }
                        }
                    }

                    Flow {
                        id: pressedLine
                        readonly property var parts: qsTr("Pressed: %1").split("%1")

                        width: parent.width

                        PressedText { text: pressedLine.parts[0] }
                        Repeater {
                            model: dialog.pressedItems
                            delegate: Row {
                                id: pressedItem
                                required property var modelData
                                required property int index
                                height: pressedSample.implicitHeight

                                PressedText { text: pressedItem.index > 0 ? ", " : "" }
                                PressedText { visible: !pressedItem.modelData.symbol; text: pressedItem.modelData.text }
                                GamepadSymbol {
                                    anchors.verticalCenter: parent.verticalCenter
                                    width: Math.round(Tokens.fontBase * 0.8)
                                    visible: pressedItem.modelData.symbol.length > 0
                                    label: pressedItem.modelData.symbol
                                    color: AppPalette.text
                                }
                            }
                        }
                        PressedText { id: pressedSample; text: dialog.pressedItems.length ? "" : "—" }
                        PressedText { text: pressedLine.parts.length > 1 ? pressedLine.parts[1] : "" }
                    }

                    Text {
                        width: parent.width
                        text: !dialog.connected ? "" :
                              [dialog.axisText("LX", gamepadMonitor.leftX), dialog.axisText("LY", gamepadMonitor.leftY),
                               dialog.axisText("RX", gamepadMonitor.rightX), dialog.axisText("RY", gamepadMonitor.rightY),
                               dialog.axisText(dialog.playStation ? "L2" : "LT", gamepadMonitor.leftTrigger),
                               dialog.axisText(dialog.playStation ? "R2" : "RT", gamepadMonitor.rightTrigger)].join("   ")
                        color: AppPalette.textSecond
                        font.pixelSize: Tokens.fontSm
                        font.family: "monospace"
                        wrapMode: Text.Wrap
                    }
                }

                Loader {
                    id: padLoader
                    anchors.top: parent.top
                    anchors.bottom: readout.top
                    anchors.bottomMargin: Tokens.spaceMd
                    width: parent.width
                    active: dialog.connected
                    source: dialog.family === "ps4" || dialog.family === "ps5" ? "GamepadDualShock.qml"
                          : dialog.family === "steam" ? "GamepadSteam.qml"
                          : "GamepadXbox.qml"
                    onLoaded: item.view = dialog
                }
            }

            Rectangle {
                id: columnRule
                x: padColumn.width + Tokens.spaceLg
                width: 1
                height: body.height
                visible: dialog.connected && dialog.hasHelm
                color: AppPalette.border
            }

            Flickable {
                id: listFlick
                anchors.left: columnRule.right
                anchors.leftMargin: Tokens.spaceLg
                anchors.right: parent.right
                height: body.height
                visible: dialog.connected && dialog.hasHelm
                contentWidth: width
                contentHeight: bindingList.implicitHeight
                boundsBehavior: Flickable.StopAtBounds
                clip: true
                ScrollBar.vertical: ScrollBar { active: listFlick.moving || hovered || pressed }

                Column {
                    id: bindingList
                    width: listFlick.width - Tokens.spaceMd
                    spacing: Tokens.spaceXs

                    SectionHeader { text: qsTr("Driving") }

                    Repeater {
                        model: dialog.drivingBindings
                        delegate: BindingRow {}
                    }

                    SectionHeader { text: qsTr("Buttons") }

                    Repeater {
                        model: dialog.buttonBindings
                        delegate: BindingRow {}
                    }

                    FixedRow {
                        badge: dialog.playStation ? "L1 + R1" : "LB + RB"
                        label: "ARM / DISARM"
                        mask: (1 << 9) | (1 << 10)
                        all: true
                    }

                    SectionHeader { text: qsTr("Dead zones") }

                    DeadzoneRow {
                        label: qsTr("Sticks")
                        value: dialog.hasHelm ? helm.stickDeadzone : 0
                        onMoved: function(val) { helm.stickDeadzone = val }
                    }

                    DeadzoneRow {
                        label: qsTr("Triggers")
                        value: dialog.hasHelm ? helm.triggerDeadzone : 0
                        onMoved: function(val) { helm.triggerDeadzone = val }
                    }
                }
            }
        }

        Rectangle {
            id: footerRule
            anchors.bottom: footer.top
            anchors.bottomMargin: Tokens.spaceLg
            width: parent.width
            height: 1
            color: AppPalette.border
        }

        Item {
            id: footer
            anchors.bottom: parent.bottom
            width: parent.width
            height: Math.max(Tokens.controlHMd, footerLabel.implicitHeight)

            Text {
                id: footerLabel
                anchors.left: parent.left
                anchors.right: resetButton.visible ? resetButton.left : closeButton.left
                anchors.rightMargin: Tokens.spaceSm
                anchors.verticalCenter: parent.verticalCenter
                text: dialog.footerText
                color: dialog.listeningAction >= 0 ? AppPalette.accentBorder
                     : (dialog.message.length && dialog.messageWarn) ? AppPalette.dangerText
                     : AppPalette.textSecond
                font.pixelSize: Tokens.fontSm
                wrapMode: Text.Wrap
            }

            TextMetrics {
                id: resetMetrics
                font.pixelSize: Tokens.fontMd
                font.bold: true
                text: qsTr("Reset to defaults")
            }

            KButton {
                id: resetButton
                anchors.right: closeButton.left
                anchors.rightMargin: Tokens.spaceSm
                anchors.verticalCenter: parent.verticalCenter
                width: Math.max(Math.round(124 * AppPalette.scale), Math.ceil(resetMetrics.advanceWidth) + 4 * Tokens.spaceLg)
                height: Tokens.controlHMd
                visible: dialog.hasHelm && dialog.connected
                enabled: dialog.hasHelm && helm.layoutEditable
                fontPixelSize: Tokens.fontMd
                text: qsTr("Reset to defaults")
                onClicked: {
                    dialog.listeningAction = -1
                    dialog.message = ""
                    helm.resetPadSettings()
                }
            }

            KButton {
                id: closeButton
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                width: Math.round(124 * AppPalette.scale)
                height: Tokens.controlHMd
                fontPixelSize: Tokens.fontMd
                text: qsTr("Close")
                onClicked: dialog.close()
            }
        }
    }
}
