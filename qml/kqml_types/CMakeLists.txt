add_library(kogger_kqml_types STATIC)
set_source_files_properties(
    ZOrder.qml AppPalette.qml Tokens.qml Anim.qml DataFieldCatalog.qml DeviceParamCatalog.qml ToolTipFactory.qml
    PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(kogger_kqml_types
    URI kqml_types VERSION 1.0 RESOURCE_PREFIX /qml
    QML_FILES
        Anim.qml
        AppPalette.qml
        DataFieldCatalog.qml
        DeviceParamCatalog.qml
        IdleFade.qml
        KBatteryGlyph.qml
        KBatteryLevel.qml
        KButton.qml
        KCheckBox.qml
        KCircleIconButton.qml
        KCloseBadge.qml
        KCoachArrow.qml
        KCoachBubble.qml
        KCombo.qml
        KCountdownButton.qml
        KDragBar.qml
        KFocusRing.qml
        KGamepadIcon.qml
        KIcon.qml
        KIsland.qml
        KIslandRow.qml
        KIslandSection.qml
        KLazyToolTip.qml
        KParamGrid.qml
        KScrollBar.qml
        KSlider.qml
        KSpinBox.qml
        KSpotlight.qml
        KSwitch.qml
        KTabBar.qml
        KTapArea.qml
        KToggleRow.qml
        KToolIconButton.qml
        KToolTip.qml
        KTutorialAnchor.qml
        Tokens.qml
        ToolTipFactory.qml
        ZOrder.qml
        UsblFieldLogic.js)
