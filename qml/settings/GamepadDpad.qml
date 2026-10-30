import QtQuick
import kqml_types 1.0

Item {
    id: dpad

    property Item pad: null
    property real key: 24
    property bool split: false
    property real tipGap: 4
    readonly property real span: key * 3
    readonly property real arm: span / 2 - tipGap

    width: split ? span : key * 3
    height: width

    GamepadKey { visible: !dpad.split; pad: dpad.pad; bit: 11; x: dpad.key; y: 0; width: dpad.key; height: dpad.key; radius: 4 }
    GamepadKey { visible: !dpad.split; pad: dpad.pad; bit: 12; x: dpad.key; y: dpad.key * 2; width: dpad.key; height: dpad.key; radius: 4 }
    GamepadKey { visible: !dpad.split; pad: dpad.pad; bit: 13; x: 0; y: dpad.key; width: dpad.key; height: dpad.key; radius: 4 }
    GamepadKey { visible: !dpad.split; pad: dpad.pad; bit: 14; x: dpad.key * 2; y: dpad.key; width: dpad.key; height: dpad.key; radius: 4 }
    Rectangle { visible: !dpad.split; x: dpad.key; y: dpad.key; width: dpad.key; height: dpad.key; color: AppPalette.controlRaised }

    GamepadArrowKey {
        visible: dpad.split; pad: dpad.pad; bit: 11
        width: dpad.key; height: dpad.arm
        x: (dpad.width - width) / 2; y: 0
    }
    GamepadArrowKey {
        visible: dpad.split; pad: dpad.pad; bit: 12
        width: dpad.key; height: dpad.arm
        x: (dpad.width - width) / 2; y: dpad.height - height
        rotation: 180
    }
    GamepadArrowKey {
        visible: dpad.split; pad: dpad.pad; bit: 13
        width: dpad.key; height: dpad.arm
        x: dpad.arm / 2 - width / 2; y: (dpad.height - height) / 2
        rotation: -90
    }
    GamepadArrowKey {
        visible: dpad.split; pad: dpad.pad; bit: 14
        width: dpad.key; height: dpad.arm
        x: dpad.width - dpad.arm / 2 - width / 2; y: (dpad.height - height) / 2
        rotation: 90
    }
}
