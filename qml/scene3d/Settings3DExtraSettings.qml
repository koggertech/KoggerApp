import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtCore
import kqml_types 1.0
import "../controls"
import "../menus"
import "../scene2d"


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
    backgroundColor: AppPalette.bg
    horizontalMargins: Tokens.spaceLg
    verticalMargins: Tokens.spaceLg
    spacing: Tokens.spaceMd

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
        else {
            Scene3dToolBarController.onSyncLoupeZoomAdjustingChanged(false)
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
        spacing: Tokens.spaceMd

        RowLayout {
            Layout.fillWidth: true
            spacing: Tokens.spaceMd

            Rectangle { Layout.fillWidth: true; height: 1; color: AppPalette.border }
            Text {
                text: qsTr("3d scene settings")
                color: AppPalette.textSecond
                font.pixelSize: Tokens.fontMd
                font.bold: true
            }
            Rectangle { Layout.fillWidth: true; height: 1; color: AppPalette.border }
        }

        KButton {
            id: cancelZoomViewButton
            text: qsTr("Reset depth zoom")
            Layout.fillWidth: true
            visible: Qt.platform.os !== "android"

            onClicked: {
                Scene3dToolBarController.onCancelZoomButtonClicked()
            }
        }

        KButton {
            id: resetProcessingButton
            objectName: "resetProcessingButton"
            text: qsTr("Reset surface")
            Layout.fillWidth: true

            onClicked: {
                Scene3dToolBarController.onResetProcessingButtonClicked()
            }

            onFocusChanged: {
                settings3DSettings.focus = true
            }
        }

        KToggleRow {
            id: showQualityLabelCheck
            objectName: "showQualityLabelCheck"
            label: qsTr("Show surface quality")

            onToggled: function(v) {
                settings3DSettings.focus = true
            }

            Settings {
                property alias showQualityLabelCheck: showQualityLabelCheck.checked
            }
        }

        RowLayout {
            visible: core.needForceZooming
            Layout.fillWidth: true
            spacing: Tokens.spaceMd

            KToggleRow {
                id: forceSingleZoomCheckButton
                objectName: "forceSingleZoomCheckButton"
                label: qsTr("Force zoom")
                Layout.fillWidth: true

                onToggled: function(v) {
                    Scene3dToolBarController.onForceSingleZoomCheckedChanged(v)
                }

                Component.onCompleted: {
                    Scene3dToolBarController.onForceSingleZoomCheckedChanged(checked)
                }

                Settings {
                    property alias forceSingleZoomCheckButton: forceSingleZoomCheckButton.checked
                }
            }

            KSpinBox {
                id: forceSingleZoomSpinBox
                objectName: "forceSingleZoomSpinBox"
                from: 2
                to: 6
                stepSize: 1
                value: 5
                enabled: forceSingleZoomCheckButton.checked

                onValueModified: function(v) {
                    Scene3dToolBarController.onForceSingleZoomValueChanged(v)
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
            Layout.fillWidth: true
            spacing: Tokens.spaceMd

            KToggleRow {
                id: syncLoupeCheckButton
                label: qsTr("Loupe")
                Layout.fillWidth: true

                onToggled: function(v) {
                    Scene3dToolBarController.onSyncLoupeVisibleChanged(v)
                }

                Component.onCompleted: {
                    Scene3dToolBarController.onSyncLoupeVisibleChanged(checked)
                }
            }
            RowLayout {
                visible: syncLoupeCheckButton.checked
                spacing: Tokens.spaceXs
                Text {
                    text: qsTr("size")
                    color: AppPalette.textSecond
                    font.pixelSize: Tokens.fontMd
                }
                KSpinBox {
                    id: syncLoupeSizeSpinBox
                    from: 1
                    to: 3
                    stepSize: 1
                    value: 1

                    onValueModified: function(v) {
                        Scene3dToolBarController.onSyncLoupeSizeChanged(v)
                    }

                    Component.onCompleted: {
                        Scene3dToolBarController.onSyncLoupeSizeChanged(value)
                    }
                }
            }

            RowLayout {
                visible: syncLoupeCheckButton.checked
                spacing: Math.max(Tokens.spaceSm, Math.round(Tokens.controlHMd * 0.2))

                Text {
                    text: qsTr("zoom")
                    color: AppPalette.textSecond
                    font.pixelSize: Tokens.fontMd
                }

                ChartLevelSingle {
                    id: syncLoupeZoomSpinBox
                    Layout.fillWidth: true
                    Layout.preferredWidth: Tokens.controlHMd * 5
                    from: 0
                    to: 300
                    stepSize: 1
                    value: 100

                    onValueChanged: {
                        Scene3dToolBarController.onSyncLoupeZoomChanged(Math.round(value))
                    }

                    onFocusChanged: {
                        settings3DSettings.focus = true
                    }

                    onPressedChanged: {
                        Scene3dToolBarController.onSyncLoupeZoomAdjustingChanged(pressed)
                        if (pressed) {
                            settings3DSettings.forceActiveFocus()
                        }
                    }

                    onMoved: {
                        Scene3dToolBarController.onSyncLoupeZoomAdjustingChanged(true)
                        settings3DSettings.forceActiveFocus()
                    }

                    Component.onCompleted: {
                        Scene3dToolBarController.onSyncLoupeZoomChanged(Math.round(value))
                    }
                }

                Text {
                    text: Math.round(syncLoupeZoomSpinBox.value) + "%"
                    color: AppPalette.text
                    font.pixelSize: Tokens.fontSm
                    horizontalAlignment: Text.AlignRight
                    Layout.preferredWidth: Tokens.controlHMd * 1.7
                }
            }

            Settings {
                property alias syncLoupeCheckButton: syncLoupeCheckButton.checked
                property alias syncLoupeSizeSpinBox: syncLoupeSizeSpinBox.value
                property alias syncLoupeZoomSpinBox: syncLoupeZoomSpinBox.value
            }
        }

        KToggleRow {
            id: isNorthViewButton
            objectName: "isNorthViewButton"
            label: qsTr("North mode")
            iconSource: "qrc:/icons/ui/location_pin.svg"

            onToggled: function(v) {
                Scene3dToolBarController.onIsNorthLocationButtonChanged(v)
            }

            Component.onCompleted: {
                Scene3dToolBarController.onIsNorthLocationButtonChanged(checked)
            }

            Settings {
                property alias isNorthViewButton: isNorthViewButton.checked
            }
        }

        KToggleRow {
            id: selectionToolButton
            objectName: "selectionToolButton"
            label: qsTr("Sync echogram")
            iconSource: "qrc:/icons/ui/click.svg"
            checked: true

            onToggled: function(v) {
                Scene3dToolBarController.onBottomTrackVertexEditingModeButtonChecked(v)
            }

            Component.onCompleted: {
                Scene3dToolBarController.onBottomTrackVertexEditingModeButtonChecked(checked)
            }

            Settings {
                property alias selectionToolButton: selectionToolButton.checked
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Tokens.spaceMd

            KToggleRow {
                id: gridCheckButton
                objectName: "gridCheckButton"
                label: qsTr("Grid")
                iconSource: "qrc:/icons/ui/grid_4x4.svg"
                Layout.fillWidth: true

                onToggled: function(v) {
                    Scene3dToolBarController.onGridVisibilityCheckedChanged(v)
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
                spacing: Tokens.spaceXs

                KToggleRow {
                    id: gridTypeCheckButton
                    objectName: "gridTypeCheckButton"
                    label: qsTr("Circle")
                    Layout.fillWidth: true

                    onToggled: function(v) {
                        Scene3dToolBarController.onPlaneGridTypeChanged(!v)
                    }
                    Component.onCompleted: {
                        Scene3dToolBarController.onPlaneGridTypeChanged(!checked)
                    }
                    Settings {
                        property alias gridTypeCheckButton: gridTypeCheckButton.checked
                    }
                }

                KToggleRow {
                    visible: gridTypeCheckButton.checked

                    id: gridLabelsCheckButton
                    objectName: "gridLabelsCheckButton"
                    label: qsTr("Labels")
                    checked: true
                    Layout.fillWidth: true

                    onToggled: function(v) {
                        Scene3dToolBarController.onPlaneGridCircleGridLabelsChanged(v)
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
                    spacing: Tokens.spaceMd

                    ColumnLayout {
                        spacing: Tokens.spaceXs
                        Text {
                            text: qsTr("Size:")
                            color: AppPalette.textSecond
                            font.pixelSize: Tokens.fontMd
                        }
                        Text {
                            text: qsTr("Step:")
                            color: AppPalette.textSecond
                            font.pixelSize: Tokens.fontMd
                        }
                        Text {
                            text: qsTr("Angle:")
                            color: AppPalette.textSecond
                            font.pixelSize: Tokens.fontMd
                        }
                    }

                    Item {
                        Layout.fillWidth: true
                    }

                    ColumnLayout {
                        spacing: Tokens.spaceXs
                        KSpinBox {
                            id: circleGridSizeSpinBox
                            from: 1
                            to: 3
                            stepSize: 1
                            value: 1

                            onValueModified: function(v) {
                                Scene3dToolBarController.onPlaneGridCircleGridSizeChanged(v)
                            }
                            Component.onCompleted: {
                                Scene3dToolBarController.onPlaneGridCircleGridSizeChanged(value)
                            }
                            Settings {
                                property alias circleGridSizeSpinBox: circleGridSizeSpinBox.value
                            }
                        }

                        KSpinBox {
                            id: circleGridStepSpinBox
                            from: 1
                            to: 20
                            stepSize: 1
                            value: 1

                            onValueModified: function(v) {
                                Scene3dToolBarController.onPlaneGridCircleGridStepChanged(v)
                            }
                            Component.onCompleted: {
                                Scene3dToolBarController.onPlaneGridCircleGridStepChanged(value)
                            }
                            Settings {
                                property alias circleGridStepSpinBox: circleGridStepSpinBox.value
                            }
                        }

                        KSpinBox {
                            id: circleGridAngleSpinBox
                            from: 1
                            to: 5
                            stepSize: 1
                            value: 1

                            onValueModified: function(v) {
                                Scene3dToolBarController.onPlaneGridCircleGridAngleChanged(v)
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

        KToggleRow {
            id: shadowEnabledCheckButton
            objectName: "shadowEnabledCheckButton"
            label: qsTr("Shadows")
            checked: true

            onToggled: function(v) {
                Scene3dToolBarController.onShadowsEnabledChanged(v)
            }

            Component.onCompleted: {
                Scene3dToolBarController.onShadowsEnabledChanged(checked)
            }
        }

        // (visible: false in original — preserved here)
        RowLayout {
            id: shadowSettingsRow
            Layout.fillWidth: true
            visible: false //shadowEnabledCheckButton.checked

            function formatShadowValue(value) {
                return Number(value).toLocaleString(Qt.locale(), 'f', 2)
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: Tokens.spaceXs

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Tokens.spaceMd

                    Text {
                        Layout.preferredWidth: Math.round(84 * AppPalette.scale)
                        text: qsTr("Vector X:")
                        color: AppPalette.textSecond
                        font.pixelSize: Tokens.fontMd
                    }

                    KSlider {
                        id: shadowVectorXSlider
                        objectName: "shadowVectorXSpinBox"
                        Layout.fillWidth: true
                        from: -1.0
                        to: 1.0
                        stepSize: 0.01
                        value: 0.40
                        enabled: shadowEnabledCheckButton.checked
                        valueDecimals: 2

                        onPressedChanged: {
                            if (pressed) {
                                settings3DSettings.focus = true
                            }
                        }

                        onValueModified: function(v) {
                            Scene3dToolBarController.onShadowVectorXChanged(v)
                        }

                        Component.onCompleted: {
                            Scene3dToolBarController.onShadowVectorXChanged(value)
                        }
                    }

                    Text {
                        Layout.preferredWidth: Math.round(48 * AppPalette.scale)
                        horizontalAlignment: Text.AlignRight
                        text: shadowSettingsRow.formatShadowValue(shadowVectorXSlider.value)
                        color: AppPalette.text
                        font.pixelSize: Tokens.fontMd
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Tokens.spaceMd

                    Text {
                        Layout.preferredWidth: Math.round(84 * AppPalette.scale)
                        text: qsTr("Vector Y:")
                        color: AppPalette.textSecond
                        font.pixelSize: Tokens.fontMd
                    }

                    KSlider {
                        id: shadowVectorYSlider
                        objectName: "shadowVectorYSpinBox"
                        Layout.fillWidth: true
                        from: -1.0
                        to: 1.0
                        stepSize: 0.01
                        value: 0.40
                        enabled: shadowEnabledCheckButton.checked
                        valueDecimals: 2

                        onPressedChanged: {
                            if (pressed) {
                                settings3DSettings.focus = true
                            }
                        }

                        onValueModified: function(v) {
                            Scene3dToolBarController.onShadowVectorYChanged(v)
                        }

                        Component.onCompleted: {
                            Scene3dToolBarController.onShadowVectorYChanged(value)
                        }
                    }

                    Text {
                        Layout.preferredWidth: Math.round(48 * AppPalette.scale)
                        horizontalAlignment: Text.AlignRight
                        text: shadowSettingsRow.formatShadowValue(shadowVectorYSlider.value)
                        color: AppPalette.text
                        font.pixelSize: Tokens.fontMd
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Tokens.spaceMd

                    Text {
                        Layout.preferredWidth: Math.round(84 * AppPalette.scale)
                        text: qsTr("Vector Z:")
                        color: AppPalette.textSecond
                        font.pixelSize: Tokens.fontMd
                    }

                    KSlider {
                        id: shadowVectorZSlider
                        objectName: "shadowVectorZSpinBox"
                        Layout.fillWidth: true
                        from: -1.0
                        to: 1.0
                        stepSize: 0.01
                        value: 0.40
                        enabled: shadowEnabledCheckButton.checked
                        valueDecimals: 2

                        onPressedChanged: {
                            if (pressed) {
                                settings3DSettings.focus = true
                            }
                        }

                        onValueModified: function(v) {
                            Scene3dToolBarController.onShadowVectorZChanged(v)
                        }

                        Component.onCompleted: {
                            Scene3dToolBarController.onShadowVectorZChanged(value)
                        }
                    }

                    Text {
                        Layout.preferredWidth: Math.round(48 * AppPalette.scale)
                        horizontalAlignment: Text.AlignRight
                        text: shadowSettingsRow.formatShadowValue(shadowVectorZSlider.value)
                        color: AppPalette.text
                        font.pixelSize: Tokens.fontMd
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Tokens.spaceMd

                    Text {
                        Layout.preferredWidth: Math.round(84 * AppPalette.scale)
                        text: qsTr("Ambient:")
                        color: AppPalette.textSecond
                        font.pixelSize: Tokens.fontMd
                    }

                    KSlider {
                        id: shadowAmbientSlider
                        objectName: "shadowAmbientSpinBox"
                        Layout.fillWidth: true
                        from: 0.0
                        to: 1.0
                        stepSize: 0.05
                        value: 0.35
                        enabled: shadowEnabledCheckButton.checked
                        valueDecimals: 2

                        onPressedChanged: {
                            if (pressed) {
                                settings3DSettings.focus = true
                            }
                        }

                        onValueModified: function(v) {
                            Scene3dToolBarController.onShadowAmbientChanged(v)
                        }

                        Component.onCompleted: {
                            Scene3dToolBarController.onShadowAmbientChanged(value)
                        }
                    }

                    Text {
                        Layout.preferredWidth: Math.round(48 * AppPalette.scale)
                        horizontalAlignment: Text.AlignRight
                        text: shadowSettingsRow.formatShadowValue(shadowAmbientSlider.value)
                        color: AppPalette.text
                        font.pixelSize: Tokens.fontMd
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Tokens.spaceMd

                    Text {
                        Layout.preferredWidth: Math.round(84 * AppPalette.scale)
                        text: qsTr("Highlight:")
                        color: AppPalette.textSecond
                        font.pixelSize: Tokens.fontMd
                    }

                    KSlider {
                        id: shadowHighlightSlider
                        objectName: "shadowHighlightSpinBox"
                        Layout.fillWidth: true
                        from: 0.0
                        to: 1.0
                        stepSize: 0.05
                        value: 0.70
                        enabled: shadowEnabledCheckButton.checked
                        valueDecimals: 2

                        onPressedChanged: {
                            if (pressed) {
                                settings3DSettings.focus = true
                            }
                        }

                        onValueModified: function(v) {
                            Scene3dToolBarController.onShadowHighlightChanged(v)
                        }

                        Component.onCompleted: {
                            Scene3dToolBarController.onShadowHighlightChanged(value)
                        }
                    }

                    Text {
                        Layout.preferredWidth: Math.round(48 * AppPalette.scale)
                        horizontalAlignment: Text.AlignRight
                        text: shadowSettingsRow.formatShadowValue(shadowHighlightSlider.value)
                        color: AppPalette.text
                        font.pixelSize: Tokens.fontMd
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

        RowLayout {
            Layout.fillWidth: true
            spacing: Tokens.spaceMd

            KToggleRow {
                id: navigationArrowCheckButton
                objectName: "navigationArrowCheckButton"
                label: qsTr("Boat")
                iconSource: "qrc:/icons/ui/speedboat.svg"
                checked: true
                Layout.fillWidth: true

                onToggled: function(v) {
                    NavigationArrowControlMenuController.onVisibilityCheckBoxCheckedChanged(v)
                }

                Component.onCompleted: {
                    NavigationArrowControlMenuController.onVisibilityCheckBoxCheckedChanged(checked)
                }
            }

            RowLayout {
                visible: navigationArrowCheckButton.checked
                spacing: Tokens.spaceXs

                Text {
                    text: qsTr("Size:")
                    color: AppPalette.textSecond
                    font.pixelSize: Tokens.fontMd
                }

                KSpinBox {
                    id: navigationArrowSizeSpinBox
                    from: 1
                    to: 5
                    stepSize: 1
                    value: 1

                    onValueModified: function(v) {
                        NavigationArrowControlMenuController.onSizeSpinBoxValueChanged(v)
                    }

                    Component.onCompleted: {
                        NavigationArrowControlMenuController.onSizeSpinBoxValueChanged(value)
                    }
                }
            }

            Settings {
                property alias navigationArrowCheckButton: navigationArrowCheckButton.checked
                property alias navigationArrowSizeSpinBox: navigationArrowSizeSpinBox.value
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Tokens.spaceMd

            KToggleRow {
                id: compassCheckButton
                objectName: "compassCheckButton"
                label: qsTr("Compass")
                iconSource: "qrc:/icons/ui/compass.svg"
                checked: true
                Layout.fillWidth: true

                onToggled: function(v) {
                    Scene3dToolBarController.onCompassButtonChanged(v)
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
                spacing: Tokens.spaceMd

                ColumnLayout {
                    spacing: Tokens.spaceXs
                    Text {
                        text: qsTr("Pos:")
                        color: AppPalette.textSecond
                        font.pixelSize: Tokens.fontMd
                    }
                    Text {
                        text: qsTr("Size:")
                        color: AppPalette.textSecond
                        font.pixelSize: Tokens.fontMd
                    }
                }

                Item {
                    Layout.fillWidth: true
                }

                ColumnLayout {
                    spacing: Tokens.spaceXs
                    KSpinBox {
                        id: compassPosSpinBox
                        from: 1
                        to: 3
                        stepSize: 1
                        value: 2

                        onValueModified: function(v) {
                            Scene3dToolBarController.onCompassPosChanged(v)
                        }
                        Component.onCompleted: {
                            Scene3dToolBarController.onCompassPosChanged(value)
                        }
                        Settings {
                            property alias compassPosSpinBox: compassPosSpinBox.value
                        }
                    }

                    KSpinBox {
                        id: compassSizeSpinBox
                        from: 1
                        to: 5
                        stepSize: 1
                        value: 1

                        onValueModified: function(v) {
                            Scene3dToolBarController.onCompassSizeChanged(v)
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
