add_library(kogger_scene2d STATIC)
qt_add_qml_module(kogger_scene2d
    URI scene2d VERSION 1.0 RESOURCE_PREFIX /qml
    QML_FILES
        ChartBox.qml
        ChartLevel.qml
        ChartLevelSingle.qml
        EchogramContactPopup.qml
        KChartLevelCapsule.qml
        KThemeSwitcher.qml
        Plot2D.qml)
