import QtQuick 2.15
import SceneGraphRendering 1.0
import QtQuick.Window 2.15
import QtQuick.Layouts 1.15
import Qt.labs.settings 1.1
import QtQuick.Dialogs 1.2
import QtQuick.Controls 2.15
import WaterFall 1.0
import KoggerCommon 1.0


ApplicationWindow  {
    id:            mainview
    visible:       true
    width:         1024
    minimumWidth:  512
    height:        512
    minimumHeight: 256
    color:         "black"
    title:         qsTr("KoggerApp, KOGGER")

    readonly property int _rightBarWidth:                360
    readonly property int _activeObjectParamsMenuHeight: 500
    readonly property int _sceneObjectsListHeight:       300

    Settings {
            id: appSettings
            property bool isFullScreen: false
            //property int savedX: 100
            //property int savedY: 100
    }

    Loader {
        id: stateGroupLoader
        active: (Qt.platform.os === "windows")
        sourceComponent: stateGroupComp
    }

    Component {
        id: stateGroupComp
        StateGroup {
            state: appSettings.isFullScreen ? "FullScreen" : "Windowed"

            states: [
                State {
                    name: "FullScreen"
                    StateChangeScript {
                        script: { // empty
                        }
                    }
                    PropertyChanges {
                        target: mainview
                        visibility: "FullScreen"

                        flags: Qt.FramelessWindowHint
                        x: 0
                        y: - 1
                        width: Screen.width
                        height: Screen.height + 1
                    }
                },
                State {
                    name: "Windowed"
                    StateChangeScript {
                        script: {
                            if (Qt.platform.os !== "android") {
                                mainview.flags = Qt.Window
                            }
                        }
                    }
                    PropertyChanges {
                        target: mainview
                        visibility: "Windowed"
                    }
                }
            ]
        }
    }

    Component.onCompleted: {
        //theme.updateResCoeff(); // for UI scaling

        menuBar.languageChanged.connect(handleChildSignal)

        if (Qt.platform.os !== "windows") {
            if (appSettings.isFullScreen) {
                mainview.showFullScreen();
            }
        }

        // contacts
        function setupConnections() {
            if (typeof contacts !== "undefined") {
                contactConnections.target = contacts;
            }
            else {
                Qt.callLater(setupConnections);
            }
        }
        Qt.callLater(setupConnections);
    }

    // banner on languageChanged
    property bool showBanner: false
    property string selectedLanguageStr: qsTr("Undefined")

    Rectangle {
        id: banner
        anchors.fill: parent
        color: "black"
        opacity: 0.8
        visible: showBanner

        Column {
            anchors.centerIn: parent
            spacing: 20

            Text {
                text: qsTr("Please restart the application to apply the language change") + " (" + selectedLanguageStr + ")"
                color: "white"
                font.pixelSize: 24
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
            }

            CButton {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Ok")
                onClicked: {
                    mainview.showBanner = false
                }
            }
        }
    }

    //-> drag-n-drop
    property string draggedFilePath: ""

    Rectangle {
        id: overlay
        anchors.fill: parent
        color: "white"
        opacity: 0
        z: 1

        Behavior on opacity {
            NumberAnimation {
                duration: 300
            }
        }
    }

    DropArea {
        anchors.fill: parent

        onEntered: {
            if (!showBanner) {
                draggedFilePath = ""
                if (drag.hasUrls) {
                    for (var i = 0; i < drag.urls.length; ++i) {
                        var url = drag.urls[i]
                        var filePath = url.replace("file:///", "").toLowerCase()
                        if (filePath.endsWith(".klf")) {
                            draggedFilePath = filePath
                            overlay.opacity = 0.3
                            break
                        }
                    }
                }
            }
        }

        onExited: {
            if (!showBanner) {
                overlay.opacity = 0
                draggedFilePath = ""
            }
        }

        onDropped: {
            if (!showBanner) {
                if (draggedFilePath !== "") {
                    core.openLogFile(draggedFilePath, false, true)
                    overlay.opacity = 0
                    draggedFilePath = ""
                }
                overlay.opacity = 0
            }
        }
    }
    // drag-n-drop <-

    SplitView {
        id: splitLayer
        visible: !showBanner
        Layout.fillHeight: true
        Layout.fillWidth:  true
        anchors.fill:      parent
        orientation:       Qt.Vertical

        Keys.onReleased: {
            let sc = event.nativeScanCode.toString()
            let hotkeyData = hotkeysMapScan[sc];
            if (hotkeyData === undefined) {
                return
            }

            let fn = hotkeyData["functionName"];
            let p = hotkeyData["parameter"];

            // high priority
            if (fn === "toggleFullScreen") {
                if (Qt.platform.os === "windows") {
                    appSettings.isFullScreen = !appSettings.isFullScreen
                }
                else if (Qt.platform.os === "linux") {
                    if (mainview.visibility === Window.FullScreen) {
                        mainview.showNormal();
                        appSettings.isFullScreen = false;
                    }
                    else {
                        appSettings.isFullScreen = true;
                        mainview.showFullScreen();
                    }
                }
                return;
            }
            if (fn === "openFile") {
                core.openLogFile(menuBar.filePath, false, false)
                return;
            }
            if (fn === "closeFile") {
                core.closeLogFile()
                return;
            }
            if (fn === "updateBottomTrack") {
                menuBar.updateBottomTrack()
            }
            if (fn === "updateSurface") {
                scene3DToolbar.updateSurface()
            }
            if (fn === "updateMosaic") {
                scene3DToolbar.updateMosaic()
            }
            if (fn === "closeSettings") {
                menuBar.closeMenus()
                splitLayer.focus = true
                return;
            }

            if (mainview.activeFocusItem &&
                (mainview.activeFocusItem instanceof TextEdit || mainview.activeFocusItem instanceof TextField)) {
                return;
            }

            if (fn !== undefined) {
                if (p === undefined) {
                    p = 5
                }

                switch (fn) {
                case "horScrollLeft": {
                    waterView.horScrollEvent(-p)
                    break
                }
                case "horScrollRight": {
                    waterView.horScrollEvent(p)
                    break
                }
                case "verScrollUp": {
                    waterView.verScrollEvent(-p)
                    break
                }
                case "verScrollDown": {
                    waterView.verScrollEvent(p)
                    break
                }
                case "verZoomOut": {
                    waterView.verZoomEvent(-p)
                    break
                }
                case "verZoomIn": {
                    waterView.verZoomEvent(p)
                    break
                }
                case "increaseLowLevel": {
                    let newLow = Math.min(120, waterView.getLowEchogramLevel() + p)
                    let newHigh = waterView.getHighEchogramLevel()
                    if (newLow > newHigh) newHigh = newLow
                    waterView.plotEchogramSetLevels(newLow, newHigh)
                    waterView.setLevels(newLow, newHigh)
                    break
                }
                case "decreaseLowLevel": {
                    let newLow = Math.max(0, waterView.getLowEchogramLevel() - p)
                    let newHigh = waterView.getHighEchogramLevel()
                    waterView.plotEchogramSetLevels(newLow, newHigh)
                    waterView.setLevels(newLow, newHigh)
                    break
                }
                case "increaseHighLevel": {
                    let newHigh = Math.min(120, waterView.getHighEchogramLevel() + p)
                    let newLow = waterView.getLowEchogramLevel()
                    waterView.plotEchogramSetLevels(newLow, newHigh)
                    waterView.setLevels(newLow, newHigh)
                    break
                }
                case "decreaseHighLevel": {
                    let newHigh = Math.max(0, waterView.getHighEchogramLevel() - p)
                    let newLow = waterView.getLowEchogramLevel()
                    if (newHigh < newLow) newLow = newHigh
                    waterView.plotEchogramSetLevels(newLow, newHigh)
                    waterView.setLevels(newLow, newHigh)
                    break
                }
                case "prevTheme": {
                    let themeId = waterView.getThemeId()
                    if (themeId > 0) waterView.plotEchogramTheme(themeId - 1)
                    break
                }
                case "nextTheme": {
                    let themeId = waterView.getThemeId()
                    if (themeId < 4) waterView.plotEchogramTheme(themeId + 1)
                    break
                }
                case "clickConnections": {
                    menuBar.clickConnections()
                    break
                }
                case "clickSettings": {
                    menuBar.clickSettings()
                    break
                }
                case "click3D": {
                    menuBar.click3D()
                    break
                }
                case "click2D": {
                    menuBar.click2D()
                    break
                }
                default: {
                    break
                }
                }
            }
        }

        handle: Rectangle {
            // implicitWidth:  5
            implicitHeight: theme.controlHeight/2
            color:          SplitHandle.pressed ? "#A0A0A0" : "#707070"

            Rectangle {
                width:  parent.width
                height: 1
                color:  "#A0A0A0"
            }

            Rectangle {
                y:      parent.height
                width:  parent.width
                height: 1
                color:  "#A0A0A0"
            }
        }

        GridLayout {
            id:                   visualisationLayout
            SplitView.fillHeight: true
            // anchors.fill: parent
            Layout.fillHeight: true
            Layout.fillWidth:  true
            rowSpacing: 0
            columnSpacing: 0
            rows    : mainview.width > mainview.height ? 1 : 2
            columns : mainview.width > mainview.height ? 2 : 1

            property int lastKeyPressed: Qt.Key_unknown

            Keys.onPressed: {
                visualisationLayout.lastKeyPressed = event.key;
            }

            Keys.onReleased: {
                visualisationLayout.lastKeyPressed = Qt.Key_unknown;
            }

            GraphicsScene3dView {
                id:                renderer
                visible: menuBar.is3DVisible
                objectName: "GraphicsScene3dView"
                Layout.fillHeight: true
                Layout.fillWidth:  true
                focus:             true

                property bool longPressTriggered: false

                KWaitProgressBar{
                    id:        surfaceProcessingProgressBar
                    objectName: "surfaceProcessingProgressBar"
                    text:      qsTr("Calculating surface\nPlease wait...")
                    textColor: "white"
                    visible:   false
                }

                KWaitProgressBar{
                    id:        sideScanProcessingProgressBar
                    objectName: "sideScanProcessingProgressBar"
                    text:      qsTr("Calculating mosaic\nPlease wait...")
                    textColor: "white"
                    visible:  core.isMosaicUpdatingInThread && core.isSideScanPerformanceMode
                }

                PinchArea {
                    id:           pinch3D
                    anchors.fill: parent
                    enabled:      true

                    onPinchStarted: {
                        menuBlock.visible = false
                        mousearea3D.enabled = false
                    }

                    onPinchUpdated: {
                        var shiftScale = pinch.scale - pinch.previousScale;
                        var shiftAngle = pinch.angle - pinch.previousAngle;
                        renderer.pinchTrigger(pinch.previousCenter, pinch.center, shiftScale, shiftAngle)
                    }

                    onPinchFinished: {
                        mousearea3D.enabled = true
                    }

                    MouseArea {
                        id: mousearea3D
                        enabled:              true
                        anchors.fill:         parent
                        acceptedButtons:      Qt.LeftButton | Qt.RightButton | Qt.MiddleButton
                        focus:                true
                        hoverEnabled:         true
                        Keys.enabled:         true
                        Keys.onDeletePressed: renderer.keyPressTrigger(event.key)

                        property int lastMouseKeyPressed: Qt.NoButton // TODO: maybe this mouseArea should be outside pinchArea
                        property point startMousePos: Qt.point(-1, -1)
                        property bool wasMoved: false
                        property real mouseThreshold: 15
                        property bool vertexMode: false

                        onEntered: {
                            mousearea3D.forceActiveFocus();
                        }

                        onWheel: {
                            renderer.mouseWheelTrigger(wheel.buttons, wheel.x, wheel.y, wheel.angleDelta, visualisationLayout.lastKeyPressed)
                        }

                        onPositionChanged: {
                            if (Qt.platform.os === "android") {
                                if (!wasMoved) {
                                    var delta = Math.sqrt(Math.pow((mouse.x - startMousePos.x), 2) + Math.pow((mouse.y - startMousePos.y), 2));
                                    if (delta > mouseThreshold) {
                                        wasMoved = true;
                                    }
                                }
                                if (renderer.longPressTriggered && !wasMoved) {
                                    if (!vertexMode) {
                                        renderer.switchToBottomTrackVertexComboSelectionMode(mouse.x, mouse.y)
                                    }
                                    vertexMode = true
                                }
                            }

                            renderer.mouseMoveTrigger(mouse.buttons, mouse.x, mouse.y, visualisationLayout.lastKeyPressed)
                        }

                        onPressed: {
                            menuBlock.visible = false
                            startMousePos = Qt.point(mouse.x, mouse.y)
                            wasMoved = false
                            vertexMode = false
                            longPressTimer.start()
                            renderer.longPressTriggered = false

                            lastMouseKeyPressed = mouse.buttons
                            renderer.mousePressTrigger(mouse.buttons, mouse.x, mouse.y, visualisationLayout.lastKeyPressed)
                        }

                        onReleased: {
                            startMousePos = Qt.point(-1, -1)
                            wasMoved = false
                            longPressTimer.stop()

                            renderer.mouseReleaseTrigger(lastMouseKeyPressed, mouse.x, mouse.y, visualisationLayout.lastKeyPressed)

                            if (mouse.button === Qt.RightButton || (Qt.platform.os === "android" && vertexMode)) {
                                menuBlock.position(mouse.x, mouse.y)
                            }

                            vertexMode = false

                            lastMouseKeyPressed = Qt.NoButton
                        }

                        onCanceled: {
                            startMousePos = Qt.point(-1, -1)
                            wasMoved = false
                            vertexMode = false
                            longPressTimer.stop()
                        }
                    }
                }

                Timer {
                    id: longPressTimer
                    interval: 500 // ms
                    repeat: false

                    onTriggered: {
                        renderer.longPressTriggered = true
                    }
                }

                Scene3DToolbar{
                    id:                       scene3DToolbar
                    // anchors.bottom:              parent.bottom
                    y:renderer.height - height - 2
                    anchors.horizontalCenter: parent.horizontalCenter
                    // anchors.rightMargin:      20
                    Keys.forwardTo:           [mousearea3D]
                }

                CContact {
                    id: contactDialog
                    visible: false
                    offsetOpacityArea: 20 // increase in 3D

                    onInputAccepted: {
                        contacts.setContact(contactDialog.indx, contactDialog.inputFieldText)
                    }
                    onSetButtonClicked: {
                        contacts.setContact(contactDialog.indx, contactDialog.inputFieldText)
                    }
                    onDeleteButtonClicked: {
                        contacts.deleteContact(contactDialog.indx)
                    }
                    onCopyButtonClicked: {
                        contacts.update()
                    }
                }

                Connections {
                    id: contactConnections
                    target: null // contacts will init later
                    function onContactChanged() {
                        contactDialog.visible = contacts.contactVisible
                        if (contacts.contactVisible) {
                            contactDialog.info           = contacts.contactInfo
                            contactDialog.inputFieldText = contacts.contactInfo
                            contactDialog.x              = contacts.contactPositionX
                            contactDialog.y              = contacts.contactPositionY
                            contactDialog.indx           = contacts.contactIndx
                            contactDialog.lat            = contacts.contactLat
                            contactDialog.lon            = contacts.contactLon
                            contactDialog.depth          = contacts.contactDepth
                        }
                    }
                }

                RowLayout {
                    id: menuBlock
                    Layout.alignment: Qt.AlignHCenter
                    spacing: 1
                    visible: false
                    Layout.margins: 0

                    function position(mx, my) {
                        var oy = renderer.height - (my + implicitHeight)
                        if (oy < 0) {
                            my = my + oy
                        }
                        if (my < 0) {
                            my = 0
                        }
                        var ox = renderer.width - (mx - implicitWidth)
                        if (ox < 0) {
                            mx = mx + ox
                        }
                        x = mx
                        y = my
                        visible = true
                    }

                    ButtonGroup { id: pencilbuttonGroup }

                    CheckButton {
                        icon.source: "qrc:/icons/ui/arrow_bar_to_down.svg"
                        backColor: theme.controlBackColor
                        checkable: false
                        implicitWidth: theme.controlHeight

                        onClicked: {
                            renderer.bottomTrackActionEvent(BottomTrack.MinDistProc)
                            menuBlock.visible = false
                        }

                        ButtonGroup.group: pencilbuttonGroup
                    }

                    CheckButton {
                        icon.source: "qrc:/icons/ui/arrow_bar_to_up.svg"
                        backColor: theme.controlBackColor
                        checkable: false
                        implicitWidth: theme.controlHeight

                        onClicked: {
                            renderer.bottomTrackActionEvent(BottomTrack.MaxDistProc)
                            menuBlock.visible = false
                        }

                        ButtonGroup.group: pencilbuttonGroup
                    }

                    CheckButton {
                        icon.source: "qrc:/icons/ui/eraser.svg"
                        backColor: theme.controlBackColor
                        checkable: false
                        implicitWidth: theme.controlHeight

                        onClicked: {
                            renderer.bottomTrackActionEvent(BottomTrack.ClearDistProc)
                            menuBlock.visible = false
                        }

                        ButtonGroup.group: pencilbuttonGroup
                    }

                    CheckButton {
                        icon.source: "qrc:/icons/ui/x.svg"
                        backColor: theme.controlBackColor
                        checkable: false
                        implicitWidth: theme.controlHeight

                        onClicked: {
                            renderer.bottomTrackActionEvent(BottomTrack.Undefined)

                            menuBlock.visible = false
                        }

                        ButtonGroup.group: pencilbuttonGroup
                    }
                }
            }


            Item {
                Layout.fillHeight: true
                Layout.fillWidth: true
                visible: menuBar.is2DVisible

                GridLayout {
                    anchors.fill: parent
                    rows    : 2
                    columns : 1
                    columnSpacing: 0
                    rowSpacing: 0

                    Plot2D {
                        id: waterView
                        Layout.fillHeight: true
                        Layout.fillWidth: true
                        Layout.margins: 0

                        Layout.rowSpan   : 1
                        Layout.columnSpan: 1
                        focus: true
                        horizontal: menuBar.is2DHorizontal


                    }

                    // Plot2D {
                    //     id: waterView1

                    //     Layout.fillHeight: true
                    //     Layout.fillWidth: true

                    //     Layout.rowSpan   : 1
                    //     Layout.columnSpan: 1
                    //     focus: true
                    //     horizontal: menuBar.is2DHorizontal
                    // }

                    CSlider {
                        id: historyScroll
                        Layout.margins: 0
                        Layout.fillWidth: true
                        Layout.fillHeight: false
                        Layout.columnSpan: parent.columns
                        implicitHeight: theme.controlHeight
                        height: theme.controlHeight
                        value: waterView.timelinePosition
                        stepSize: 0.0001
                        from: 0
                        to: 1

                        onValueChanged: {
                            core.setTimelinePosition(value);
                        }
                        onMoved: {
                            core.resetAim();
                        }
                    }
                }
            }
        }

        Console {
            id:                      console_vis
            visible:                 theme.consoleVisible
            SplitView.minimumHeight: 150
            SplitView.maximumHeight: mainview.height - theme.controlHeight/2 - theme.controlHeight
        }
    }

    MenuFrame {
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        visible: (deviceManagerWrapper.pilotArmState >= 0) && !showBanner
        isDraggable: true
        isOpacityControlled: true
        Keys.forwardTo: [splitLayer]

        ColumnLayout {
            RowLayout {
                Layout.alignment: Qt.AlignHCenter
                CheckButton {
                    // text: checked ? "Armed" : "Disarmed"
                    icon.source: checked ? "qrc:/icons/ui/propeller.svg" : "qrc:/icons/ui/propeller_off.svg"
                    checked: deviceManagerWrapper.pilotArmState == 1
                    color: "white"
                    backColor: "red"
                    // checkedColor: "white"
                    // checkedBackColor: "transparent"
                    borderColor: "transparent"
                    checkedBorderColor: theme.textColor
                    implicitWidth: theme.controlHeight
                }

                ButtonGroup { id: autopilotModeGroup }

                CheckButton {
                    // Layout.fillWidth: true
                    icon.source: "qrc:/icons/ui/direction_arrows.svg"
                    checked: deviceManagerWrapper.pilotModeState == 0 // "Manual"
                    onCheckedChanged: {
                    }
                    ButtonGroup.group: autopilotModeGroup
                    implicitWidth: theme.controlHeight
                }

                CheckButton {
                    // Layout.fillWidth: true
                    icon.source: "qrc:/icons/ui/route.svg"
                    checked: deviceManagerWrapper.pilotModeState == 10 // "Auto"
                    onCheckedChanged: {
                    }
                    ButtonGroup.group: autopilotModeGroup
                    implicitWidth: theme.controlHeight
                }

                CheckButton {
                    // Layout.fillWidth: true
                    icon.source: "qrc:/icons/ui/anchor.svg"
                    checked: deviceManagerWrapper.pilotModeState == 5 // "Loiter"
                    onCheckedChanged: {
                    }
                    ButtonGroup.group: autopilotModeGroup
                    implicitWidth: theme.controlHeight
                }

                CheckButton {
                    // Layout.fillWidth: true
                    icon.source: "qrc:/icons/ui/map_pin.svg"
                    checked: deviceManagerWrapper.pilotModeState == 15 // "Guided"
                    onCheckedChanged: {
                    }
                    ButtonGroup.group: autopilotModeGroup
                    implicitWidth: theme.controlHeight
                }

                CheckButton {
                    // Layout.fillWidth: true
                    icon.source: "qrc:/icons/ui/home.svg"
                    checked: deviceManagerWrapper.pilotModeState == 11 || deviceManagerWrapper.pilotModeState == 12  // "RTL" || "SmartRTL"
                    onCheckedChanged: {
                    }
                    ButtonGroup.group: autopilotModeGroup
                    implicitWidth: theme.controlHeight
                }

                // CCombo  {
                //     id: pilotModeState
                //     visible: deviceManagerWrapper.pilotModeState >= 0
                //     model: [
                //         "Manual",
                //         "Acro",
                //         "Steering",
                //         "Hold",
                //         "Loiter",
                //         "Follow",
                //         "Simple",
                //         "Dock",
                //         "Circle",
                //         "Auto",
                //         "RTL",
                //         "SmartRTL",
                //         "Guided",
                //         "Mode16",
                //         "Mode17"
                //     ]
                //     currentIndex: deviceManagerWrapper.pilotModeState

                //     onCurrentIndexChanged: {
                //         if(currentIndex != deviceManagerWrapper.pilotModeState) {
                //             currentIndex = deviceManagerWrapper.pilotModeState
                //         }
                //     }
            }

            RowLayout {
                CText {
                    id: fcTextBatt
                    // Layout.margins: 4
                    visible: isFinite(deviceManagerWrapper.vruVoltage)
                    rightPadding: 4
                    leftPadding: 4
                    text: deviceManagerWrapper.vruVoltage.toFixed(1) + qsTr(" V   ") + deviceManagerWrapper.vruCurrent.toFixed(1) + qsTr(" A   ") + deviceManagerWrapper.vruVelocityH.toFixed(2) + qsTr(" m/s ")
                }
                CText {
                    id: errText
                    //visible: isFinite(deviceManagerWrapper.vruVoltage)
                    rightPadding: 4
                    leftPadding: 4
                    text: deviceManagerWrapper.averageChartLosses + qsTr(" %")
                }
            }
        }
    }

    MenuBar {
        id:                menuBar
        objectName:        "menuBar"
        Layout.fillHeight: true
        Keys.forwardTo:    [splitLayer, mousearea3D]
        height: visualisationLayout.height
        targetPlot: waterView
        visible: !showBanner
    }

    function handleChildSignal(langStr) {
        mainview.showBanner = true
        selectedLanguageStr = langStr
    }

    Connections {
        target: SurfaceControlMenuController

        function onSurfaceProcessorTaskStarted() {
            surfaceProcessingProgressBar.visible = true
        }

        function onSurfaceProcessorTaskFinished() {
            surfaceProcessingProgressBar.visible = false
        }
    }

    // banner on file opening
    Rectangle {
        id: fileOpeningOverlay
        color: theme.controlBackColor
        opacity: 0.8
        radius: 10
        anchors.centerIn: parent
        visible: core.isFileOpening && !core.isSeparateReading
        implicitWidth: textItem.implicitWidth + 40
        implicitHeight: textItem.implicitHeight + 40

        Column {
            anchors.centerIn: parent
            spacing: 10

            Text {
                id: textItem
                text: qsTr("Please wait, the file is opening")
                color: "white"
                font.pixelSize: 20
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
            }
        }
    }
}
