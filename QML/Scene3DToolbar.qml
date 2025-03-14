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
            ParamSetup {
                paramName: qsTr("Edge limit, m:")

                SpinBoxCustom {
                    id: triangleEdgeLengthLimitSpinBox
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
                }

                onFocusChanged: {
                    surfaceSettings.focus = true
                }
            }

            // export
            RowLayout {
                CTextField {
                    id: exportSurfacePathText
                    hoverEnabled: true
                    Layout.maximumWidth: 200
                    Layout.fillWidth: true
                    placeholderText: qsTr("Enter path")
                }

                CButton {
                    text: "..."
                    Layout.fillWidth: false

                    onClicked: {
                        exportSurfaceFileDialog.open()
                    }
                }

                FileDialog {
                    id: exportSurfaceFileDialog
                    folder: shortcuts.home
                    selectFolder: false
                    selectExisting: false
                    selectMultiple: false
                    defaultSuffix: "csv"
                    nameFilters: ["CSV Files (*.csv)", "All Files (*)"]

                    onAccepted: {
                        exportSurfacePathText.text = exportSurfaceFileDialog.fileUrl
                    }
                }

                CButton {
                    text: qsTr("Export to CSV")
                    Layout.fillWidth: true
                    onClicked:  SurfaceControlMenuController.onExportToCSVButtonClicked(exportSurfacePathText.text)
                }


                Settings {
                    property alias exportSurfaceFolder: exportSurfaceFileDialog.folder
                }

                Settings {
                    property alias exportSurfaceFolderText: exportSurfacePathText.text
                }
            }
        }
    }

    /*// usbl settings
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
                text: qsTr("Update")
                Layout.fillWidth: true
                Layout.preferredWidth: 200

                onClicked: {
                    UsblViewControlMenuController.onUpdateUsblViewButtonClicked()
                }
                onFocusChanged: {
                    usblViewSettings.focus = true
                }
            }

            CButton {
                text: qsTr("Clear")
                Layout.fillWidth: true
                Layout.preferredWidth: 200

                onClicked: {
                    UsblViewControlMenuController.onClearUsblViewButtonClicked()
                }
                onFocusChanged: {
                    usblViewSettings.focus = true
                }
            }
        }
    }*/

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
                CheckButton {
                    id: sideScanTrackLastData
                    text: qsTr("Track last data")
                    Layout.fillWidth: true
                    Layout.preferredWidth: 200
                    checked: true

                    onToggled: {
                        SideScanViewControlMenuController.onTrackLastEpochChanged(checked)
                    }

                    onFocusChanged: {
                        sideScanViewSettings.focus = true
                    }

                    Component.onCompleted: {
                        SideScanViewControlMenuController.onTrackLastEpochChanged(checked)
                    }

                    Settings {
                        property alias sideScanTrackLastData: sideScanTrackLastData.checked
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
                            stepSize: 1
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

/*    // imageViewSettings extra settings
    MenuFrame {
        id: imageViewSettings
        visible: imageViewCheckButton.hovered || isHovered || imageViewCheckButton.imageViewLongPressTriggered
        z: imageViewSettings.visible
        Layout.alignment: Qt.AlignRight

        onIsHoveredChanged: {
            if (Qt.platform.os === "android") {
                if (isHovered) {
                    isHovered = false
                }
            }
            else {
                if (!isHovered || !imageViewCheckButton.hovered) {
                    imageViewCheckButton.imageViewLongPressTriggered = false
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
                imageViewCheckButton.imageViewLongPressTriggered = false
            }
        }

        ColumnLayout {
            RowLayout {
                CTextField {
                    id: imagePathText
                    implicitWidth: 200

                    placeholderText: qsTr("Select an image")
                    Settings {
                        property alias imagePathText: imagePathText.text
                    }
                }
                CheckButton {
                    icon.source: "./icons/file.svg"
                    checkable: false
                    backColor: theme.controlSolidBackColor
                    borderWidth: 0
                    implicitWidth: theme.controlHeight
                    onClicked: {
                        openImageFileDialog.open()
                    }
                    onFocusChanged: {
                        imageViewSettings.focus = true
                    }
                    FileDialog {
                        id: openImageFileDialog
                        title: qsTr("Please choose an image file (.png, .jpg, .bmp)")
                        folder: shortcuts.home
                        selectExisting: true
                        //nameFilters: ["Image (*.png)", "Image (*.jpg)", "Image (*.bmp)"]
                        onAccepted: {
                            imagePathText.text = openImageFileDialog.fileUrl.toString().replace("file:///", "")
                        }
                        onRejected: {
                            console.log("File selection was canceled")
                        }
                    }
                    Settings {
                        property alias openImageFolder: openImageFileDialog.folder
                    }
                }
            }

            RowLayout {
                CText {
                    text: qsTr("lt")
                }
                Item {
                    Layout.fillWidth: true
                }
                ColumnLayout {
                    RowLayout {
                        CText {
                            text: qsTr("x:")
                        }
                        SpinBoxCustom {
                            id: ltXSpinBox
                            implicitWidth: 200
                            from: -1000
                            to: 1000
                            stepSize: 1
                            value: 0
                            editable: true
                            onFocusChanged: {
                                imageViewSettings.focus = true
                            }
                            Settings {
                                property alias ltXSpinBox: ltXSpinBox.value
                            }
                        }
                    }
                    RowLayout {
                        CText {
                            text: qsTr("y:")
                        }
                        SpinBoxCustom {
                            id: ltYSpinBox
                            implicitWidth: 200
                            from: -1000
                            to: 1000
                            stepSize: 1
                            value: 0
                            editable: true
                            onFocusChanged: {
                                imageViewSettings.focus = true
                            }
                            Settings {
                                property alias ltYSpinBox: ltYSpinBox.value
                            }
                        }
                    }
                }
            }
            RowLayout {
                CText {
                    text: qsTr("rb")
                }
                Item {
                    Layout.fillWidth: true
                }
                ColumnLayout {
                    RowLayout {
                        CText {
                            text: qsTr("x:")
                        }
                        SpinBoxCustom {
                            id: rbXSpinBox
                            implicitWidth: 200
                            from: -1000
                            to: 1000
                            stepSize: 1
                            value: 0
                            onFocusChanged: {
                                imageViewSettings.focus = true
                            }
                            Settings {
                                property alias rbXSpinBox: rbXSpinBox.value
                            }
                        }
                    }
                    RowLayout {
                        CText {
                            text: qsTr("y:")
                        }
                        SpinBoxCustom {
                            id: rbYSpinBox
                            implicitWidth: 200
                            from: -1000
                            to: 1000
                            stepSize: 1
                            value: 0
                            onFocusChanged: {
                                imageViewSettings.focus = true
                            }
                            Settings {
                                property alias rbYSpinBox: rbYSpinBox.value
                            }
                        }
                    }
                }
            }
            RowLayout {
                CText {
                    text: qsTr("z:")
                }
                Item {
                    Layout.fillWidth: true
                }
                SpinBoxCustom {
                    id: zSpinBox
                    implicitWidth: 200
                    from: -1000
                    to: 1000
                    stepSize: 1
                    value: -1
                    onFocusChanged: {
                        imageViewSettings.focus = true
                    }
                    Settings {
                        property alias zSpinBox: zSpinBox.value
                    }
                }
            }

            CheckButton {
                id: imageLinearFilter
                text: qsTr("Use linear filter")
                Layout.fillWidth: true
                Layout.preferredWidth: 200

                onClicked: {
                    ImageViewControlMenuController.onUseFilterChanged(checked)
                }

                onFocusChanged: {
                    imageViewSettings.focus = true
                }

                Settings {
                    property alias imageLinearFilter: imageLinearFilter.checked
                }
            }
            CButton {
                text: qsTr("Update")
                Layout.fillWidth: true
                Layout.preferredWidth: 200

                onClicked: {
                    // ImageViewControlMenuController.onUpdateClicked(imagePathText.text, ltXSpinBox.value, ltYSpinBox.value, rbXSpinBox.value, rbYSpinBox.value, zSpinBox.value)
                    ImageViewControlMenuController.onUpdateClicked(imagePathText.text,  40.165167,  44.469925,  40.155800, 44.486353, zSpinBox.value)
                }

                onFocusChanged: {
                    imageViewSettings.focus = true
                }
            }
        }
    }*/

/*
    // mapViewSettings extra settings
    MenuFrame {
        id: mapViewSettings
        visible: mapViewCheckButton.hovered || isHovered || mapViewCheckButton.mapViewLongPressTriggered
        z: mapViewSettings.visible
        Layout.alignment: Qt.AlignRight

        onIsHoveredChanged: {
            if (Qt.platform.os === "android") {
                if (isHovered) {
                    isHovered = false
                }
            }
            else {
                if (!isHovered || !mapViewCheckButton.hovered) {
                    mapViewCheckButton.mapViewLongPressTriggered = false
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
                mapViewCheckButton.mapViewLongPressTriggered = false
            }
        }

        ColumnLayout {
            CButton {
                text: qsTr("Update")
                Layout.fillWidth: true
                Layout.preferredWidth: 200

                onClicked: {
                    MapViewControlMenuController.onUpdateClicked()
                }

                onFocusChanged: {
                    mapViewSettings.focus = true
                }
            }
        }
    }
*/
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
            id: navigationArrowCheckButton
            objectName: "navigationArrowCheckButton"
            backColor: theme.controlBackColor
            borderColor: theme.controlBackColor
            checkedBorderColor: theme.controlBorderColor
            checked: true
            iconSource: "./icons/speedboat.svg"
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

/*        // usbl view button
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
        }*/

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

            Component.onCompleted: {
                SideScanViewControlMenuController.onVisibilityChanged(checked)
            }

            Settings {
                property alias sideScanViewCheckButton: sideScanViewCheckButton.checked
            }
        }

/*        // image view button
        CheckButton {
            id: imageViewCheckButton
            backColor: theme.controlBackColor
            borderColor: theme.controlBackColor
            checkedBorderColor: theme.controlBorderColor
            checked: true
            iconSource: "./icons/photo.svg"
            implicitWidth: theme.controlHeight

            onCheckedChanged: {
                ImageViewControlMenuController.onVisibilityChanged(checked)
            }

            property bool imageViewLongPressTriggered: false

            MouseArea {
                id: imageViewTouchArea
                anchors.fill: parent
                onPressed: {
                    imageViewLongPressTimer.start()
                    imageViewCheckButton.imageViewLongPressTriggered = false
                }

                onReleased: {
                    if (!imageViewCheckButton.imageViewLongPressTriggered) {
                        imageViewCheckButton.checked = !imageViewCheckButton.checked
                    }
                    imageViewLongPressTimer.stop()
                }

                onCanceled: {
                    imageViewLongPressTimer.stop()
                }
            }

            Timer {
                id: imageViewLongPressTimer
                interval: 100 // ms
                repeat: false

                onTriggered: {
                    imageViewCheckButton.imageViewLongPressTriggered = true;
                }
            }
        }*/

        // mapView button
        CheckButton {
            id: mapViewCheckButton
            backColor: theme.controlBackColor
            borderColor: theme.controlBackColor
            checkedBorderColor: theme.controlBorderColor
            checked: true
            iconSource: "./icons/map.svg"
            implicitWidth: theme.controlHeight

            onCheckedChanged: {
                MapViewControlMenuController.onVisibilityChanged(checked)
            }
/*
            property bool mapViewLongPressTriggered: false

            MouseArea {
                id: mapViewTouchArea
                anchors.fill: parent
                onPressed: {
                    mapViewLongPressTimer.start()
                    mapViewCheckButton.mapViewLongPressTriggered = false
                }

                onReleased: {
                    if (!mapViewCheckButton.mapViewLongPressTriggered) {
                        mapViewCheckButton.checked = !mapViewCheckButton.checked
                    }
                    mapViewLongPressTimer.stop()
                }

                onCanceled: {
                    mapViewLongPressTimer.stop()
                }
            }

            Timer {
                id: mapViewLongPressTimer
                interval: 100 // ms
                repeat: false

                onTriggered: {
                    mapViewCheckButton.mapViewLongPressTriggered = true;
                }
            }
*/

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
