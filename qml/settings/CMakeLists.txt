add_library(kogger_settings STATIC)
qt_add_qml_module(kogger_settings
    URI settings VERSION 1.0 RESOURCE_PREFIX /qml
    QML_FILES
        DisplaySettings.qml
        DisplaySettingsViewer.qml
        ExtraInfoPanel.qml
        GeoJsonPanel.qml
        HotkeysDialog.qml)
