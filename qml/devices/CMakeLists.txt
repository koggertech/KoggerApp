add_library(kogger_devices STATIC)
qt_add_qml_module(kogger_devices
    URI devices VERSION 1.0 RESOURCE_PREFIX /qml
    QML_FILES
        ConnectionViewer.qml
        DevSettingsBox.qml
        DeviceItem.qml
        DeviceSettingsViewer.qml
        DopplerBox.qml
        ExportDist.qml
        ExportSettings.qml
        FactoryBox.qml
        FilesViewer.qml
        FlashBox.qml
        RecorderBox.qml
        ServoBox.qml
        SonarBox.qml
        USBLBox.qml
        UpgradeBox.qml)
