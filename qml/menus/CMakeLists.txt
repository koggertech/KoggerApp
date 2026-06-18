add_library(kogger_menus STATIC)
qt_add_qml_module(kogger_menus
    URI menus VERSION 1.0 RESOURCE_PREFIX /qml
    QML_FILES
        Console.qml
        MainMenuBar.qml
        MenuBlock.qml
        MenuButton.qml
        MenuFrame.qml
        MenuRow.qml
        MenuScroll.qml
        MenuViewer.qml
        ParamGroup.qml
        ParamSetup.qml
        TitleMeasurement.qml
        TitleMenuBox.qml
        TitleSetup.qml)
