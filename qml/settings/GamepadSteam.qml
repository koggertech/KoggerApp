import QtQuick
import QtQuick.Shapes
import kqml_types 1.0

GamepadPad {
    id: root

    GamepadTrigger { x: 70;  y: 0; label: "LT"; value: root.trigger(110); deadzone: root.triggerZone }
    GamepadTrigger { x: 400; y: 0; label: "RT"; value: root.trigger(111); deadzone: root.triggerZone }

    GamepadKey { pad: root; x: 60;  y: 38; width: 130; height: 28; radius: 12; bit: 9;  label: "LB" }
    GamepadKey { pad: root; x: 370; y: 38; width: 130; height: 28; radius: 12; bit: 10; label: "RB" }

    Shape {
        anchors.fill: parent
        preferredRendererType: Shape.CurveRenderer

        ShapePath {
            fillColor: AppPalette.card
            strokeColor: AppPalette.borderHover
            strokeWidth: 2
            startX: 120; startY: 74
            PathLine { x: 440; y: 74 }
            PathCubic { control1X: 500; control1Y: 74;  control2X: 530; control2Y: 110; x: 536; y: 170 }
            PathCubic { control1X: 544; control1Y: 250; control2X: 550; control2Y: 330; x: 516; y: 356 }
            PathCubic { control1X: 486; control1Y: 378; control2X: 452; control2Y: 362; x: 440; y: 330 }
            PathLine { x: 120; y: 330 }
            PathCubic { control1X: 108; control1Y: 362; control2X: 74;  control2Y: 378; x: 44;  y: 356 }
            PathCubic { control1X: 10;  control1Y: 330; control2X: 16;  control2Y: 250; x: 24;  y: 170 }
            PathCubic { control1X: 30;  control1Y: 110; control2X: 60;  control2Y: 74;  x: 120; y: 74 }
        }

        ShapePath {
            fillColor: AppPalette.controlRaised
            strokeColor: AppPalette.borderHover
            strokeWidth: 2
            startX: 140; startY: 212
            PathLine { x: 226; y: 212 }
            PathLine { x: 226; y: 298 }
            PathLine { x: 140; y: 298 }
            PathLine { x: 140; y: 212 }
            PathMove { x: 334; y: 212 }
            PathLine { x: 420; y: 212 }
            PathLine { x: 420; y: 298 }
            PathLine { x: 334; y: 298 }
            PathLine { x: 334; y: 212 }
        }
    }

    GamepadDpad { pad: root; x: 108 - width / 2; y: 140 - height / 2 }

    GamepadKey { pad: root; bit: 0; x: 452 - 17; y: 172 - 17; width: 34; height: 34; tinted: true; label: "A"; tint: "#3CB043" }
    GamepadKey { pad: root; bit: 1; x: 484 - 17; y: 140 - 17; width: 34; height: 34; tinted: true; label: "B"; tint: "#E53935" }
    GamepadKey { pad: root; bit: 2; x: 420 - 17; y: 140 - 17; width: 34; height: 34; tinted: true; label: "X"; tint: "#1E88E5" }
    GamepadKey { pad: root; bit: 3; x: 452 - 17; y: 108 - 17; width: 34; height: 34; tinted: true; label: "Y"; tint: "#E2B800" }

    GamepadStick { pad: root; bit: 7; x: 210 - width / 2; y: 150 - height / 2; axisX: root.axis(0); axisY: root.axis(1); deadzone: root.stickZone }
    GamepadStick { pad: root; bit: 8; x: 350 - width / 2; y: 150 - height / 2; axisX: root.axis(2); axisY: root.axis(3); deadzone: root.stickZone }

    GamepadKey { pad: root; bit: 4;  x: 190 - 14; y: 88; width: 28; height: 16 }
    GamepadKey { pad: root; bit: 6;  x: 370 - 14; y: 88; width: 28; height: 16 }
    GamepadKey { pad: root; bit: 5;  x: 280 - 16; y: 84; width: 32; height: 32 }
    GamepadKey { pad: root; bit: 15; x: 280 - 16; y: 250; width: 32; height: 16; label: "…"; labelSize: 12 }

    GamepadKey { pad: root; bit: 17; x: 62;  y: 296; width: 34; height: 18; label: "L4"; labelSize: 11 }
    GamepadKey { pad: root; bit: 19; x: 78;  y: 322; width: 34; height: 18; label: "L5"; labelSize: 11 }
    GamepadKey { pad: root; bit: 16; x: 464; y: 296; width: 34; height: 18; label: "R4"; labelSize: 11 }
    GamepadKey { pad: root; bit: 18; x: 448; y: 322; width: 34; height: 18; label: "R5"; labelSize: 11 }
}
