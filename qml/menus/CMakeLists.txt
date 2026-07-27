add_library(kogger_menus STATIC)
qt_add_qml_module(kogger_menus
    URI menus VERSION 1.0 RESOURCE_PREFIX /qml
    QML_FILES
        MenuFrame.qml)
