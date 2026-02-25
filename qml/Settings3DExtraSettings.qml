import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtQuick.Dialogs
import QtCore


// settings3D extra settings
MenuFrame {
    id: settings3DSettings

    property CheckButton settings3DCheckButton
    property alias showQualityLabel: showQualityLabelCheck.checked

    visible: Qt.platform.os === "android"
             ? (settings3DCheckButton.settings3DLongPressTriggered)
             : (settings3DCheckButton.hovered ||
                isHovered)

    z: settings3DSettings.visible
    Layout.alignment: Qt.AlignCenter

    onIsHoveredChanged: {
        if (Qt.platform.os === "android") {
            if (isHovered) {
                isHovered = false
            }
        }
    }

    onVisibleChanged: {
        if (visible) {
            focus = true;
        }
    }

    onFocusChanged: {
        if (Qt.platform.os === "android" && !focus) {
            Qt.callLater(function() {
                if (!settings3DSettings.focus) {
                    settings3DCheckButton.settings3DLongPressTriggered = false
                }
            })
        }
    }

    ColumnLayout {
        RowLayout {
            spacing: 16

            Rectangle {
                Layout.fillWidth: true
                height: 2
                color: "#808080"
            }

            CText {
                text: qsTr("3d scene settings")
            }

            Rectangle {
                Layout.fillWidth: true
                height: 2
                color: "#808080"
            }
        }

        CheckButton{
            id: cancelZoomViewButton
            iconSource: "qrc:/icons/ui/zoom_cancel.svg"
            backColor: theme.controlBackColor
            borderColor: theme.controlBackColor
            checkedBorderColor: theme.controlBorderColor
            checkable: false
            checked: false
            text: qsTr("Reset depth zoom")
            Layout.fillWidth: true
            visible: Qt.platform.os !== "android"

            onClicked: {
                Scene3dToolBarController.onCancelZoomButtonClicked()
            }
        }

        CheckButton {
            id: resetProcessingButton
            objectName: "resetProcessingButton"
            iconSource: "qrc:/icons/ui/refresh.svg"
            backColor: theme.controlBackColor
            borderColor: theme.controlBackColor
            checkedBorderColor: theme.controlBorderColor
            checkable: false
            checked: false
            text: qsTr("Reset surface")
            Layout.fillWidth: true

            onClicked: {
                Scene3dToolBarController.onResetProcessingButtonClicked()
            }

            onFocusChanged: {
                settings3DSettings.focus = true
            }
        }

        CheckButton {
            id: showQualityLabelCheck
            objectName: "showQualityLabelCheck"
            backColor: theme.controlBackColor
            borderColor: theme.controlBackColor
            checkedBorderColor: theme.controlBorderColor
            checked: false
            text: qsTr("Show surface quality")
            Layout.fillWidth: true

            onFocusChanged: {
                settings3DSettings.focus = true
            }

            Settings {
                property alias showQualityLabelCheck: showQualityLabelCheck.checked
            }
        }

        RowLayout { // forcing zoom
            visible: core.needForceZooming

            CheckButton {
                id: forceSingleZoomCheckButton
                objectName: "forceSingleZoomCheckButton"
                backColor: theme.controlBackColor
                borderColor: theme.controlBackColor
                checkedBorderColor: theme.controlBorderColor
                checked: false
                text: qsTr("Force zoom")
                Layout.fillWidth: true

                onToggled: {
                    Scene3dToolBarController.onForceSingleZoomCheckedChanged(checked)
                }

                onFocusChanged: {
                    settings3DSettings.focus = true
                }

                Component.onCompleted: {
                    Scene3dToolBarController.onForceSingleZoomCheckedChanged(checked)
                }

                Settings {
                    property alias forceSingleZoomCheckButton: forceSingleZoomCheckButton.checked
                }
            }

            SpinBoxCustom {
                id: forceSingleZoomSpinBox
                objectName: "forceSingleZoomSpinBox"
                from: 2
                to: 6
                stepSize: 1
                value: 5
                enabled: forceSingleZoomCheckButton.checked

                onValueChanged: {
                    Scene3dToolBarController.onForceSingleZoomValueChanged(value)
                }

                onFocusChanged: {
                    settings3DSettings.focus = true
                }

                Component.onCompleted: {
                    Scene3dToolBarController.onForceSingleZoomValueChanged(value)
                }

                Settings {
                    property alias forceSingleZoomSpinBox: forceSingleZoomSpinBox.value
                }
            }
        }

        RowLayout {
            CheckButton {
                id: syncLoupeCheckButton
                backColor: theme.controlBackColor
                borderColor: theme.controlBackColor
                checkedBorderColor: theme.controlBorderColor
                checked: false
                text: qsTr("Loupe")
                Layout.fillWidth: true

                onToggled: {
                    Scene3dToolBarController.onSyncLoupeVisibleChanged(checked)
                }

                onFocusChanged: {
                    settings3DSettings.focus = true
                }

                Component.onCompleted: {
                    Scene3dToolBarController.onSyncLoupeVisibleChanged(checked)
                }
            }

            RowLayout {
                visible: syncLoupeCheckButton.checked
                CText {
                    text: qsTr("size")
                }
                SpinBoxCustom {
                    id: syncLoupeSizeSpinBox
                    from: 1
                    to: 3
                    stepSize: 1
                    value: 1

                    onValueChanged: {
                        Scene3dToolBarController.onSyncLoupeSizeChanged(value)
                    }

                    onFocusChanged: {
                        settings3DSettings.focus = true
                    }

                    Component.onCompleted: {
                        Scene3dToolBarController.onSyncLoupeSizeChanged(value)
                    }
                }
            }

            RowLayout {
                visible: syncLoupeCheckButton.checked
                CText {
                    text: qsTr("zoom")
                }
                SpinBoxCustom {
                    id: syncLoupeZoomSpinBox
                    from: 1
                    to: 3
                    stepSize: 1
                    value: 1

                    onValueChanged: {
                        Scene3dToolBarController.onSyncLoupeZoomChanged(value)
                    }

                    onFocusChanged: {
                        settings3DSettings.focus = true
                    }

                    Component.onCompleted: {
                        Scene3dToolBarController.onSyncLoupeZoomChanged(value)
                    }
                }
            }

            Settings {
                property alias syncLoupeCheckButton: syncLoupeCheckButton.checked
                property alias syncLoupeSizeSpinBox: syncLoupeSizeSpinBox.value
                property alias syncLoupeZoomSpinBox: syncLoupeZoomSpinBox.value
            }
        }

        CheckButton {
            id: isNorthViewButton
            objectName: "isNorthViewButton"
            backColor: theme.controlBackColor
            borderColor: theme.controlBackColor
            checkedBorderColor: theme.controlBorderColor
            iconSource: "qrc:/icons/ui/location_pin.svg"
            text: qsTr("North mode")
            Layout.fillWidth: true

            onToggled: {
                Scene3dToolBarController.onIsNorthLocationButtonChanged(checked)
            }

            onFocusChanged: {
                settings3DSettings.focus = true
            }

            Component.onCompleted: {
                Scene3dToolBarController.onIsNorthLocationButtonChanged(checked)
            }

            Settings {
                property alias isNorthViewButton: isNorthViewButton.checked
            }
        }

        CheckButton {
            id: selectionToolButton
            objectName: "selectionToolButton"
            backColor: theme.controlBackColor
            borderColor: theme.controlBackColor
            checkedBorderColor: theme.controlBorderColor
            iconSource: "qrc:/icons/ui/click.svg"
            text: qsTr("Sync echogram")
            Layout.fillWidth: true

            onToggled: {
                Scene3dToolBarController.onBottomTrackVertexEditingModeButtonChecked(checked)
            }

            onFocusChanged: {
                settings3DSettings.focus = true
            }

            Component.onCompleted: {
                Scene3dToolBarController.onBottomTrackVertexEditingModeButtonChecked(checked)
            }

            Settings {
                property alias selectionToolButton: selectionToolButton.checked
            }
        }

        // CheckButton {
        //     id: trackLastDataCheckButton
        //     objectName: "trackLastDataCheckButton"
        //     backColor: theme.controlBackColor
        //     borderColor: theme.controlBackColor
        //     checkedBorderColor: theme.controlBorderColor
        //     checked: false
        //     iconSource: "qrc:/icons/ui/location.svg"
        //     text: qsTr("Follow last location")
        //     Layout.fillWidth: true

        //     onToggled: {
        //         Scene3dToolBarController.onTrackLastDataCheckButtonCheckedChanged(checked)
        //     }

        //     onFocusChanged: {
        //         settings3DSettings.focus = true
        //     }

        //     Component.onCompleted: {
        //         Scene3dToolBarController.onTrackLastDataCheckButtonCheckedChanged(checked)
        //     }

        //     Settings {
        //         property alias trackLastDataCheckButton: trackLastDataCheckButton.checked
        //     }
        // }

        RowLayout {
            CheckButton {
                id: gridCheckButton
                objectName: "gridCheckButton"
                backColor: theme.controlBackColor
                borderColor: theme.controlBackColor
                checkedBorderColor: theme.controlBorderColor
                checked: false
                iconSource: "qrc:/icons/ui/grid_4x4.svg"
                text: qsTr("Grid")
                Layout.fillWidth: true

                onToggled: {
                    Scene3dToolBarController.onGridVisibilityCheckedChanged(checked)
                }

                onFocusChanged: {
                    settings3DSettings.focus = true
                }

                Component.onCompleted: {
                    Scene3dToolBarController.onGridVisibilityCheckedChanged(checked)
                }

                Settings {
                    property alias gridCheckButton: gridCheckButton.checked
                }
            }

            ColumnLayout {
                visible: gridCheckButton.checked

                CheckButton {
                    id: gridTypeCheckButton
                    objectName: "gridTypeCheckButton"
                    backColor: theme.controlBackColor
                    borderColor: theme.controlBackColor
                    checkedBorderColor: theme.controlBorderColor
                    checked: false
                    //iconSource: "qrc:/icons/ui/gps.svg"
                    text: qsTr("Circle")
                    Layout.fillWidth: true

                    onToggled: {
                        Scene3dToolBarController.onPlaneGridTypeChanged(!checked)
                    }
                    onFocusChanged: {
                        settings3DSettings.focus = true
                    }
                    Component.onCompleted: {
                        Scene3dToolBarController.onPlaneGridTypeChanged(!checked)
                    }
                    Settings {
                        property alias gridTypeCheckButton: gridTypeCheckButton.checked
                    }
                }

                CheckButton {
                    visible: gridTypeCheckButton.checked

                    id: gridLabelsCheckButton
                    objectName: "gridLabelsCheckButton"
                    backColor: theme.controlBackColor
                    borderColor: theme.controlBackColor
                    checkedBorderColor: theme.controlBorderColor
                    checked: true
                    //iconSource: "qrc:/icons/ui/gps.svg"
                    text: qsTr("Labels")
                    Layout.fillWidth: true

                    onToggled: {
                        Scene3dToolBarController.onPlaneGridCircleGridLabelsChanged(checked)
                    }
                    onFocusChanged: {
                        settings3DSettings.focus = true
                    }
                    Component.onCompleted: {
                        Scene3dToolBarController.onPlaneGridCircleGridLabelsChanged(checked)
                    }
                    Settings {
                        property alias gridLabelsCheckButton: gridLabelsCheckButton.checked
                    }
                }

                RowLayout {
                    visible: gridTypeCheckButton.checked

                    ColumnLayout {
                        CText {
                            text: qsTr("Size:")
                        }
                        CText {
                            text: qsTr("Step:")
                        }
                        CText {
                            text: qsTr("Angle:")
                        }
                    }

                    Item {
                        Layout.fillWidth: true
                    }

                    ColumnLayout {
                        SpinBoxCustom {
                            id: circleGridSizeSpinBox
                            from: 1
                            to: 3
                            stepSize: 1
                            value: 1

                            onValueChanged: {
                                Scene3dToolBarController.onPlaneGridCircleGridSizeChanged(value)
                            }
                            onFocusChanged: {
                                settings3DSettings.focus = true
                            }
                            Component.onCompleted: {
                                Scene3dToolBarController.onPlaneGridCircleGridSizeChanged(value)
                            }
                            Settings {
                                property alias circleGridSizeSpinBox: circleGridSizeSpinBox.value
                            }
                        }

                        SpinBoxCustom {
                            id: circleGridStepSpinBox
                            from: 1
                            to: 20
                            stepSize: 1
                            value: 1

                            onValueChanged: {
                                Scene3dToolBarController.onPlaneGridCircleGridStepChanged(value)
                            }
                            onFocusChanged: {
                                settings3DSettings.focus = true
                            }
                            Component.onCompleted: {
                                Scene3dToolBarController.onPlaneGridCircleGridStepChanged(value)
                            }
                            Settings {
                                property alias circleGridStepSpinBox: circleGridStepSpinBox.value
                            }
                        }

                        SpinBoxCustom {
                            id: circleGridAngleSpinBox
                            from: 1
                            to: 5
                            stepSize: 1
                            value: 1

                            onValueChanged: {
                                Scene3dToolBarController.onPlaneGridCircleGridAngleChanged(value)
                            }
                            onFocusChanged: {
                                settings3DSettings.focus = true
                            }
                            Component.onCompleted: {
                                Scene3dToolBarController.onPlaneGridCircleGridAngleChanged(value)
                            }
                            Settings {
                                property alias circleGridAngleSpinBox: circleGridAngleSpinBox.value
                            }
                        }
                    }
                }
            }
        }

        CheckButton {
            id: shadowEnabledCheckButton
            objectName: "shadowEnabledCheckButton"
            backColor: theme.controlBackColor
            borderColor: theme.controlBackColor
            checkedBorderColor: theme.controlBorderColor
            checked: true
            text: qsTr("Shadows")
            Layout.fillWidth: true

            onToggled: {
                Scene3dToolBarController.onShadowsEnabledChanged(checked)
            }

            onFocusChanged: {
                settings3DSettings.focus = true
            }

            Component.onCompleted: {
                Scene3dToolBarController.onShadowsEnabledChanged(checked)
            }
        }

        RowLayout {
            id: shadowSettingsRow
            Layout.fillWidth: true
            visible: false //shadowEnabledCheckButton.checked

            function formatShadowValue(value) {
                return Number(value).toLocaleString(Qt.locale(), 'f', 2)
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 4

                RowLayout {
                    Layout.fillWidth: true

                    CText {
                        Layout.preferredWidth: 84
                        text: qsTr("Vector X:")
                    }

                    Slider {
                        id: shadowVectorXSlider
                        objectName: "shadowVectorXSpinBox"
                        Layout.fillWidth: true
                        from: -1.0
                        to: 1.0
                        stepSize: 0.01
                        value: 0.40
                        enabled: shadowEnabledCheckButton.checked

                        onPressedChanged: {
                            if (pressed) {
                                settings3DSettings.focus = true
                            }
                        }

                        onValueChanged: {
                            Scene3dToolBarController.onShadowVectorXChanged(value)
                        }

                        Component.onCompleted: {
                            Scene3dToolBarController.onShadowVectorXChanged(value)
                        }
                    }

                    CText {
                        Layout.preferredWidth: 48
                        horizontalAlignment: Text.AlignRight
                        text: shadowSettingsRow.formatShadowValue(shadowVectorXSlider.value)
                    }
                }

                RowLayout {
                    Layout.fillWidth: true

                    CText {
                        Layout.preferredWidth: 84
                        text: qsTr("Vector Y:")
                    }

                    Slider {
                        id: shadowVectorYSlider
                        objectName: "shadowVectorYSpinBox"
                        Layout.fillWidth: true
                        from: -1.0
                        to: 1.0
                        stepSize: 0.01
                        value: 0.40
                        enabled: shadowEnabledCheckButton.checked

                        onPressedChanged: {
                            if (pressed) {
                                settings3DSettings.focus = true
                            }
                        }

                        onValueChanged: {
                            Scene3dToolBarController.onShadowVectorYChanged(value)
                        }

                        Component.onCompleted: {
                            Scene3dToolBarController.onShadowVectorYChanged(value)
                        }
                    }

                    CText {
                        Layout.preferredWidth: 48
                        horizontalAlignment: Text.AlignRight
                        text: shadowSettingsRow.formatShadowValue(shadowVectorYSlider.value)
                    }
                }

                RowLayout {
                    Layout.fillWidth: true

                    CText {
                        Layout.preferredWidth: 84
                        text: qsTr("Vector Z:")
                    }

                    Slider {
                        id: shadowVectorZSlider
                        objectName: "shadowVectorZSpinBox"
                        Layout.fillWidth: true
                        from: -1.0
                        to: 1.0
                        stepSize: 0.01
                        value: 0.40
                        enabled: shadowEnabledCheckButton.checked

                        onPressedChanged: {
                            if (pressed) {
                                settings3DSettings.focus = true
                            }
                        }

                        onValueChanged: {
                            Scene3dToolBarController.onShadowVectorZChanged(value)
                        }

                        Component.onCompleted: {
                            Scene3dToolBarController.onShadowVectorZChanged(value)
                        }
                    }

                    CText {
                        Layout.preferredWidth: 48
                        horizontalAlignment: Text.AlignRight
                        text: shadowSettingsRow.formatShadowValue(shadowVectorZSlider.value)
                    }
                }

                RowLayout {
                    Layout.fillWidth: true

                    CText {
                        Layout.preferredWidth: 84
                        text: qsTr("Ambient:")
                    }

                    Slider {
                        id: shadowAmbientSlider
                        objectName: "shadowAmbientSpinBox"
                        Layout.fillWidth: true
                        from: 0.0
                        to: 1.0
                        stepSize: 0.05
                        value: 0.35
                        enabled: shadowEnabledCheckButton.checked

                        onPressedChanged: {
                            if (pressed) {
                                settings3DSettings.focus = true
                            }
                        }

                        onValueChanged: {
                            Scene3dToolBarController.onShadowAmbientChanged(value)
                        }

                        Component.onCompleted: {
                            Scene3dToolBarController.onShadowAmbientChanged(value)
                        }
                    }

                    CText {
                        Layout.preferredWidth: 48
                        horizontalAlignment: Text.AlignRight
                        text: shadowSettingsRow.formatShadowValue(shadowAmbientSlider.value)
                    }
                }

                RowLayout {
                    Layout.fillWidth: true

                    CText {
                        Layout.preferredWidth: 84
                        text: qsTr("Highlight:")
                    }

                    Slider {
                        id: shadowHighlightSlider
                        objectName: "shadowHighlightSpinBox"
                        Layout.fillWidth: true
                        from: 0.0
                        to: 1.0
                        stepSize: 0.05
                        value: 0.70
                        enabled: shadowEnabledCheckButton.checked

                        onPressedChanged: {
                            if (pressed) {
                                settings3DSettings.focus = true
                            }
                        }

                        onValueChanged: {
                            Scene3dToolBarController.onShadowHighlightChanged(value)
                        }

                        Component.onCompleted: {
                            Scene3dToolBarController.onShadowHighlightChanged(value)
                        }
                    }

                    CText {
                        Layout.preferredWidth: 48
                        horizontalAlignment: Text.AlignRight
                        text: shadowSettingsRow.formatShadowValue(shadowHighlightSlider.value)
                    }
                }
            }

            Settings {
                property alias shadowEnabledCheckButton: shadowEnabledCheckButton.checked
                property alias shadowVectorXSpinBox: shadowVectorXSlider.value
                property alias shadowVectorYSpinBox: shadowVectorYSlider.value
                property alias shadowVectorZSpinBox: shadowVectorZSlider.value
                property alias shadowAmbientSpinBox: shadowAmbientSlider.value
                property alias shadowHighlightSpinBox: shadowHighlightSlider.value
            }
        }

        CheckButton {
            id: navigationArrowCheckButton
            objectName: "navigationArrowCheckButton"
            backColor: theme.controlBackColor
            borderColor: theme.controlBackColor
            checkedBorderColor: theme.controlBorderColor
            checked: true
            iconSource: "qrc:/icons/ui/speedboat.svg"
            text: qsTr("Boat")
            Layout.fillWidth: true

            onToggled: {
                NavigationArrowControlMenuController.onVisibilityCheckBoxCheckedChanged(checked)
            }

            onFocusChanged: {
                settings3DSettings.focus = true
            }

            Component.onCompleted: {
                NavigationArrowControlMenuController.onVisibilityCheckBoxCheckedChanged(checked)
            }

            Settings {
                property alias navigationArrowCheckButton: navigationArrowCheckButton.checked
            }
        }

        RowLayout {
            CheckButton {
                id: compassCheckButton
                objectName: "compassCheckButton"
                backColor: theme.controlBackColor
                borderColor: theme.controlBackColor
                checkedBorderColor: theme.controlBorderColor
                checked: true
                iconSource: "qrc:/icons/ui/compass.svg"
                text: qsTr("Compass")
                Layout.fillWidth: true

                onToggled: {
                    Scene3dToolBarController.onCompassButtonChanged(checked)
                }

                onFocusChanged: {
                    settings3DSettings.focus = true
                }

                Component.onCompleted: {
                    Scene3dToolBarController.onCompassButtonChanged(checked)
                }

                Settings {
                    property alias compassCheckButton: compassCheckButton.checked
                }
            }

            RowLayout {
                visible: compassCheckButton.checked

                ColumnLayout {
                    CText {
                        text: qsTr("Pos:")
                    }
                    CText {
                        text: qsTr("Size:")
                    }
                }

                Item {
                    Layout.fillWidth: true
                }

                ColumnLayout {
                    SpinBoxCustom {
                        id: compassPosSpinBox
                        from: 1
                        to: 3
                        stepSize: 1
                        value: 1

                        onValueChanged: {
                            Scene3dToolBarController.onCompassPosChanged(value)
                        }
                        onFocusChanged: {
                            settings3DSettings.focus = true
                        }
                        Component.onCompleted: {
                            Scene3dToolBarController.onCompassPosChanged(value)
                        }
                        Settings {
                            property alias compassPosSpinBox: compassPosSpinBox.value
                        }
                    }

                    SpinBoxCustom {
                        id: compassSizeSpinBox
                        from: 1
                        to: 5
                        stepSize: 1
                        value: 1

                        onValueChanged: {
                            Scene3dToolBarController.onCompassSizeChanged(value)
                        }
                        onFocusChanged: {
                            settings3DSettings.focus = true
                        }
                        Component.onCompleted: {
                            Scene3dToolBarController.onCompassSizeChanged(value)
                        }
                        Settings {
                            property alias compassSizeSpinBox: compassSizeSpinBox.value
                        }
                    }
                }
            }
        }

    }
}
