import QtQuick
import QtQuick.Shapes
import kqml_types 1.0

GamepadPad {
    id: root

    readonly property bool generic: !!view && view.family === "generic"

    function faceLabel(index, fallback) {
        var label = root.face(index)
        return label.length ? label : (root.generic ? "" : fallback)
    }

    GamepadTrigger { x: 80;  y: 0; label: "LT"; value: root.trigger(110); deadzone: root.triggerZone }
    GamepadTrigger { x: 390; y: 0; label: "RT"; value: root.trigger(111); deadzone: root.triggerZone }

    GamepadKey { pad: root; x: 70;  y: 38; width: 130; height: 28; radius: 12; bit: 9;  label: "LB" }
    GamepadKey { pad: root; x: 360; y: 38; width: 130; height: 28; radius: 12; bit: 10; label: "RB" }

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

    GamepadStick { pad: root; bit: 7; x: 150 - width / 2; y: 150 - height / 2; axisX: root.axis(0); axisY: root.axis(1); deadzone: root.stickZone }
    GamepadStick { pad: root; bit: 8; x: 345 - width / 2; y: 222 - height / 2; axisX: root.axis(2); axisY: root.axis(3); deadzone: root.stickZone }

    GamepadDpad { pad: root; x: 215 - width / 2; y: 222 - height / 2 }

    GamepadKey { pad: root; bit: 0; x: 410 - 18; y: 184 - 18; width: 36; height: 36; tinted: true; label: root.faceLabel(0, "A"); tint: root.generic ? AppPalette.accent : "#3CB043" }
    GamepadKey { pad: root; bit: 1; x: 444 - 18; y: 150 - 18; width: 36; height: 36; tinted: true; label: root.faceLabel(1, "B"); tint: root.generic ? AppPalette.accent : "#E53935" }
    GamepadKey { pad: root; bit: 2; x: 376 - 18; y: 150 - 18; width: 36; height: 36; tinted: true; label: root.faceLabel(2, "X"); tint: root.generic ? AppPalette.accent : "#1E88E5" }
    GamepadKey { pad: root; bit: 3; x: 410 - 18; y: 116 - 18; width: 36; height: 36; tinted: true; label: root.faceLabel(3, "Y"); tint: root.generic ? AppPalette.accent : "#E2B800" }

    GamepadKey { pad: root; bit: 4;  x: 240 - 14; y: 150 - 9; width: 28; height: 18 }
    GamepadKey { pad: root; bit: 6;  x: 320 - 14; y: 150 - 9; width: 28; height: 18 }
    GamepadKey { pad: root; bit: 5;  x: 280 - 19; y: 112 - 19; width: 38; height: 38 }
    GamepadKey { pad: root; bit: 15; x: 280 - 10; y: 160 - 6;  width: 20; height: 12 }
}
