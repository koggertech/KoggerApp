import QtQuick
import kqml_types 1.0

Item {
    id: dpad

    property Item pad: null
    property real key: 24

    width: key * 3
    height: key * 3

    GamepadKey { pad: dpad.pad; bit: 11; x: dpad.key; y: 0; width: dpad.key; height: dpad.key; radius: 4 }
    GamepadKey { pad: dpad.pad; bit: 12; x: dpad.key; y: dpad.key * 2; width: dpad.key; height: dpad.key; radius: 4 }
    GamepadKey { pad: dpad.pad; bit: 13; x: 0; y: dpad.key; width: dpad.key; height: dpad.key; radius: 4 }
    GamepadKey { pad: dpad.pad; bit: 14; x: dpad.key * 2; y: dpad.key; width: dpad.key; height: dpad.key; radius: 4 }
    Rectangle { x: dpad.key; y: dpad.key; width: dpad.key; height: dpad.key; color: AppPalette.controlRaised }
}
