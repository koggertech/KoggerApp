add_library(kogger_devices STATIC)
qt_add_qml_module(kogger_devices
    URI devices VERSION 1.0 RESOURCE_PREFIX /qml
    QML_FILES
        ConnectionViewer.qml
        DeviceTopologyView.qml
        FilesViewer.qml)
