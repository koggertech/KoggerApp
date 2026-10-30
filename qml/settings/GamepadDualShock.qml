import QtQuick
import QtQuick.Shapes
import kqml_types 1.0

GamepadPad {
    id: root

    readonly property bool dualSense: !!view && view.family === "ps5"
    readonly property real padWidth: dualSense ? 176 : 164
    readonly property real padLeft: 280 - padWidth / 2
    readonly property real sideGap: 8
    readonly property real sideKeyWidth: 12
    readonly property real sideLeftX: padLeft - sideGap - sideKeyWidth / 2
    readonly property real sideRightX: padLeft + padWidth + sideGap + sideKeyWidth / 2
    readonly property color lightColor: "#4C7FD9"
    readonly property real lightThickness: 4
    readonly property real padTop: dualSense ? 78 : 86
    readonly property real padBottom: padTop + (dualSense ? 92 : 78)
    readonly property real dpadX: 110
    readonly property real faceX: 450

    GamepadTrigger { x: 70;  y: 0; label: "L2"; value: root.trigger(110); deadzone: root.triggerZone }
    GamepadTrigger { x: 400; y: 0; label: "R2"; value: root.trigger(111); deadzone: root.triggerZone }

    GamepadKey { pad: root; x: 64;  y: 40; width: 124; height: 22; radius: 9; bit: 9;  label: "L1" }
    GamepadKey { pad: root; x: 372; y: 40; width: 124; height: 22; radius: 9; bit: 10; label: "R1" }

    Shape {
        anchors.fill: parent
        preferredRendererType: Shape.CurveRenderer

        ShapePath {
            fillColor: AppPalette.card
            strokeColor: AppPalette.borderHover
            strokeWidth: 2
            joinStyle: ShapePath.RoundJoin
            startX: 150; startY: 72
            PathLine { x: 410; y: 72 }
            PathCubic { control1X: 458; control1Y: 70;  control2X: 494; control2Y: 80;  x: 512; y: 112 }
            PathCubic { control1X: 532; control1Y: 150; control2X: 540; control2Y: 196; x: 544; y: 240 }
            PathCubic { control1X: 550; control1Y: 286; control2X: 554; control2Y: 328; x: 530; y: 346 }
            PathCubic { control1X: 506; control1Y: 364; control2X: 468; control2Y: 354; x: 456; y: 320 }
            PathCubic { control1X: 446; control1Y: 290; control2X: 424; control2Y: 270; x: 392; y: 268 }
            PathLine { x: 168; y: 268 }
            PathCubic { control1X: 136; control1Y: 270; control2X: 114; control2Y: 290; x: 104; y: 320 }
            PathCubic { control1X: 92;  control1Y: 354; control2X: 54;  control2Y: 364; x: 30;  y: 346 }
            PathCubic { control1X: 6;   control1Y: 328; control2X: 10;  control2Y: 286; x: 16;  y: 240 }
            PathCubic { control1X: 20;  control1Y: 196; control2X: 28;  control2Y: 150; x: 48;  y: 112 }
            PathCubic { control1X: 66;  control1Y: 80;  control2X: 102; control2Y: 70;  x: 150; y: 72 }
        }
    }

    Rectangle {
        visible: !root.dualSense
        x: root.padLeft + 6
        y: 77
        width: root.padWidth - 12
        height: root.lightThickness
        radius: root.lightThickness / 2
        color: root.lightColor
        opacity: 0.8
    }
    component SideLight: Rectangle {
        width: root.lightThickness
        height: root.padBottom - y
        radius: root.lightThickness / 2
        color: root.lightColor
        opacity: 0.8
        visible: root.dualSense
    }

    SideLight { x: root.sideLeftX - width / 2; y: 124 }
    SideLight { x: root.sideRightX - width / 2; y: 124 }

    GamepadKey {
        pad: root
        bit: 20
        x: root.padLeft
        y: root.padTop
        width: root.padWidth
        height: root.padBottom - root.padTop
        radius: 12
    }

    GamepadDpad { pad: root; split: true; key: 26; x: root.dpadX - width / 2; y: 168 - height / 2 }

    GamepadKey { pad: root; bit: 0; x: root.faceX - 18;      y: 202 - 18; width: 36; height: 36; tinted: true; label: "cross";    tint: "#7EB2F2" }
    GamepadKey { pad: root; bit: 1; x: root.faceX + 36 - 18; y: 166 - 18; width: 36; height: 36; tinted: true; label: "circle";   tint: "#F06A6A" }
    GamepadKey { pad: root; bit: 2; x: root.faceX - 36 - 18; y: 166 - 18; width: 36; height: 36; tinted: true; label: "square";   tint: "#E68CC8" }
    GamepadKey { pad: root; bit: 3; x: root.faceX - 18;      y: 130 - 18; width: 36; height: 36; tinted: true; label: "triangle"; tint: "#4FD1A5" }

    GamepadKey { pad: root; bit: 4; x: root.sideLeftX - width / 2; y: 92; width: root.sideKeyWidth; height: 26 }
    GamepadKey { pad: root; bit: 6; x: root.sideRightX - width / 2; y: 92; width: root.sideKeyWidth; height: 26 }

    GamepadStick { pad: root; bit: 7; x: 206 - width / 2; y: 222 - height / 2; axisX: root.axis(0); axisY: root.axis(1); deadzone: root.stickZone }
    GamepadStick { pad: root; bit: 8; x: 354 - width / 2; y: 222 - height / 2; axisX: root.axis(2); axisY: root.axis(3); deadzone: root.stickZone }

    GamepadKey { pad: root; bit: 5;  x: 280 - 13; y: root.dualSense ? 201 : 209; width: 26; height: 26 }
    GamepadKey { pad: root; bit: 15; x: 280 - 13; y: 236; width: 26; height: 8; visible: root.dualSense }
}
