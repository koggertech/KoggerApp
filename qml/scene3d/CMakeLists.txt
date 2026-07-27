add_library(kogger_scene3d STATIC)
qt_add_qml_module(kogger_scene3d
    URI scene3d VERSION 1.0 RESOURCE_PREFIX /qml
    QML_FILES
        Scene3DGeometryPanel.qml
        Scene3DLayerControl.qml
        Scene3DRightToolbar.qml
        Scene3DToolbar.qml)
