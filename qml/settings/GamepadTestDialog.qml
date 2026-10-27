import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Shapes
import kqml_types 1.0

Popup {
    id: dialog

    property var store: null

    readonly property bool hasMonitor: typeof gamepadMonitor !== "undefined" && gamepadMonitor !== null
    readonly property bool supported: hasMonitor && gamepadMonitor.supported
    readonly property bool connected: hasMonitor && gamepadMonitor.connected
    readonly property int buttons: hasMonitor ? gamepadMonitor.buttons : 0
    readonly property int availableButtons: hasMonitor ? gamepadMonitor.availableButtons : 0
    readonly property int drawnButtons: 0xFFFF
    readonly property bool someDrawnMissing: connected && (availableButtons & drawnButtons) !== drawnButtons

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
        return !connected || (availableButtons & (1 << bit)) !== 0
    }

    function axisText(label, value) {
        return label + " " + (value >= 0 ? "+" : "") + value.toFixed(2)
    }

    function setMonitoring(on) {
        if (hasMonitor)
            gamepadMonitor.active = on
    }

    parent: Overlay.overlay
    anchors.centerIn: parent
    modal: true
    focus: true
    padding: Math.round(14 * AppPalette.scale)
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    width: Math.min(Math.round(640 * AppPalette.scale), parent ? parent.width * 0.95 : 640)
    height: Math.min(content.implicitHeight + topPadding + bottomPadding, parent ? parent.height * 0.95 : 600)

    onOpened: {
        setMonitoring(true)
        if (store)
            store.activeHotkeysDialog = dialog
    }

    onClosed: {
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

    Column {
        id: content
        width: dialog.availableWidth
        spacing: Tokens.spaceLg

        Text {
            width: parent.width
            text: qsTr("Gamepad")
            color: AppPalette.textStrong
            font.pixelSize: Tokens.fontXl
            font.bold: true
        }

        Text {
            width: parent.width
            text: dialog.statusText
            color: dialog.connected ? AppPalette.text : AppPalette.textSecond
            font.pixelSize: Tokens.fontBase
            wrapMode: Text.Wrap
        }

        Item {
            id: padArea
            readonly property real designW: 560
            readonly property real designH: 380
            readonly property real fit: width / designW

            width: parent.width
            height: designH * fit
            opacity: dialog.connected ? 1.0 : 0.45

            Item {
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
            width: parent.width
            text: qsTr("Pressed: %1").arg(dialog.pressedText)
            color: AppPalette.text
            font.pixelSize: Tokens.fontBase
            wrapMode: Text.Wrap
        }

        Text {
            width: parent.width
            visible: dialog.someDrawnMissing
            text: qsTr("Dimmed buttons are not reported by this controller or its driver.")
            color: AppPalette.textSecond
            font.pixelSize: Tokens.fontSm
            wrapMode: Text.Wrap
        }

        Text {
            width: parent.width
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

        KButton {
            anchors.right: parent.right
            width: Math.round(124 * AppPalette.scale)
            height: Tokens.controlHMd
            fontPixelSize: Tokens.fontMd
            text: qsTr("Close")
            onClicked: dialog.close()
        }
    }
}
