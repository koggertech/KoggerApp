add_library(kogger_controls STATIC)
qt_add_qml_module(kogger_controls
    URI controls VERSION 1.0 RESOURCE_PREFIX /qml
    QML_FILES
        BackStyle.qml
        CButton.qml
        CCheck.qml
        CCombo.qml
        CMouseOpacityArea.qml
        CPopup.qml
        CProgress.qml
        CRButton.qml
        CRadioDel.qml
        CSlider.qml
        CText.qml
        CTextField.qml
        CheckButton.qml
        ComboBackStyle.qml
        SpinBoxCustom.qml
        StyleSet.qml
        TabBackStyle.qml)
