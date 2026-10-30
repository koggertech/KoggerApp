add_library(kogger_settings STATIC)
qt_add_qml_module(kogger_settings
    URI settings VERSION 1.0 RESOURCE_PREFIX /qml
    QML_FILES
        HotkeysDialog.qml
        GamepadTestDialog.qml
        GamepadPad.qml
        GamepadKey.qml
        GamepadTrigger.qml
        GamepadStick.qml
        GamepadDpad.qml
        GamepadArrowKey.qml
        GamepadSymbol.qml
        GamepadXbox.qml
        GamepadDualShock.qml
        GamepadSteam.qml
        GamepadGallery.qml)
