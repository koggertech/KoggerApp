add_library(kogger_kqml_types STATIC)
set_source_files_properties(
    ZOrder.qml AppPalette.qml Tokens.qml Anim.qml
    PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(kogger_kqml_types
    URI kqml_types VERSION 1.0 RESOURCE_PREFIX /qml
    QML_FILES
        Anim.qml
        AppPalette.qml
        IdleFade.qml
        KButton.qml
        KCheckBox.qml
        KCircleIconButton.qml
        KCloseBadge.qml
        KCombo.qml
        KDragBar.qml
        KFocusRing.qml
        KParamGrid.qml
        KSlider.qml
        KSpinBox.qml
        KSwitch.qml
        KTabBar.qml
        KTapArea.qml
        KToggleRow.qml
        KToolTip.qml
        Tokens.qml
        ZOrder.qml)
