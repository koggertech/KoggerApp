import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import Qt.labs.settings 1.1
import QtQuick.Dialogs 1.2


ColumnLayout {

    function updateSurface() {
        updateSurfaceButton.clicked();
    }

    function updateMosaic() {
        updateMosaicButton.clicked();
    }

    Layout.alignment: Qt.AlignHCenter

    // surface view extra settings
    MenuFrame {
        id: surfaceViewSettings
        visible: surfaceViewCheckButton.hovered || isHovered || surfaceViewCheckButton.surfaceViewLongPressTriggered || surfaceViewTheme.activeFocus
        z: surfaceViewSettings.visible
        Layout.alignment: Qt.AlignRight

        onIsHoveredChanged: {
            if (Qt.platform.os === "android") {
                if (isHovered) {
                    isHovered = false
                }
            }
            else {
                if (!isHovered || !surfaceViewCheckButton.hovered) {
                    surfaceViewCheckButton.surfaceViewLongPressTriggered = false
                }
            }
        }

        onVisibleChanged: {
            if (visible) {
                focus = true;
            }
        }

        onFocusChanged: {
            if (!focus) {
                surfaceViewCheckButton.surfaceViewLongPressTriggered = false
            }
        }

        ColumnLayout {
            CheckButton {
                id: realtimeSurfaceProcessingCheckButton
                text: qsTr("Realtime processing")
                Layout.fillWidth: true

                onCheckedChanged: {
                    SurfaceViewControlMenuController.onProcessStateChanged(checked);
                }

                onFocusChanged: {
                    surfaceViewSettings.focus = true
                }

                Component.onCompleted: {
                    SurfaceViewControlMenuController.onProcessStateChanged(checked);

                }

                Settings {
                    property alias realtimeSurfaceProcessingCheckButton: realtimeSurfaceProcessingCheckButton.checked
                }
            }

            CButton {
                id: resetSurfaceViewButton
                text: qsTr("Reset")
                Layout.fillWidth: true
                onClicked: {
                    SurfaceViewControlMenuController.onResetSurfaceViewButtonClicked()
                }

                onFocusChanged: {
                    surfaceViewSettings.focus = true
                }
            }

            RowLayout {
                CText {
                    text: qsTr("Edge limit, m:")
                    Layout.fillWidth: true
                }
                SpinBoxCustom {
                    id: surfaceViewEdgeLimitSpinBox
                    implicitWidth: 200
                    from: 10
                    to: 1000
                    stepSize: 5
                    value: 20
                    editable: false

                    property int decimals: 1

                    onFocusChanged: {
                        surfaceViewSettings.focus = true
                    }

                    Component.onCompleted: {
                        SurfaceViewControlMenuController.onEdgeLimitChanged(surfaceViewEdgeLimitSpinBox.value)
                    }

                    onValueChanged: {
                        SurfaceViewControlMenuController.onEdgeLimitChanged(surfaceViewEdgeLimitSpinBox.value)
                    }

                    Settings {
                        property alias surfaceViewEdgeLimitSpinBox: surfaceViewEdgeLimitSpinBox.value
                    }
                }
            }

            RowLayout {
                CText {
                    text: qsTr("Handle each call:")
                    Layout.fillWidth: true
                }
                SpinBoxCustom {
                    id: surfaceViewHandleXCallSpinBox
                    implicitWidth: 200
                    from: 1
                    to: 100
                    stepSize: 1
                    value: 1
                    editable: false

                    property int decimals: 1

                    onFocusChanged: {
                        surfaceViewSettings.focus = true
                    }

                    Component.onCompleted: {
                        SurfaceViewControlMenuController.onHandleXCallChanged(surfaceViewHandleXCallSpinBox.value)
                    }

                    onValueChanged: {
                        SurfaceViewControlMenuController.onHandleXCallChanged(surfaceViewHandleXCallSpinBox.value)
                    }

                    Settings {
                        property alias surfaceViewHandleXCallSpinBox: surfaceViewHandleXCallSpinBox.value
                    }
                }
            }

            RowLayout {
                visible: !surfaceViewDebugModeCheckButton.checked
                CText {
                    text: qsTr("Theme:")
                }
                Item {
                    Layout.fillWidth: true
                }
                CCombo  {
                    id: surfaceViewTheme
                    Layout.preferredWidth: 300
                    model: [qsTr("Midnight"), qsTr("Default"), qsTr("Blue"), qsTr("Sepia"), qsTr("WRGBD"), qsTr("WhiteBlack"), qsTr("BlackWhite")]
                    currentIndex: 0
                    onCurrentIndexChanged: {
                        SurfaceViewControlMenuController.onThemeChanged(currentIndex)
                    }

                    onFocusChanged: {
                        if (Qt.platform.os === 'android') {
                            surfaceViewSettings.focus = true
                        }
                    }

                    Component.onCompleted: {
                        SurfaceViewControlMenuController.onThemeChanged(currentIndex)
                    }

                    Settings {
                        property alias surfaceViewTheme: surfaceViewTheme.currentIndex
                    }
                }
            }

            RowLayout {
                visible: !surfaceViewDebugModeCheckButton.checked

                CText {
                    text: qsTr("Surface/line step, m:")
                    Layout.fillWidth: true

                }
                SpinBoxCustom {
                    id: surfaceViewSurfaceLineStepSizeSpinBox
                    implicitWidth: 200
                    from: 1
                    to: 200
                    stepSize: 1
                    value: 3
                    editable: false

                    property int decimals: 1
                    property real realValue: value / 10

                    validator: DoubleValidator {
                        bottom: Math.min(surfaceViewSurfaceLineStepSizeSpinBox.from, surfaceViewSurfaceLineStepSizeSpinBox.to)
                        top:  Math.max(surfaceViewSurfaceLineStepSizeSpinBox.from, surfaceViewSurfaceLineStepSizeSpinBox.to)
                    }

                    textFromValue: function(value, locale) {
                        return Number(value / 10).toLocaleString(locale, 'f', decimals)
                    }

                    valueFromText: function(text, locale) {
                        return Number.fromLocaleString(locale, text) * 10
                    }

                    onFocusChanged: {
                        surfaceViewSettings.focus = true
                    }

                    Component.onCompleted: {
                        SurfaceViewControlMenuController.onSetSurfaceLineStepSize(surfaceViewSurfaceLineStepSizeSpinBox.realValue)
                    }

                    onRealValueChanged: {
                        SurfaceViewControlMenuController.onSetSurfaceLineStepSize(surfaceViewSurfaceLineStepSizeSpinBox.realValue)
                    }

                    Settings {
                        property alias surfaceViewSurfaceLineStepSizeSpinBox: surfaceViewSurfaceLineStepSizeSpinBox.value
                    }
                }
            }
            RowLayout {
                visible: !surfaceViewDebugModeCheckButton.checked

                CText {
                    text: qsTr("Label step, m:")
                    Layout.fillWidth: true
                }
                SpinBoxCustom {
                    id: surfaceViewLabelStepSpinBox
                    implicitWidth: 200
                    from: 10
                    to: 1000
                    stepSize: 5
                    value: 100
                    editable: false

                    property int decimals: 1

                    onFocusChanged: {
                        surfaceViewSettings.focus = true
                    }

                    Component.onCompleted: {
                        SurfaceViewControlMenuController.onSetLabelStepSize(surfaceViewLabelStepSpinBox.value)
                    }

                    onValueChanged: {
                        SurfaceViewControlMenuController.onSetLabelStepSize(surfaceViewLabelStepSpinBox.value)
                    }

                    Settings {
                        property alias surfaceViewLabelStepSpinBox: surfaceViewLabelStepSpinBox.value
                    }
                }
            }

            CheckButton {
                text: qsTr("Triangles")
                Layout.fillWidth: true
                checked: true
                visible: surfaceViewDebugModeCheckButton.checked

                onCheckedChanged: {
                    SurfaceViewControlMenuController.onTrianglesVisible(checked);
                }

                onFocusChanged: {
                    surfaceViewSettings.focus = true
                }
            }

            CheckButton {
                text: qsTr("Edges")
                Layout.fillWidth: true
                checked: true
                visible: surfaceViewDebugModeCheckButton.checked

                onCheckedChanged: {
                    SurfaceViewControlMenuController.onEdgesVisible(checked);
                }

                onFocusChanged: {
                    surfaceViewSettings.focus = true
                }
            }

            CheckButton {
                id: surfaceViewDebugModeCheckButton
                text: qsTr("Debug mode")
                Layout.fillWidth: true
                checked: false

                onCheckedChanged: {
                    SurfaceViewControlMenuController.onDebugModeView(checked);
                }

                onFocusChanged: {
                    surfaceViewSettings.focus = true
                }
            }

            // CButton {
            //     id: updateSurfaceViewButton
            //     text: qsTr("some action")
            //     Layout.fillWidth: true

            //     onClicked: {
            //         SurfaceViewControlMenuController.onUpdateSurfaceViewButtonClicked()
            //     }

            //     onFocusChanged: {
            //         surfaceViewSettings.focus = true
            //     }
            // }
        }
    }

    // side-scan extra settings
    MenuFrame {
        id: sideScanViewSettings
        visible: sideScanViewCheckButton.hovered || isHovered || sideScanViewCheckButton.sideScanLongPressTriggered || sideScanTheme.activeFocus
        z: sideScanViewSettings.visible
        Layout.alignment: Qt.AlignRight

        onIsHoveredChanged: {
            if (Qt.platform.os === "android") {
                if (isHovered) {
                    isHovered = false
                }
            }
            else {
                if (!isHovered || !sideScanViewCheckButton.hovered) {
                    sideScanViewCheckButton.sideScanLongPressTriggered = false
                }
            }
        }

        onVisibleChanged: {
            if (visible) {
                focus = true;
            }
        }

        onFocusChanged: {
            if (!focus) {
                sideScanViewCheckButton.sideScanLongPressTriggered = false
            }
        }

        RowLayout {
            ColumnLayout {
                CheckButton {
                    id: realtimeProcessingButton
                    text: qsTr("Realtime processing")
                    Layout.fillWidth: true
                    Layout.preferredWidth: 200
                    checked: false

                    onToggled: {
                        SideScanViewControlMenuController.onUpdateStateChanged(checked)
                    }

                    onFocusChanged: {
                        sideScanViewSettings.focus = true
                    }

                    Component.onCompleted: {
                        //realtimeProcessingButton.checked = core.isSeparateReading
                        SideScanViewControlMenuController.onUpdateStateChanged(checked)

                    }

                    Settings {
                        property alias realtimeProcessingButton: realtimeProcessingButton.checked
                    }
                }
                RowLayout {
                    CText {
                        text: qsTr("Theme:")
                    }
                    Item {
                        Layout.fillWidth: true
                    }
                    CCombo  {
                        id: sideScanTheme
                        Layout.preferredWidth: 300
                        model: [qsTr("Blue"), qsTr("Sepia"), qsTr("WRGBD"), qsTr("WhiteBlack"), qsTr("BlackWhite")]
                        currentIndex: 0
                        onCurrentIndexChanged: {
                            SideScanViewControlMenuController.onThemeChanged(currentIndex)
                        }

                        onFocusChanged: {
                            if (Qt.platform.os === 'android') {
                                sideScanViewSettings.focus = true
                            }
                        }

                        Component.onCompleted: {
                            SideScanViewControlMenuController.onThemeChanged(currentIndex)
                        }

                        Settings {
                            property alias sideScanTheme: sideScanTheme.currentIndex
                        }
                    }
                }
                RowLayout {
                    CText {
                        text: qsTr("Angle offset, °")
                    }
                    Item {
                        Layout.fillWidth: true
                    }
                    ColumnLayout {
                        RowLayout {
                            Item {
                                Layout.fillWidth: true
                            }
                            CText {
                                text: qsTr("left:")
                            }
                            SpinBoxCustom  {
                                id: sideScanLAngleOffset
                                implicitWidth: 200
                                from: -90
                                to: 90
                                stepSize: 1
                                value: 0
                                editable: false

                                onValueChanged: {
                                    SideScanViewControlMenuController.onSetLAngleOffset(value)
                                }

                                onFocusChanged: {
                                    sideScanViewSettings.focus = true
                                }

                                Component.onCompleted: {
                                    SideScanViewControlMenuController.onSetLAngleOffset(value)
                                }

                                Settings {
                                    property alias sideScanLAngleOffset: sideScanLAngleOffset.value
                                }
                            }
                        }
                        RowLayout {
                            Item {
                                Layout.fillWidth: true
                            }
                            CText {
                                text: qsTr("right:")
                            }
                            SpinBoxCustom  {
                                id: sideScanRAngleOffset
                                implicitWidth: 200
                                from: -90
                                to: 90
                                stepSize: 1
                                value: 0
                                editable: false

                                onValueChanged: {
                                    SideScanViewControlMenuController.onSetRAngleOffset(value)
                                }

                                onFocusChanged: {
                                    sideScanViewSettings.focus = true
                                }

                                Component.onCompleted: {
                                    SideScanViewControlMenuController.onSetRAngleOffset(value)
                                }

                                Settings {
                                    property alias sideScanRAngleOffset: sideScanRAngleOffset.value
                                }
                            }
                        }
                    }
                }

                ColumnLayout {
                    RowLayout {
                        //visible: core.isSeparateReading

                        CText {
                            text: qsTr("Tile side pixel size:")
                        }
                        Item {
                            Layout.fillWidth: true
                        }
                        SpinBoxCustom {
                            id: sideScanTileSidePixelSizeSpinBox
                            implicitWidth: 200
                            from: 32
                            to: 2048
                            stepSize: 1
                            value: 256
                            editable: false

                            onFocusChanged: {
                                sideScanViewSettings.focus = true
                            }
                        }
                    }

                    RowLayout {
                        //visible: core.isSeparateReading

                        CText {
                            text: qsTr("Tile height matrix ratio:")
                        }
                        Item {
                            Layout.fillWidth: true
                        }
                        SpinBoxCustom {
                            id: sideScanTileHeightMatrixRatioSpinBox
                            implicitWidth: 200
                            from: 2
                            to: 256
                            stepSize: 1
                            value: 16
                            editable: false

                            onFocusChanged: {
                                sideScanViewSettings.focus = true
                            }
                        }
                    }

                    RowLayout {
                        CText {
                            text: qsTr("Tile resolution, pix/m:")
                        }
                        Item {
                            Layout.fillWidth: true
                        }
                        SpinBoxCustom {
                            id: sideScanTileResolutionSpinBox
                            implicitWidth: 200
                            from: 1
                            to: 100
                            stepSize: 5
                            value: 10
                            editable: false

                            onFocusChanged: {
                                sideScanViewSettings.focus = true
                            }
                        }
                    }

                    CButton {
                        text: qsTr("Reinit global mesh")
                        Layout.fillWidth: true
                        Layout.preferredWidth: 200
                        enabled: !core.isMosaicUpdatingInThread && !core.isFileOpening

                        onClicked: {
                            SideScanViewControlMenuController.onGlobalMeshChanged(
                                        sideScanTileSidePixelSizeSpinBox.value, sideScanTileHeightMatrixRatioSpinBox.value, 1 / sideScanTileResolutionSpinBox.value)
                        }

                        onFocusChanged: {
                            sideScanViewSettings.focus = true
                        }
                    }
                }
                CheckButton {
                    id: sideScanUseLinearFilter
                    text: qsTr("Use linear filter")
                    Layout.fillWidth: true
                    Layout.preferredWidth: 200
                    //visible: core.isSeparateReading

                    onClicked: {
                        SideScanViewControlMenuController.onUseFilterChanged(checked)
                    }

                    onFocusChanged: {
                        sideScanViewSettings.focus = true
                    }

                    Component.onCompleted: {
                        SideScanViewControlMenuController.onUseFilterChanged(checked)
                    }

                    Settings {
                        property alias sideScanUseLinearFilter: sideScanUseLinearFilter.checked
                    }
                }
                CheckButton {
                    id: sideScanGridContourVisible
                    text: qsTr("Grid/contour visible")
                    Layout.fillWidth: true
                    Layout.preferredWidth: 200
                    checked: false
                    //visible: core.isSeparateReading

                    onClicked: {
                        SideScanViewControlMenuController.onGridVisibleChanged(checked)
                    }

                    onFocusChanged: {
                        sideScanViewSettings.focus = true
                    }

                    Component.onCompleted: {
                        SideScanViewControlMenuController.onGridVisibleChanged(checked)
                    }

                    Settings {
                        property alias sideScanGridContourVisible: sideScanGridContourVisible.checked
                    }
                }
                CheckButton {
                    id: sideScanMeasLinesVisible
                    text: qsTr("Measuse lines visible")
                    Layout.fillWidth: true
                    Layout.preferredWidth: 200
                    checked: false
                    //visible: core.isSeparateReading

                    onClicked: {
                        SideScanViewControlMenuController.onMeasLineVisibleChanged(checked)
                    }

                    onFocusChanged: {
                        sideScanViewSettings.focus = true
                    }

                    Component.onCompleted: {
                        SideScanViewControlMenuController.onMeasLineVisibleChanged(checked)
                    }

                    Settings {
                        property alias sideScanMeasLinesVisible: sideScanMeasLinesVisible.checked
                    }
                }
                CheckButton {
                    id: sideScanGenerateGridContour
                    text: qsTr("Generate grid/contour")
                    Layout.fillWidth: true
                    Layout.preferredWidth: 200
                    checked: false
                    //visible: core.isSeparateReading

                    onClicked: {
                        SideScanViewControlMenuController.onGenerateGridContourChanged(checked)
                    }

                    onFocusChanged: {
                        sideScanViewSettings.focus = true
                    }

                    Component.onCompleted: {
                        SideScanViewControlMenuController.onGenerateGridContourChanged(checked)
                    }

                    Settings {
                        property alias sideScanGenerateGridContour: sideScanGenerateGridContour.checked
                    }
                }

                CButton {
                    text: qsTr("Clear")
                    Layout.fillWidth: true
                    Layout.preferredWidth: 200
                    //visible: core.isSeparateReading
                    enabled: !core.isMosaicUpdatingInThread && !core.isFileOpening

                    onClicked: {
                        SideScanViewControlMenuController.onClearClicked()
                    }

                    onFocusChanged: {
                        sideScanViewSettings.focus = true
                    }
                }

                Item {
                    Layout.fillHeight: true
                }

                CButton {
                    id: updateMosaicButton
                    text: qsTr("Update")
                    Layout.fillWidth: true
                    Layout.preferredWidth: 200
                    enabled: !core.isMosaicUpdatingInThread && !core.isFileOpening

                    onClicked: {
                        SideScanViewControlMenuController.onUpdateClicked()
                    }

                    onFocusChanged: {
                        sideScanViewSettings.focus = true
                    }
                }
            }

            // levels
            ColumnLayout {
                CText {
                    Layout.fillWidth: true
                    Layout.topMargin: 0
                    Layout.preferredWidth: theme.controlHeight*1.2
                    horizontalAlignment: Text.AlignHCenter
                    text: sideScanLevelsSlider.stopValue
                    small: true
                }

                ChartLevel {
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    Layout.preferredWidth: theme.controlHeight * 1.2
                    id: sideScanLevelsSlider
                    Layout.alignment: Qt.AlignHCenter

                    onStartValueChanged: {
                       SideScanViewControlMenuController.onLevelChanged(startValue, stopValue);
                    }

                    onStopValueChanged: {
                       SideScanViewControlMenuController.onLevelChanged(startValue, stopValue);
                    }

                    Component.onCompleted: {
                        SideScanViewControlMenuController.onLevelChanged(startValue, stopValue);
                    }

                    Settings {
                        property alias sideScanLevelsStart: sideScanLevelsSlider.startValue
                        property alias sideScanLevelsStop: sideScanLevelsSlider.stopValue
                    }
                }

                CText {
                    Layout.fillWidth: true
                    Layout.preferredWidth: theme.controlHeight * 1.2
                    Layout.bottomMargin: 0
                    horizontalAlignment: Text.AlignHCenter

                    text: sideScanLevelsSlider.startValue
                    small: true
                }
            }
        }
    }

    RowLayout {
        spacing: 2
        Layout.alignment: Qt.AlignHCenter

        CheckButton {
            id: setCameraIsometricView
            backColor: theme.controlBackColor
            iconSource: "./fit-in-view.svg"
            checkable: false
            checked: false
            implicitWidth: theme.controlHeight

            onClicked: Scene3dToolBarController.onSetCameraMapViewButtonClicked()
        }

        CheckButton {
            id: fitAllinViewButton
            iconSource: "qrc:/icons/ui/zoom_cancel.svg"
            backColor: theme.controlBackColor
            checkable: false
            checked: false
            implicitWidth: theme.controlHeight

            onClicked: Scene3dToolBarController.onFitAllInViewButtonClicked()
        }

        CheckButton {
            id: cancelZoomViewButton
            iconSource: "qrc:/icons/ui/ruler_measure.svg"
            backColor: theme.controlBackColor
            checkable: false
            checked: false
            implicitWidth: theme.controlHeight

            onClicked: Scene3dToolBarController.onCancelZoomButtonClicked()
        }

        CheckButton {
            id: selectionToolButton
            objectName: "selectionToolButton"
            backColor: theme.controlBackColor
            borderColor: theme.controlBackColor
            checkedBorderColor: theme.controlBorderColor
            iconSource: "qrc:/icons/ui/click.svg"
            implicitWidth: theme.controlHeight

            onCheckedChanged: {
                Scene3dToolBarController.onBottomTrackVertexEditingModeButtonChecked(checked)
            }

            Settings {
                property alias selectionToolButton: selectionToolButton.checked
            }
        }

        CheckButton {
            id: trackLastDataCheckButton
            objectName: "trackLastDataCheckButton"
            backColor: theme.controlBackColor
            borderColor: theme.controlBackColor
            checkedBorderColor: theme.controlBorderColor
            checked: false
            iconSource: "qrc:/icons/ui/location.svg"
            implicitWidth: theme.controlHeight

            onToggled: {
                Scene3dToolBarController.onTrackLastDataCheckButtonCheckedChanged(checked)
            }

            Component.onCompleted: {
                Scene3dToolBarController.onTrackLastDataCheckButtonCheckedChanged(checked)
            }

            Settings {
                property alias trackLastDataCheckButton: trackLastDataCheckButton.checked
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
            implicitWidth: theme.controlHeight

            onCheckedChanged: {
                NavigationArrowControlMenuController.onVisibilityCheckBoxCheckedChanged(checked)
            }

            Component.onCompleted: {
                NavigationArrowControlMenuController.onVisibilityCheckBoxCheckedChanged(checked)
            }

            Settings {
                property alias navigationArrowCheckButton: navigationArrowCheckButton.checked
            }
        }

        CheckButton {
            id: boatTrackCheckButton
            backColor: theme.controlBackColor
            borderColor: theme.controlBackColor
            checkedBorderColor: theme.controlBorderColor
            checked: true
            iconSource: "qrc:/icons/ui/route.svg"
            implicitWidth: theme.controlHeight

            onCheckedChanged: {
                BoatTrackControlMenuController.onVisibilityCheckBoxCheckedChanged(checked)
            }

            Component.onCompleted: {
                BoatTrackControlMenuController.onVisibilityCheckBoxCheckedChanged(checked)
            }

            Settings {
                property alias boatTrackCheckButton: boatTrackCheckButton.checked
            }
        }

        CheckButton {
            id: bottomTrackCheckButton
            backColor: theme.controlBackColor
            borderColor: theme.controlBackColor
            checkedBorderColor: theme.controlBorderColor
            checked: true
            iconSource: "qrc:/icons/ui/overline.svg"
            implicitWidth: theme.controlHeight

            onCheckedChanged: {
                BottomTrackControlMenuController.onVisibilityCheckBoxCheckedChanged(checked)
            }

            Component.onCompleted: {
                BottomTrackControlMenuController.onVisibilityCheckBoxCheckedChanged(checked)
            }

            Settings {
                property alias bottomTrackCheckButton: bottomTrackCheckButton.checked
            }
        }

        // surface view check button
        CheckButton {
            id: surfaceViewCheckButton
            backColor: theme.controlBackColor
            borderColor: theme.controlBackColor
            checkedBorderColor: theme.controlBorderColor
            checked: true
            iconSource: "qrc:/icons/ui/grid_4x4.svg"
            implicitWidth: theme.controlHeight

            onCheckedChanged: {
                SurfaceViewControlMenuController.onSurfaceViewVisibilityCheckBoxCheckedChanged(checked)
            }

            Component.onCompleted: {
                SurfaceViewControlMenuController.onSurfaceViewVisibilityCheckBoxCheckedChanged(checked)
            }

            property bool surfaceViewLongPressTriggered: false

            MouseArea {
                id: surfaceViewTouchArea
                anchors.fill: parent
                onPressed: {
                    surfaceViewLongPressTimer.start()
                    surfaceViewCheckButton.surfaceViewLongPressTriggered = false
                }

                onReleased: {
                    if (!surfaceViewCheckButton.surfaceViewLongPressTriggered) {
                        surfaceViewCheckButton.checked = !surfaceViewCheckButton.checked
                    }
                    surfaceViewLongPressTimer.stop()
                }

                onCanceled: {
                    surfaceViewLongPressTimer.stop()
                }
            }

            Timer {
                id: surfaceViewLongPressTimer
                interval: 100 // ms
                repeat: false

                onTriggered: {
                    surfaceViewCheckButton.surfaceViewLongPressTriggered = true;
                }
            }

            Settings {
                property alias surfaceViewCheckButton: surfaceViewCheckButton.checked
            }
        }

        // side scan view button
        CheckButton {
            id: sideScanViewCheckButton
            backColor: theme.controlBackColor
            borderColor: theme.controlBackColor
            checkedBorderColor: theme.controlBorderColor
            checked: true
            iconSource: "qrc:/icons/ui/map_route.svg"
            implicitWidth: theme.controlHeight

            onCheckedChanged: {
                SideScanViewControlMenuController.onVisibilityChanged(checked)
            }

            property bool sideScanLongPressTriggered: false

            MouseArea {
                id: sideScanViewTouchArea
                anchors.fill: parent
                onPressed: {
                    sideScanViewLongPressTimer.start()
                    sideScanViewCheckButton.sideScanLongPressTriggered = false
                }

                onReleased: {
                    if (!sideScanViewCheckButton.sideScanLongPressTriggered) {
                        sideScanViewCheckButton.checked = !sideScanViewCheckButton.checked
                    }
                    sideScanViewLongPressTimer.stop()
                }

                onCanceled: {
                    sideScanViewLongPressTimer.stop()
                }
            }

            Timer {
                id: sideScanViewLongPressTimer
                interval: 100 // ms
                repeat: false

                onTriggered: {
                    sideScanViewCheckButton.sideScanLongPressTriggered = true;
                }
            }

            Component.onCompleted: {
                SideScanViewControlMenuController.onVisibilityChanged(checked)
            }

            Settings {
                property alias sideScanViewCheckButton: sideScanViewCheckButton.checked
            }
        }

        // mapView button
        CheckButton {
            id: mapViewCheckButton
            backColor: theme.controlBackColor
            borderColor: theme.controlBackColor
            checkedBorderColor: theme.controlBorderColor
            checked: true
            iconSource: "qrc:/icons/ui/map.svg"
            implicitWidth: theme.controlHeight

            onCheckedChanged: {
                MapViewControlMenuController.onVisibilityChanged(checked)
            }

            Component.onCompleted: {
                MapViewControlMenuController.onVisibilityChanged(checked)
            }


            Settings {
                property alias mapViewCheckButton: mapViewCheckButton.checked
            }
        }

        ButtonGroup{
            property bool buttonChangeFlag : false
            id: buttonGroup
            onCheckedButtonChanged: buttonChangeFlag = true
            onClicked: {
                if (!buttonChangeFlag) {
                    checkedButton = null
                }

                buttonChangeFlag = false;
            }
        }
    }
}
