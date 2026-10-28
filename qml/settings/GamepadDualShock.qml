import QtQuick
import QtQuick.Shapes
import kqml_types 1.0

GamepadPad {
    id: root

    readonly property bool dualSense: !!view && view.family === "ps5"

    GamepadTrigger { x: 70;  y: 0; label: "L2"; value: root.trigger(110); deadzone: root.triggerZone }
    GamepadTrigger { x: 400; y: 0; label: "R2"; value: root.trigger(111); deadzone: root.triggerZone }

    GamepadKey { pad: root; x: 60;  y: 38; width: 130; height: 28; radius: 12; bit: 9;  label: "L1" }
    GamepadKey { pad: root; x: 370; y: 38; width: 130; height: 28; radius: 12; bit: 10; label: "R1" }

    Shape {
        anchors.fill: parent
        preferredRendererType: Shape.CurveRenderer

        ShapePath {
            fillColor: AppPalette.card
            strokeColor: AppPalette.borderHover
            strokeWidth: 2
            startX: 140; startY: 74
            PathLine { x: 420; y: 74 }
            PathCubic { control1X: 480; control1Y: 74;  control2X: 512; control2Y: 100; x: 522; y: 160 }
            PathCubic { control1X: 534; control1Y: 230; control2X: 548; control2Y: 320; x: 512; y: 352 }
            PathCubic { control1X: 484; control1Y: 376; control2X: 446; control2Y: 360; x: 428; y: 322 }
            PathCubic { control1X: 410; control1Y: 290; control2X: 396; control2Y: 282; x: 372; y: 282 }
            PathLine { x: 188; y: 282 }
            PathCubic { control1X: 164; control1Y: 282; control2X: 150; control2Y: 290; x: 132; y: 322 }
            PathCubic { control1X: 114; control1Y: 360; control2X: 76;  control2Y: 376; x: 48;  y: 352 }
            PathCubic { control1X: 12;  control1Y: 320; control2X: 26;  control2Y: 230; x: 38;  y: 160 }
            PathCubic { control1X: 48;  control1Y: 100; control2X: 80;  control2Y: 74;  x: 140; y: 74 }
        }
    }

    GamepadKey {
        pad: root
        bit: 20
        x: root.dualSense ? 186 : 196
        y: 82
        width: root.dualSense ? 188 : 168
        height: root.dualSense ? 86 : 74
        radius: 10
    }

    GamepadDpad { pad: root; x: 128 - width / 2; y: 158 - height / 2 }

    GamepadKey { pad: root; bit: 0; x: 432 - 18; y: 192 - 18; width: 36; height: 36; tinted: true; label: "cross";    tint: "#7EB2F2" }
    GamepadKey { pad: root; bit: 1; x: 466 - 18; y: 158 - 18; width: 36; height: 36; tinted: true; label: "circle";   tint: "#F06A6A" }
    GamepadKey { pad: root; bit: 2; x: 398 - 18; y: 158 - 18; width: 36; height: 36; tinted: true; label: "square";   tint: "#E68CC8" }
    GamepadKey { pad: root; bit: 3; x: 432 - 18; y: 124 - 18; width: 36; height: 36; tinted: true; label: "triangle"; tint: "#4FD1A5" }

    GamepadKey { pad: root; bit: 4; x: (root.dualSense ? 170 : 178) - 8; y: 92; width: 16; height: 30 }
    GamepadKey { pad: root; bit: 6; x: (root.dualSense ? 390 : 382) - 8; y: 92; width: 16; height: 30 }

    GamepadStick { pad: root; bit: 7; x: 206 - width / 2; y: 236 - height / 2; axisX: root.axis(0); axisY: root.axis(1); deadzone: root.stickZone }
    GamepadStick { pad: root; bit: 8; x: 354 - width / 2; y: 236 - height / 2; axisX: root.axis(2); axisY: root.axis(3); deadzone: root.stickZone }

    GamepadKey { pad: root; bit: 5;  x: 280 - 14; y: 236 - 14; width: 28; height: 28 }
    GamepadKey { pad: root; bit: 15; x: 280 - 14; y: 264 - 5;  width: 28; height: 10; visible: root.dualSense }
}
