import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Shapes
import kqml_types 1.0

Popup {
    id: dialog

    property var store: null

    readonly property bool hasMonitor: typeof gamepadMonitor !== "undefined" && gamepadMonitor !== null
    readonly property bool supported: hasMonitor && gamepadMonitor.supported
    readonly property bool connected: hasMonitor && gamepadMonitor.connected
    readonly property int guideButton: 5
    readonly property int reservedMask: (1 << 11) | (1 << 12)
    readonly property int buttons: hasMonitor ? (gamepadMonitor.buttons & ~(1 << guideButton)) : 0
    readonly property int availableButtons: hasMonitor ? gamepadMonitor.availableButtons : 0

    readonly property var buttonNames: [
        "A", "B", "X", "Y", "View", "Xbox", "Menu", "LS", "RS", "LB", "RB",
        qsTr("D-pad up"), qsTr("D-pad down"), qsTr("D-pad left"), qsTr("D-pad right"), "Share",
        "P1", "P3", "P2", "P4", "Touchpad"
    ]

    readonly property string pressedText: {
        var names = []
        for (var i = 0; i < buttonNames.length; ++i) {
            if ((buttons & (1 << i)) !== 0)
                names.push(buttonNames[i])
        }
        return names.length ? names.join(", ") : qsTr("none")
    }

    readonly property string statusText: {
        if (!supported)
            return qsTr("Gamepad support is not built into this version")
        if (hasMonitor && gamepadMonitor.error.length)
            return qsTr("Gamepad subsystem failed: %1").arg(gamepadMonitor.error)
        if (!connected)
            return qsTr("No gamepad found. Connect one over USB or Bluetooth.")
        var type = gamepadMonitor.typeName.length ? " (" + gamepadMonitor.typeName + ")" : ""
        return gamepadMonitor.name + type
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
    }

    readonly property bool hasHelm: typeof helm !== "undefined" && helm !== null
    readonly property var bindings: hasHelm ? helm.bindings : []
    readonly property var drivingBindings: bindings.filter(function(b) { return b.kind !== "button" })
    readonly property var buttonBindings: bindings.filter(function(b) { return b.kind === "button" })
    readonly property bool editable: hasHelm && connected && !helm.active
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
            case "toggleLoiter":    return qsTr("Switch Manual / Loiter")
            case "hold":            return qsTr("Hold (motors stop)")
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
            return input < buttonNames.length ? buttonNames[input] : String(input)
        if (input === leftTriggerInput)
            return "LT"
        if (input === rightTriggerInput)
            return "RT"
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

    function inputActive(kind, input) {
        if (!connected || input < 0)
            return false
        if (kind === "button")
            return isDown(input)
        if (kind === "steering")
            return Math.abs(halfRaw(input)) > 0.25
        return halfRaw(input) > 0.25
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
            if (fresh !== 0 && (fresh & ~reservedMask) === 0) {
                listeningAction = -1
                message = qsTr("D-pad up and down change the forward limit and cannot be bound")
                messageWarn = true
                return
            }
            for (var b = 0; b < buttonNames.length; ++b) {
                if ((reservedMask & (1 << b)) !== 0)
                    continue
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
        if (!hasHelm)
            return ""
        if (helm.active)
            return qsTr("Stop gamepad control to change the layout")
        if (!connected)
            return qsTr("Connect a gamepad to change the layout")
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

    component PadKey: Rectangle {
        property int bit: 0
        property string label: ""
        property color idleColor: AppPalette.controlRaised
        property color downColor: AppPalette.accent
        readonly property bool down: dialog.isDown(bit)
        readonly property bool available: dialog.isAvailable(bit)

        opacity: available ? 1.0 : 0.2
        radius: Math.min(width, height) / 2
        color: down ? downColor : idleColor
        border.color: down ? Qt.lighter(downColor, 1.3) : AppPalette.borderHover
        border.width: 2

        Text {
            anchors.centerIn: parent
            text: parent.label
            color: parent.down ? "#FFFFFF" : AppPalette.text
            font.pixelSize: 13
            font.bold: true
        }
    }

    component FaceKey: PadKey {
        width: 36
        height: 36
        idleColor: Qt.rgba(downColor.r, downColor.g, downColor.b, 0.28)
    }

    component Trigger: Rectangle {
        property real value: 0
        property string label: ""

        width: 90
        height: 30
        radius: 8
        color: AppPalette.controlRaised
        border.color: value > 0.05 ? AppPalette.accent : AppPalette.borderHover
        border.width: 2
        clip: true

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: parent.height * Math.max(0, parent.value)
            color: AppPalette.accent
        }

        Text {
            anchors.centerIn: parent
            text: parent.label
            color: AppPalette.text
            font.pixelSize: 13
            font.bold: true
        }
    }

    component Stick: Item {
        property real axisX: 0
        property real axisY: 0
        property int bit: 0
        readonly property bool down: dialog.isDown(bit)

        width: 76
        height: 76

        Rectangle {
            anchors.fill: parent
            radius: width / 2
            color: AppPalette.bgDeep
            border.color: AppPalette.borderHover
            border.width: 2
        }

        Rectangle {
            width: 50
            height: 50
            radius: 25
            x: (parent.width - width) / 2 + parent.axisX * 16
            y: (parent.height - height) / 2 + parent.axisY * 16
            color: parent.down ? AppPalette.accent : AppPalette.controlRaised
            border.color: parent.down ? Qt.lighter(AppPalette.accent, 1.3) : AppPalette.borderFocus
            border.width: 2
        }
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
                anchors.centerIn: parent
                width: parent.width - 2 * Tokens.spaceSm
                horizontalAlignment: Text.AlignHCenter
                elide: Text.ElideRight
                text: bindingRow.listening ? "…" : dialog.inputName(bindingRow.kind, bindingRow.input)
                color: bindingRow.listening ? AppPalette.accentText
                     : bindingRow.input < 0 ? AppPalette.dangerText
                     : dialog.editable ? AppPalette.text : AppPalette.textSecond
                font.pixelSize: Tokens.fontMd
                font.bold: true
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

    ColumnLayout {
        id: content
        anchors.fill: parent
        spacing: Tokens.spaceLg

        Text {
            Layout.fillWidth: true
            text: qsTr("Gamepad")
            color: AppPalette.textStrong
            font.pixelSize: Tokens.fontXl
            font.bold: true
        }

        Text {
            Layout.fillWidth: true
            text: dialog.statusText
            color: dialog.connected ? AppPalette.text : AppPalette.textSecond
            font.pixelSize: Tokens.fontBase
            wrapMode: Text.Wrap
        }

        RowLayout {
            id: mainRow
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: Tokens.spaceLg

            ColumnLayout {
                Layout.fillHeight: true
                Layout.fillWidth: !dialog.hasHelm
                Layout.preferredWidth: dialog.hasHelm ? Math.round((mainRow.width - 2 * mainRow.spacing - 1) * 0.55) : mainRow.width
                spacing: Tokens.spaceMd

                Item {
                    id: padArea
                    readonly property real designW: 560
                    readonly property real designH: 380
                    readonly property real fit: Math.max(0, Math.min(width / designW, height / designH))

                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    opacity: dialog.connected ? 1.0 : 0.45

                    Item {
                        x: (padArea.width - padArea.designW * padArea.fit) / 2
                        y: (padArea.height - padArea.designH * padArea.fit) / 2
                        width: padArea.designW
                        height: padArea.designH
                        transform: Scale { xScale: padArea.fit; yScale: padArea.fit }

                        Trigger { x: 80;  y: 0; label: "LT"; value: dialog.hasMonitor ? gamepadMonitor.leftTrigger : 0 }
                        Trigger { x: 390; y: 0; label: "RT"; value: dialog.hasMonitor ? gamepadMonitor.rightTrigger : 0 }

                        PadKey { x: 70;  y: 38; width: 130; height: 28; radius: 12; bit: 9;  label: "LB" }
                        PadKey { x: 360; y: 38; width: 130; height: 28; radius: 12; bit: 10; label: "RB" }

                        Shape {
                            anchors.fill: parent
                            preferredRendererType: Shape.CurveRenderer

                            ShapePath {
                                fillColor: AppPalette.card
                                strokeColor: AppPalette.borderHover
                                strokeWidth: 2
                                startX: 170; startY: 72
                                PathLine { x: 390; y: 72 }
                                PathCubic { control1X: 450; control1Y: 72;  control2X: 490; control2Y: 92;  x: 510; y: 150 }
                                PathCubic { control1X: 535; control1Y: 230; control2X: 548; control2Y: 310; x: 518; y: 348 }
                                PathCubic { control1X: 492; control1Y: 375; control2X: 455; control2Y: 360; x: 435; y: 330 }
                                PathCubic { control1X: 410; control1Y: 292; control2X: 392; control2Y: 268; x: 356; y: 264 }
                                PathLine { x: 204; y: 264 }
                                PathCubic { control1X: 168; control1Y: 268; control2X: 150; control2Y: 292; x: 125; y: 330 }
                                PathCubic { control1X: 105; control1Y: 360; control2X: 68;  control2Y: 375; x: 42;  y: 348 }
                                PathCubic { control1X: 12;  control1Y: 310; control2X: 25;  control2Y: 230; x: 50;  y: 150 }
                                PathCubic { control1X: 70;  control1Y: 92;  control2X: 110; control2Y: 72;  x: 170; y: 72 }
                            }
                        }

                        Stick {
                            x: 150 - width / 2; y: 150 - height / 2
                            bit: 7
                            axisX: dialog.hasMonitor ? gamepadMonitor.leftX : 0
                            axisY: dialog.hasMonitor ? gamepadMonitor.leftY : 0
                        }

                        Stick {
                            x: 345 - width / 2; y: 222 - height / 2
                            bit: 8
                            axisX: dialog.hasMonitor ? gamepadMonitor.rightX : 0
                            axisY: dialog.hasMonitor ? gamepadMonitor.rightY : 0
                        }

                        PadKey { x: 215 - 12; y: 198 - 12; width: 24; height: 24; radius: 4; bit: 11 }
                        PadKey { x: 215 - 12; y: 246 - 12; width: 24; height: 24; radius: 4; bit: 12 }
                        PadKey { x: 191 - 12; y: 222 - 12; width: 24; height: 24; radius: 4; bit: 13 }
                        PadKey { x: 239 - 12; y: 222 - 12; width: 24; height: 24; radius: 4; bit: 14 }
                        Rectangle { x: 215 - 12; y: 222 - 12; width: 24; height: 24; color: AppPalette.controlRaised }

                        FaceKey { x: 410 - 18; y: 184 - 18; bit: 0; label: "A"; downColor: "#3CB043" }
                        FaceKey { x: 444 - 18; y: 150 - 18; bit: 1; label: "B"; downColor: "#E53935" }
                        FaceKey { x: 376 - 18; y: 150 - 18; bit: 2; label: "X"; downColor: "#1E88E5" }
                        FaceKey { x: 410 - 18; y: 116 - 18; bit: 3; label: "Y"; downColor: "#E2B800" }

                        PadKey { x: 240 - 14; y: 150 - 9; width: 28; height: 18; bit: 4 }
                        PadKey { x: 320 - 14; y: 150 - 9; width: 28; height: 18; bit: 6 }
                        PadKey { x: 280 - 19; y: 112 - 19; width: 38; height: 38; bit: 5 }
                        PadKey { x: 280 - 10; y: 160 - 6;  width: 20; height: 12; bit: 15 }
                    }
                }

                Text {
                    Layout.fillWidth: true
                    text: qsTr("Pressed: %1").arg(dialog.pressedText)
                    color: AppPalette.text
                    font.pixelSize: Tokens.fontBase
                    wrapMode: Text.Wrap
                }

                Text {
                    Layout.fillWidth: true
                    visible: dialog.connected
                    text: !dialog.connected ? "" :
                          [dialog.axisText("LX", gamepadMonitor.leftX), dialog.axisText("LY", gamepadMonitor.leftY),
                           dialog.axisText("RX", gamepadMonitor.rightX), dialog.axisText("RY", gamepadMonitor.rightY),
                           dialog.axisText("LT", gamepadMonitor.leftTrigger), dialog.axisText("RT", gamepadMonitor.rightTrigger)].join("   ")
                    color: AppPalette.textSecond
                    font.pixelSize: Tokens.fontSm
                    font.family: "monospace"
                    wrapMode: Text.Wrap
                }
            }

            Rectangle {
                Layout.fillHeight: true
                Layout.preferredWidth: 1
                visible: dialog.hasHelm
                color: AppPalette.border
            }

            Flickable {
                id: listFlick
                Layout.fillWidth: true
                Layout.fillHeight: true
                visible: dialog.hasHelm
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
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: AppPalette.border
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Tokens.spaceSm

            Text {
                Layout.fillWidth: true
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
                Layout.preferredWidth: Math.max(Math.round(124 * AppPalette.scale), Math.ceil(resetMetrics.advanceWidth) + 4 * Tokens.spaceLg)
                Layout.preferredHeight: Tokens.controlHMd
                visible: dialog.hasHelm
                enabled: dialog.hasHelm && !helm.active
                fontPixelSize: Tokens.fontMd
                text: qsTr("Reset to defaults")
                onClicked: {
                    dialog.listeningAction = -1
                    dialog.message = ""
                    helm.resetBindings()
                }
            }

            KButton {
                Layout.preferredWidth: Math.round(124 * AppPalette.scale)
                Layout.preferredHeight: Tokens.controlHMd
                fontPixelSize: Tokens.fontMd
                text: qsTr("Close")
                onClicked: dialog.close()
            }
        }
    }
}
