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

    GamepadKey { pad: root; x: 72;  y: 38; width: 128; height: 26; radius: 13; bit: 9;  label: "LB" }
    GamepadKey { pad: root; x: 360; y: 38; width: 128; height: 26; radius: 13; bit: 10; label: "RB" }

    Shape {
        anchors.fill: parent
        preferredRendererType: Shape.CurveRenderer

        ShapePath {
            fillColor: AppPalette.card
            strokeColor: AppPalette.borderHover
            strokeWidth: 2
            joinStyle: ShapePath.RoundJoin
            startX: 176; startY: 78
            PathLine { x: 384; y: 78 }
            PathCubic { control1X: 426; control1Y: 74;  control2X: 458; control2Y: 82;  x: 478; y: 100 }
            PathCubic { control1X: 500; control1Y: 120; control2X: 512; control2Y: 150; x: 520; y: 190 }
            PathCubic { control1X: 530; control1Y: 240; control2X: 540; control2Y: 300; x: 516; y: 324 }
            PathCubic { control1X: 494; control1Y: 346; control2X: 456; control2Y: 342; x: 434; y: 314 }
            PathCubic { control1X: 420; control1Y: 300; control2X: 404; control2Y: 290; x: 372; y: 288 }
            PathLine { x: 188; y: 288 }
            PathCubic { control1X: 156; control1Y: 290; control2X: 140; control2Y: 300; x: 126; y: 314 }
            PathCubic { control1X: 104; control1Y: 342; control2X: 66;  control2Y: 346; x: 44;  y: 324 }
            PathCubic { control1X: 20;  control1Y: 300; control2X: 30;  control2Y: 240; x: 40;  y: 190 }
            PathCubic { control1X: 48;  control1Y: 150; control2X: 60;  control2Y: 120; x: 82;  y: 100 }
            PathCubic { control1X: 102; control1Y: 82;  control2X: 134; control2Y: 74;  x: 176; y: 78 }
        }
    }

    GamepadStick { pad: root; bit: 7; x: 146 - width / 2; y: 156 - height / 2; axisX: root.axis(0); axisY: root.axis(1); deadzone: root.stickZone }
    GamepadStick { pad: root; bit: 8; x: 346 - width / 2; y: 226 - height / 2; axisX: root.axis(2); axisY: root.axis(3); deadzone: root.stickZone }

    GamepadDpad { pad: root; key: 23; x: 214 - width / 2; y: 226 - height / 2 }

    GamepadKey { pad: root; bit: 0; x: 414 - 18; y: 190 - 18; width: 36; height: 36; tinted: true; label: root.faceLabel(0, "A"); tint: root.generic ? AppPalette.accent : "#3CB043" }
    GamepadKey { pad: root; bit: 1; x: 450 - 18; y: 156 - 18; width: 36; height: 36; tinted: true; label: root.faceLabel(1, "B"); tint: root.generic ? AppPalette.accent : "#E53935" }
    GamepadKey { pad: root; bit: 2; x: 378 - 18; y: 156 - 18; width: 36; height: 36; tinted: true; label: root.faceLabel(2, "X"); tint: root.generic ? AppPalette.accent : "#1E88E5" }
    GamepadKey { pad: root; bit: 3; x: 414 - 18; y: 122 - 18; width: 36; height: 36; tinted: true; label: root.faceLabel(3, "Y"); tint: root.generic ? AppPalette.accent : "#E2B800" }

    GamepadKey { pad: root; bit: 4;  x: 242 - 11; y: 156 - 11; width: 22; height: 22 }
    GamepadKey { pad: root; bit: 6;  x: 318 - 11; y: 156 - 11; width: 22; height: 22 }
    GamepadKey { pad: root; bit: 5;  x: 280 - 20; y: 112 - 20; width: 40; height: 40 }
    GamepadKey { pad: root; bit: 15; x: 280 - 10; y: 182 - 6;  width: 20; height: 12 }
}
