import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import Qt.labs.settings 1.1
import QtQuick.Dialogs 1.2


ColumnLayout {
    Layout.alignment: Qt.AlignHCenter

    // surface extra settings
    MenuFrame {
        id: surfaceSettings
        visible: surfaceCheckButton.hovered || isHovered || surfaceCheckButton.longPressTriggered
        z: surfaceSettings.visible
        Layout.alignment: Qt.AlignHCenter

        onIsHoveredChanged: {
            if (Qt.platform.os === "android") {
                if (isHovered) {
                    isHovered = false
                }
            }
            else {
                if (!isHovered || !surfaceCheckButton.hovered) {
                    surfaceCheckButton.longPressTriggered = false
                }
            }
            //console.debug("surface menu hovered " + isHovered.toString())
        }

        onVisibleChanged: {
            if (visible) {
                focus = true;
            }
        }

        onFocusChanged: {
            if (!focus) {
                surfaceCheckButton.longPressTriggered = false
            }
        }

        ColumnLayout {
            //width: 300
            ParamSetup {
                paramName: qsTr("Edge limit, m:")

                SpinBoxCustom {
                    id: triangleEdgeLengthLimitSpinBox
                    //implicitWidth: 110
                    from: 5
                    to: 200
                    stepSize: 5
                    value: 50
                    editable: false

                    onFocusChanged: {
                        surfaceSettings.focus = true
                    }
                }
            }

            ParamSetup {
                paramName: qsTr("Decimation by:")

                CheckButton {
                    id: decimationCountCheck
                    text: qsTr("Count")
                    checked: true
                    ButtonGroup.group: decimationGroup

                    onFocusChanged: {
                        surfaceSettings.focus = true
                    }
                }

                CheckButton {
                    id: decimationDistanceCheck
                    text: qsTr("Distance")
                    ButtonGroup.group: decimationGroup

                    onFocusChanged: {
                        surfaceSettings.focus = true
                    }
                }

                // CheckButton {
                //     icon.source: "./icons/x.svg"
                //     ButtonGroup.group: decimationGroup
                // }

                ButtonGroup{
                    id: decimationGroup
                }
            }

            ParamSetup {
                visible: decimationCountCheck.checked
                paramName: qsTr("Point count:")

                SpinBoxCustom {
                    id: decimationCountSpinBox
                    from: 100
                    to: 10000
                    stepSize: 100
                    value: 1000
                    editable: false

                    onFocusChanged: {
                        surfaceSettings.focus = true
                    }
                }
            }

            ParamSetup {
                //id: decimationDistance
                visible: decimationDistanceCheck.checked
                paramName: qsTr("Decimation, m:")

                SpinBoxCustom {
                    id: decimationDistanceSpinBox
                    implicitWidth: 150
                    from: 1
                    to: 100
                    stepSize: 1
                    value: 5
                    editable: false

                    onFocusChanged: {
                        surfaceSettings.focus = true
                    }
                }
            }

            ParamSetup {
                paramName: qsTr("Type:")

                CheckButton {
                    id: triangleTypeCheck
                    text: qsTr("Triangle")
                    checked: true
                    ButtonGroup.group: surfaceTypeGroup

                    onFocusChanged: {
                        surfaceSettings.focus = true
                    }
                }

                CheckButton {
                    id: gridTypeCheck
                    text: qsTr("Grid")
                    ButtonGroup.group: surfaceTypeGroup

                    onFocusChanged: {
                        surfaceSettings.focus = true
                    }
                }

                ButtonGroup{
                    id: surfaceTypeGroup
                }
            }

            ParamSetup {
                visible: gridTypeCheck.checked
                paramName: qsTr("Grid step, m:")

                SpinBoxCustom {
                    id: gridCellSizeSpinBox
                    implicitWidth: 150
                    from: 1
                    to: 20
                    stepSize: 1
                    value: 5
                    editable: false

                    onFocusChanged: {
                        surfaceSettings.focus = true
                    }
                }
            }

            RowLayout {
                Layout.alignment: Qt.AlignHCenter

                CheckButton {
                    id: contourVisibilityCheckButton
                    text: qsTr("Show contour")
                    //checked: true
                    Layout.fillWidth: true

                    onToggled: {
                        SurfaceControlMenuController.onSurfaceContourVisibilityCheckBoxCheckedChanged(checked)
                    }

                    onFocusChanged: {
                        surfaceSettings.focus = true
                    }
                }
                CheckButton {
                    id: gridVisibilityCheckButton
                    text: qsTr("Show grid")
                    //checked: true
                    Layout.fillWidth: true

                    onToggled: {
                        SurfaceControlMenuController.onSurfaceGridVisibilityCheckBoxCheckedChanged(checked)
                    }

                    onFocusChanged: {
                        surfaceSettings.focus = true
                    }
                }
            }

            CButton {
                text: qsTr("Update")
                Layout.fillWidth: true
                icon.source: "./icons/refresh.svg"
                onClicked: {
                    SurfaceControlMenuController.onUpdateSurfaceButtonClicked(
                        triangleEdgeLengthLimitSpinBox.value,
                        !gridTypeCheck.checked ? -1: gridCellSizeSpinBox.value,
                        !decimationCountCheck.checked ? -1 : decimationCountSpinBox.value,
                        !decimationDistanceCheck.checked ? -1 : decimationDistanceSpinBox.value)
                    //BottomTrackControlMenuController.onSurfaceUpdated()
                }

                onFocusChanged: {
                    surfaceSettings.focus = true
                }
            }
        }
    }

    // usbl settings
    MenuFrame {
        id: usblViewSettings
        visible: usblViewCheckButton.hovered || isHovered || usblViewCheckButton.longPressTriggered
        z: usblViewSettings.visible
        Layout.alignment: Qt.AlignRight

        onIsHoveredChanged: {
            if (Qt.platform.os === "android") {
                if (isHovered) {
                    isHovered = false
                }
            }
            else {
                if (!isHovered || !usblViewCheckButton.hovered) {
                    usblViewCheckButton.longPressTriggered = false
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
                usblViewCheckButton.longPressTriggered = false
            }
        }

        ColumnLayout {
            CButton {
                text: "Update"
                Layout.fillWidth: true
                Layout.preferredWidth: 200

                onClicked: {
                    UsblViewControlMenuController.onUpdateUsblViewButtonClicked()
                }
                onFocusChanged: {
                    surfaceSettings.focus = true
                }
            }

            CButton {
                text: "Clear"
                Layout.fillWidth: true
                Layout.preferredWidth: 200

                onClicked: {
                    UsblViewControlMenuController.onClearUsblViewButtonClicked()
                }
                onFocusChanged: {
                    surfaceSettings.focus = true
                }
            }
        }
    }

    // side-scan extra settings
    MenuFrame {
        id: sideScanViewSettings
        visible: sideScanViewCheckButton.hovered || isHovered || sideScanViewCheckButton.sideScanLongPressTriggered
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
                CButton {
                    text: "Updating state"
                    Layout.fillWidth: true
                    Layout.preferredWidth: 200
                    checkable: true
                    checked: true

                    onClicked: {
                        SideScanViewControlMenuController.onUpdateStateChanged(checked)
                    }

                    onFocusChanged: {
                        sideScanViewSettings.focus = true
                    }
                }
                CButton {
                    text: "Track last epoch"
                    Layout.fillWidth: true
                    Layout.preferredWidth: 200
                    checkable: true
                    checked: true

                    onClicked: {
                        SideScanViewControlMenuController.onTrackLastEpochChanged(checked)
                    }

                    onFocusChanged: {
                        sideScanViewSettings.focus = true
                    }
                }
                RowLayout {
                    CText {
                        text: "Theme id:"
                    }
                    SpinBoxCustom  {
                        id: sideScanTheme
                        implicitWidth: 150
                        from: 0
                        to: 4
                        stepSize: 1
                        value: 0

                        onValueChanged: {
                            SideScanViewControlMenuController.onThemeChanged(value)
                        }
                    }
                }
                RowLayout {
                    CText {
                        text: "Angle offset (l/r)°:"
                    }
                    SpinBoxCustom  {
                        implicitWidth: 150
                        from: -90
                        to: 90
                        stepSize: 1
                        value: 0
                        onValueChanged: {
                            SideScanViewControlMenuController.onSetLAngleOffset(value)
                        }
                    }

                    SpinBoxCustom  {
                        implicitWidth: 150
                        from: -90
                        to: 90
                        stepSize: 1
                        value: 0
                        onValueChanged: {
                            SideScanViewControlMenuController.onSetRAngleOffset(value)
                        }
                    }
                }
                ColumnLayout {
                    RowLayout {
                        CText {
                            text: "Tile side pixel size:"
                        }
                        SpinBoxCustom {
                            id: sideScanTileSidePixelSizeSpinBox
                            implicitWidth: 150
                            from: 32
                            to: 2048
                            stepSize: 1
                            value: 256
                        }
                    }
                    RowLayout {
                        CText {
                            text: "Tile height matrix ratio:"
                        }
                        SpinBoxCustom {
                            id: sideScanTileHeightMatrixRatioSpinBox
                            implicitWidth: 150
                            from: 2
                            to: 256
                            stepSize: 1
                            value: 16
                        }
                    }
                    RowLayout {
                        CText {
                            text: "Tile resolution (px/m):"
                        }
                        SpinBoxCustom {
                             id: sideScanTileResolutionSpinBox
                             implicitWidth: 150
                             from: 1
                             to: 100
                             stepSize: 1
                             value: 10
                         }
                    }
                    CButton {
                        text: "Reinit global mesh"
                        Layout.fillWidth: true
                        Layout.preferredWidth: 200

                        onClicked: {
                            SideScanViewControlMenuController.onGlobalMeshChanged(
                                        sideScanTileSidePixelSizeSpinBox.value, sideScanTileHeightMatrixRatioSpinBox.value, 1 / sideScanTileResolutionSpinBox.value)
                        }

                        onFocusChanged: {
                            sideScanViewSettings.focus = true
                        }
                    }
                }
                CButton {
                    text: "Use linear filter"
                    Layout.fillWidth: true
                    Layout.preferredWidth: 200
                    checkable: true

                    onClicked: {
                        SideScanViewControlMenuController.onUseFilterChanged(checked)
                    }

                    onFocusChanged: {
                        sideScanViewSettings.focus = true
                    }
                }
                CButton {
                    text: "Grid/contour visible"
                    Layout.fillWidth: true
                    Layout.preferredWidth: 200
                    checkable: true
                    checked: false

                    onClicked: {
                        SideScanViewControlMenuController.onGridVisibleChanged(checked)
                    }

                    onFocusChanged: {
                        sideScanViewSettings.focus = true
                    }
                }
                CButton {
                    text: "Meas line visible"
                    Layout.fillWidth: true
                    Layout.preferredWidth: 200
                    checkable: true
                    checked: false

                    onClicked: {
                        SideScanViewControlMenuController.onMeasLineVisibleChanged(checked)
                    }

                    onFocusChanged: {
                        sideScanViewSettings.focus = true
                    }
                }
                CButton {
                    text: "Generate grid/contour"
                    Layout.fillWidth: true
                    Layout.preferredWidth: 200
                    checkable: true
                    checked: false

                    onClicked: {
                        SideScanViewControlMenuController.onGenerateGridContourChanged(checked)
                    }

                    onFocusChanged: {
                        sideScanViewSettings.focus = true
                    }
                }
                CButton {
                    text: "Clear"
                    Layout.fillWidth: true
                    Layout.preferredWidth: 200

                    onClicked: {
                        SideScanViewControlMenuController.onClearClicked()
                    }

                    onFocusChanged: {
                        sideScanViewSettings.focus = true
                    }
                }
                CButton {
                    text: "Update"
                    Layout.fillWidth: true
                    Layout.preferredWidth: 200

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
            iconSource: "./icons/zoom-cancel.svg"
            backColor: theme.controlBackColor
            checkable: false
            checked: false
            implicitWidth: theme.controlHeight

            onClicked: Scene3dToolBarController.onFitAllInViewButtonClicked()
        }

        CheckButton {
            id: cancelZoomViewButton
            iconSource: "./icons/ruler-measure.svg"
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
            checked: true
            iconSource: "./icons/click.svg"
            implicitWidth: theme.controlHeight

            onCheckedChanged: {
                Scene3dToolBarController.onBottomTrackVertexEditingModeButtonChecked(checked)
            }
        }

        CheckButton {
            id: boatTrackCheckButton
            backColor: theme.controlBackColor
            borderColor: theme.controlBackColor
            checkedBorderColor: theme.controlBorderColor
            checked: true
            iconSource: "./icons/route.svg"
            implicitWidth: theme.controlHeight


            onCheckedChanged: {
                BoatTrackControlMenuController.onVisibilityCheckBoxCheckedChanged(checked)
            }

            Component.onCompleted: {
                BoatTrackControlMenuController.onVisibilityCheckBoxCheckedChanged(checked)
            }
        }

        CheckButton {
            id: bottomTrackCheckButton
            backColor: theme.controlBackColor
            borderColor: theme.controlBackColor
            checkedBorderColor: theme.controlBorderColor
            checked: true
            iconSource: "./icons/overline.svg"
            implicitWidth: theme.controlHeight

            onCheckedChanged: {
                BottomTrackControlMenuController.onVisibilityCheckBoxCheckedChanged(checked)
            }

            Component.onCompleted: {
                BottomTrackControlMenuController.onVisibilityCheckBoxCheckedChanged(checked)
            }
        }

        CheckButton {
            id: surfaceCheckButton
            backColor: theme.controlBackColor
            borderColor: theme.controlBackColor
            checkedBorderColor: theme.controlBorderColor
            checked: true
            iconSource: "./icons/stack-backward.svg"
            implicitWidth: theme.controlHeight

            onCheckedChanged: {
                SurfaceControlMenuController.onSurfaceVisibilityCheckBoxCheckedChanged(checked)
                SurfaceControlMenuController.onSurfaceContourVisibilityCheckBoxCheckedChanged(checked)
                SurfaceControlMenuController.onSurfaceGridVisibilityCheckBoxCheckedChanged(checked)
                contourVisibilityCheckButton.checked = checked
                gridVisibilityCheckButton.checked = checked
                BottomTrackControlMenuController.onSurfaceStateChanged(checked)
            }

            Component.onCompleted: {
                SurfaceControlMenuController.onSurfaceVisibilityCheckBoxCheckedChanged(checked)
                SurfaceControlMenuController.onSurfaceContourVisibilityCheckBoxCheckedChanged(checked)
                SurfaceControlMenuController.onSurfaceGridVisibilityCheckBoxCheckedChanged(checked)
                contourVisibilityCheckButton.checked = checked
                gridVisibilityCheckButton.checked = checked
                BottomTrackControlMenuController.onSurfaceStateChanged(checked)
            }

            property bool longPressTriggered: false

            MouseArea {
                id: touchArea
                anchors.fill: parent
                onPressed: {
                    longPressTimer.start()
                    surfaceCheckButton.longPressTriggered = false
                }

                onReleased: {
                    if (!surfaceCheckButton.longPressTriggered) {
                        surfaceCheckButton.checked = !surfaceCheckButton.checked
                    }
                    longPressTimer.stop()
                }

                onCanceled: {
                    longPressTimer.stop()
                }
            }

            Timer {
                id: longPressTimer
                interval: 100 // ms
                repeat: false

                onTriggered: {
                    surfaceCheckButton.longPressTriggered = true;
                }
            }
        }

        // usbl view button
        CheckButton {
            id: usblViewCheckButton
            backColor: theme.controlBackColor
            borderColor: theme.controlBackColor
            checkedBorderColor: theme.controlBorderColor
            checked: true
            iconSource: "./icons/gps.svg"
            implicitWidth: theme.controlHeight

            onCheckedChanged: {
                UsblViewControlMenuController.onUsblViewVisibilityCheckBoxCheckedChanged(checked)
            }

            Component.onCompleted: {
                UsblViewControlMenuController.onUsblViewVisibilityCheckBoxCheckedChanged(checked)
            }


            property bool longPressTriggered: false

            MouseArea {
                id: usblViewTouchArea
                anchors.fill: parent
                onPressed: {
                    usblViewLongPressTimer.start()
                    usblViewCheckButton.longPressTriggered = false
                }

                onReleased: {
                    if (!usblViewCheckButton.longPressTriggered) {
                        usblViewCheckButton.checked = !usblViewCheckButton.checked
                    }
                    usblViewLongPressTimer.stop()
                }

                onCanceled: {
                    usblViewLongPressTimer.stop()
                }
            }

            Timer {
                id: usblViewLongPressTimer
                interval: 100 // ms
                repeat: false

                onTriggered: {
                    usblViewCheckButton.longPressTriggered = true;
                }
            }
        }

        // side scan view button
        CheckButton {
            id: sideScanViewCheckButton
            backColor: theme.controlBackColor
            borderColor: theme.controlBackColor
            checkedBorderColor: theme.controlBorderColor
            checked: true
            iconSource: "./icons/map-route.svg"
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
