import QtQuick 2.15
import SceneGraphRendering 1.0
import QtQuick.Window 2.15
import QtQuick.Layouts 1.15
import QtQuick.Dialogs
import QtQuick.Controls 2.15
import WaterFall 1.0
import BottomTrack 1.0
import QtCore


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

    function setFullScreenMode(enabled) {
        appSettings.isFullScreen = enabled
        if (enabled) {
            mainview.showFullScreen()
        }
        else {
            mainview.showNormal()
        }
    }

    function toggleFullScreenMode() {
        setFullScreenMode(mainview.visibility !== Window.FullScreen)
    }

    function handleUpdateBottomTrack() {
        menuBar.updateBottomTrack()
    }

    Component.onCompleted: {
        theme.updateResCoeff()

        scene3DToolbar.updateBottomTrack.connect(handleUpdateBottomTrack)
        menuBar.languageChanged.connect(handleChildSignal)
        menuBar.syncPlotEnabled.connect(handleSyncPlotEnabled)
        waterViewFirst.plotCursorChanged.connect(handlePlotCursorChanged)
        waterViewSecond.plotCursorChanged.connect(handlePlotCursorChanged)
        waterViewFirst.updateOtherPlot.connect(handleUpdateOtherPlot)
        waterViewSecond.updateOtherPlot.connect(handleUpdateOtherPlot)
        waterViewFirst. plotPressed.connect(handlePlotPressed)
        waterViewSecond.plotPressed.connect(handlePlotPressed)
        waterViewFirst. plotReleased.connect(handlePlotReleased)
        waterViewSecond.plotReleased.connect(handlePlotReleased)
        waterViewFirst.settingsClicked.connect(onPlotSettingsClicked)
        waterViewSecond.settingsClicked.connect(onPlotSettingsClicked)
        menuBar.menuBarSettingOpened.connect(onMenuBarSettingsOpened)

        scene3DToolbar.mosaicLAngleOffsetChanged.connect(handleMosaicLOffsetChanged)
        scene3DToolbar.mosaicRAngleOffsetChanged.connect(handleMosaicROffsetChanged)

        if (appSettings.isFullScreen) {
            mainview.showFullScreen()
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

        onEntered: function(drag) {
            if (!showBanner) {
                draggedFilePath = ""
                if (drag.hasUrls) {
                    for (var i = 0; i < drag.urls.length; ++i) {
                        var url = drag.urls[i]
                        var filePath = url.toString().replace("file:///", "").toLowerCase()
                        if (filePath.endsWith(".klf") ||
                            filePath.endsWith(".xtf")) {
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

        Keys.onReleased: function(event) {
            let sc = event.nativeScanCode.toString()
            let hotkeyData = hotkeysMapScan[sc];
            if (hotkeyData === undefined) {
                return
            }

            let fn = hotkeyData["functionName"];
            let p = hotkeyData["parameter"];

            // high priority
            if (fn === "toggleFullScreen") {
                toggleFullScreenMode()
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
            if (fn === "updateMosaic") {
                scene3DToolbar.updateMosaic()
            }
            if (fn === "closeSettings") {
                waterViewFirst.closeSettings()
                if (waterViewSecond.enabled) {
                    waterViewSecond.closeSettings()
                }
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
                    waterViewFirst.horScrollEvent(-p)
                    if (waterViewSecond.enabled) {
                        waterViewSecond.horScrollEvent(-p)
                    }
                    break
                }
                case "horScrollRight": {
                    waterViewFirst.horScrollEvent(p)
                    if (waterViewSecond.enabled) {
                        waterViewSecond.horScrollEvent(p)
                    }
                    break
                }
                case "verScrollUp": {
                    waterViewFirst.verScrollEvent(-p)
                    if (waterViewSecond.enabled) {
                        waterViewSecond.verScrollEvent(-p)
                    }
                    break
                }
                case "verScrollDown": {
                    waterViewFirst.verScrollEvent(p)
                    if (waterViewSecond.enabled) {
                        waterViewSecond.verScrollEvent(p)
                    }
                    break
                }
                case "verZoomOut": {
                    waterViewFirst.verZoomEvent(-p)
                    if (waterViewSecond.enabled) {
                        waterViewSecond.verZoomEvent(-p)
                    }
                    break
                }
                case "verZoomIn": {
                    waterViewFirst.verZoomEvent(p)
                    if (waterViewSecond.enabled) {
                        waterViewSecond.verZoomEvent(p)
                    }
                    break
                }
                case "increaseLowLevel": {
                    let newLow = Math.min(120, waterViewFirst.getLowEchogramLevel() + p)
                    let newHigh = waterViewFirst.getHighEchogramLevel()
                    if (newLow > newHigh) newHigh = newLow
                    waterViewFirst.plotEchogramSetLevels(newLow, newHigh)
                    waterViewFirst.setLevels(newLow, newHigh)
                    if (waterViewSecond.enabled) {
                        let newSLow = Math.min(120, waterViewSecond.getLowEchogramLevel() + p)
                        let newSHigh = waterViewSecond.getHighEchogramLevel()
                        if (newSLow > newSHigh) newSHigh = newSLow
                        waterViewSecond.plotEchogramSetLevels(newSLow, newSHigh)
                        waterViewSecond.setLevels(newSLow, newSHigh)
                    }
                    break
                }
                case "decreaseLowLevel": {
                    let newLow = Math.max(0, waterViewFirst.getLowEchogramLevel() - p)
                    let newHigh = waterViewFirst.getHighEchogramLevel()
                    waterViewFirst.plotEchogramSetLevels(newLow, newHigh)
                    waterViewFirst.setLevels(newLow, newHigh)
                    if (waterViewSecond.enabled) {
                        let newSLow = Math.max(0, waterViewSecond.getLowEchogramLevel() - p)
                        let newSHigh = waterViewSecond.getHighEchogramLevel()
                        waterViewSecond.plotEchogramSetLevels(newSLow, newSHigh)
                        waterViewSecond.setLevels(newSLow, newSHigh)
                    }
                    break
                }
                case "increaseHighLevel": {
                    let newHigh = Math.min(120, waterViewFirst.getHighEchogramLevel() + p)
                    let newLow = waterViewFirst.getLowEchogramLevel()
                    waterViewFirst.plotEchogramSetLevels(newLow, newHigh)
                    waterViewFirst.setLevels(newLow, newHigh)
                    if (waterViewSecond.enabled) {
                        let newSHigh = Math.min(120, waterViewSecond.getHighEchogramLevel() + p)
                        let newSLow = waterViewSecond.getLowEchogramLevel()
                        waterViewSecond.plotEchogramSetLevels(newSLow, newSHigh)
                        waterViewSecond.setLevels(newSLow, newSHigh)
                    }
                    break
                }
                case "decreaseHighLevel": {
                    let newHigh = Math.max(0, waterViewFirst.getHighEchogramLevel() - p)
                    let newLow = waterViewFirst.getLowEchogramLevel()
                    if (newHigh < newLow) newLow = newHigh
                    waterViewFirst.plotEchogramSetLevels(newLow, newHigh)
                    waterViewFirst.setLevels(newLow, newHigh)
                    if (waterViewSecond.enabled) {
                        let newSHigh = Math.max(0, waterViewSecond.getHighEchogramLevel() - p)
                        let newSLow = waterViewSecond.getLowEchogramLevel()
                        if (newSHigh < newSLow) newSLow = newSHigh
                        waterViewSecond.plotEchogramSetLevels(newSLow, newSHigh)
                        waterViewSecond.setLevels(newSLow, newSHigh)
                    }
                    break
                }
                case "prevTheme": {
                    let themeId = waterViewFirst.getThemeId()
                    if (themeId > 0) waterViewFirst.plotEchogramTheme(themeId - 1)
                    if (waterViewSecond.enabled) {
                        let themeSId = waterViewSecond.getThemeId()
                        if (themeSId > 0) waterViewSecond.plotEchogramTheme(themeSId - 1)
                    }
                    break
                }
                case "nextTheme": {
                    let themeId = waterViewFirst.getThemeId()
                    if (themeId < 9) waterViewFirst.plotEchogramTheme(themeId + 1)
                    if (waterViewSecond.enabled) {
                        let themeSId = waterViewSecond.getThemeId()
                        if (themeSId < 9) waterViewSecond.plotEchogramTheme(themeSId + 1)
                    }
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

            Keys.onPressed: function(event) {
                visualisationLayout.lastKeyPressed = event.key;
            }

            Keys.onReleased: {
                visualisationLayout.lastKeyPressed = Qt.Key_unknown;
            }

            GraphicsScene3dView {
                id:                renderer
                visible: (menuBar !== null) ? menuBar.is3DVisible : false
                objectName: "GraphicsScene3dView"
                Layout.fillHeight: true
                Layout.fillWidth:  true
                focus:             true

                property bool longPressTriggered: false
                property int currentZoom: -1

                onSendDataZoom: function(zoom) {
                    currentZoom = zoom;
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
                        Keys.onDeletePressed: function(event) { renderer.keyPressTrigger(event.key) }
                        Keys.onReturnPressed: function(event) { renderer.keyPressTrigger(event.key) }
                        Keys.onEnterPressed:  function(event) { renderer.keyPressTrigger(event.key) }
                        Keys.onEscapePressed: function(event) {
                            if (renderer.geoJsonEnabled) {
                                renderer.geojsonCancelDrawing()
                            } else {
                                renderer.clearRuler()
                            }
                        }

                        property int lastMouseKeyPressed: Qt.NoButton // TODO: maybe this mouseArea should be outside pinchArea
                        property point startMousePos: Qt.point(-1, -1)
                        property bool wasMoved: false
                        property real mouseThreshold: 15
                        property bool vertexMode: false

                        onEntered: {
                            mousearea3D.forceActiveFocus();
                        }

                        onWheel: function(wheel) {
                            renderer.mouseWheelTrigger(wheel.buttons, wheel.x, wheel.y, wheel.angleDelta, visualisationLayout.lastKeyPressed)
                        }

                        onPositionChanged: function(mouse) {
                            if (Qt.platform.os === "android") {
                                if (!wasMoved) {
                                    var delta = Math.sqrt(Math.pow((mouse.x - startMousePos.x), 2) + Math.pow((mouse.y - startMousePos.y), 2));
                                    if (delta > mouseThreshold) {
                                        wasMoved = true;
                                    }
                                }
                                if (renderer.longPressTriggered && !wasMoved) {
                                    if (renderer.geoJsonEnabled || renderer.rulerEnabled || renderer.rulerHasGeometry) {
                                        vertexMode = true
                                    } else {
                                        if (!vertexMode) {
                                            renderer.switchToBottomTrackVertexComboSelectionMode(mouse.x, mouse.y)
                                        }
                                        vertexMode = true
                                    }
                                }
                            }

                            renderer.mouseMoveTrigger(mouse.buttons, mouse.x, mouse.y, visualisationLayout.lastKeyPressed)
                        }

                        onPressed: function(mouse) {
                            menuBlock.visible = false
                            geoMenuBlock.visible = false
                            rulerMenuBlock.visible = false
                            startMousePos = Qt.point(mouse.x, mouse.y)
                            wasMoved = false
                            vertexMode = false
                            longPressTimer.start()
                            renderer.longPressTriggered = false

                            lastMouseKeyPressed = mouse.buttons
                            renderer.mousePressTrigger(mouse.buttons, mouse.x, mouse.y, visualisationLayout.lastKeyPressed)
                        }

                        onReleased: function(mouse) {
                            startMousePos = Qt.point(-1, -1)
                            wasMoved = false
                            longPressTimer.stop()

                            renderer.mouseReleaseTrigger(lastMouseKeyPressed, mouse.x, mouse.y, visualisationLayout.lastKeyPressed)

                            if (mouse.button === Qt.RightButton || (Qt.platform.os === "android" && vertexMode)) {
                                if (renderer.geoJsonEnabled) {
                                    geoMenuBlock.position(mouse.x, mouse.y)
                                } else if (renderer.rulerEnabled || renderer.rulerSelected) {
                                    rulerMenuBlock.position(mouse.x, mouse.y)
                                } else {
                                    menuBlock.position(mouse.x, mouse.y)
                                }
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
                    view: renderer
                    //anchors.horizontalCenter: parent.horizontalCenter
                    // anchors.rightMargin:      20
                    Keys.forwardTo:           [mousearea3D]
                }

                Scene3DRightToolbar {
                    id: scene3DRightToolbar
                    anchors.right: renderer.right
                    anchors.top: renderer.top
                    anchors.bottom: renderer.bottom
                    geo: renderer.geoJsonController
                    view: renderer
                    z: 3
                }

                Rectangle {
                    id: mosaicQualityBadge
                    visible: renderer.cameraPerspective
                             && (dataset.spatialPreparing
                                 || (scene3DToolbar.showMosaicQualityLabel
                                     && renderer.currentZoom > 0
                                     && (scene3DToolbar.mosaicEnabled || renderer.updateSurface)))
                    readonly property int tileSidePx: 256
                    readonly property int heightMatrixRatio: 8
                    readonly property int mosaicCmPerPix: renderer.currentZoom > 0
                                                           ? Math.pow(2, renderer.currentZoom - 1)
                                                           : 0
                    readonly property int surfaceCmPerCell: mosaicCmPerPix > 0
                                                             ? Math.round(mosaicCmPerPix * tileSidePx / heightMatrixRatio)
                                                             : 0
                    color: "#00000080"
                    radius: 4
                    anchors.left: scene3DToolbar.right
                    anchors.verticalCenter: scene3DToolbar.verticalCenter
                    anchors.leftMargin: 8
                    z: 1000
                    implicitWidth: mosaicQualityText.implicitWidth + 12
                    implicitHeight: mosaicQualityText.implicitHeight + 8
                    opacity: 1.0

                    SequentialAnimation {
                        id: mosaicQualityPreparingAnimation
                        running: dataset.spatialPreparing
                        loops: Animation.Infinite
                        NumberAnimation { target: mosaicQualityBadge; property: "opacity"; to: 0.35; duration: 500 }
                        NumberAnimation { target: mosaicQualityBadge; property: "opacity"; to: 1.0; duration: 500 }
                    }

                    onVisibleChanged: {
                        if (!visible) {
                            opacity = 1.0
                        }
                    }

                    Connections {
                        target: dataset
                        function onSpatialPreparingChanged() {
                            if (!dataset.spatialPreparing) {
                                mosaicQualityBadge.opacity = 1.0
                            }
                        }
                    }

                    Text {
                        id: mosaicQualityText
                        text: {
                            if (dataset.spatialPreparing) {
                                return qsTr("Data prepairing...")
                            }
                            var parts = [];
                            if (renderer.currentZoom > 0 && scene3DToolbar.mosaicEnabled) {
                                parts.push(qsTr("Mosaic: ") + mosaicQualityBadge.mosaicCmPerPix + qsTr(" cm/pix"));
                            }
                            if (renderer.currentZoom > 0 && renderer.updateSurface) {
                                parts.push(qsTr("Surface: ") + mosaicQualityBadge.surfaceCmPerCell + qsTr(" cm/cell"));
                            }
                            return parts.join("\n");
                        }
                        color: "#ffffff"
                        font: theme.textFont
                        anchors.centerIn: parent
                    }
                }                CContact {
                    id: contactDialog
                    visible: false
                    offsetOpacityArea: 20 // increase in 3D

                    onInputAccepted: {
                        contacts.setContact(contactDialog.indx, contactDialog.inputFieldText)
                    }
                    onSetActiveButtonClicked: {
                        contacts.setActiveContact(contactDialog.indx)
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

                RowLayout {
                    id: geoMenuBlock
                    Layout.alignment: Qt.AlignHCenter
                    spacing: 1
                    visible: false
                    Layout.margins: 0

                    property var geo: renderer.geoJsonController

                    onGeoChanged: {
                        //console.log("GeoJson menu updated, drawing: " + geo.drawing + ", selectedFeatureId: " + geo.selectedFeatureId)
                    }

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

                    CheckButton {
                        icon.source: "qrc:/icons/ui/plus.svg"
                        backColor: theme.controlBackColor
                        checkable: false
                        implicitWidth: theme.controlHeight
                        visible: geoMenuBlock.geo && geoMenuBlock.geo.drawing

                        onClicked: {
                            renderer.geojsonFinishDrawing()
                            geoMenuBlock.visible = false
                        }
                    }

                    CheckButton {
                        icon.source: "qrc:/icons/ui/stack_backward.svg"
                        backColor: theme.controlBackColor
                        checkable: false
                        implicitWidth: theme.controlHeight
                        visible: geoMenuBlock.geo && geoMenuBlock.geo.drawing

                        onClicked: {
                            renderer.geojsonUndoLastVertex()
                            geoMenuBlock.visible = false
                        }
                    }

                    CheckButton {
                        icon.source: "qrc:/icons/ui/x.svg"
                        backColor: theme.controlBackColor
                        checkable: false
                        implicitWidth: theme.controlHeight
                        visible: geoMenuBlock.geo && geoMenuBlock.geo.drawing

                        onClicked: {
                            renderer.geojsonCancelDrawing()
                            geoMenuBlock.visible = false
                        }
                    }

                    CheckButton {
                        icon.source: "qrc:/icons/ui/timeline_event_x.svg"
                        backColor: theme.controlBackColor
                        checkable: false
                        implicitWidth: theme.controlHeight
                        visible: geoMenuBlock.geo && !geoMenuBlock.geo.drawing && geoMenuBlock.geo.selectedFeatureId !== ""

                        onClicked: {
                            renderer.geojsonDeleteSelectedFeature()
                            geoMenuBlock.visible = false
                        }
                    }
                }

                RowLayout {
                    id: rulerMenuBlock
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

                    CheckButton {
                        icon.source: "qrc:/icons/ui/file-check.svg"
                        backColor: theme.controlBackColor
                        checkable: false
                        implicitWidth: theme.controlHeight
                        visible: renderer.rulerEnabled && renderer.rulerDrawing

                        onClicked: {
                            renderer.rulerFinishDrawing()
                            rulerMenuBlock.visible = false
                        }
                    }

                    CheckButton {
                        icon.source: "qrc:/icons/ui/x.svg"
                        backColor: theme.controlBackColor
                        checkable: false
                        implicitWidth: theme.controlHeight
                        visible: renderer.rulerEnabled || renderer.rulerSelected

                        onClicked: {
                            if (renderer.rulerDrawing) {
                                renderer.rulerCancelDrawing()
                            }
                            rulerMenuBlock.visible = false
                        }
                    }

                    CheckButton {
                        icon.source: "qrc:/icons/ui/timeline_event_x.svg"
                        backColor: theme.controlBackColor
                        checkable: false
                        implicitWidth: theme.controlHeight
                        visible: !renderer.rulerDrawing && renderer.rulerSelected

                        onClicked: {
                            renderer.rulerDeleteSelected()
                            rulerMenuBlock.visible = false
                        }
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
                        id: waterViewFirst
                        Layout.fillHeight: true
                        Layout.fillWidth: true

                        Layout.rowSpan   : 1
                        Layout.columnSpan: 1
                        focus: true
                        instruments: menuBar.instruments
                        indx: 1
                        is3dVisible: menuBar.is3DVisible

                        onTimelinePositionChanged: {
                            historyScroll.value = waterViewFirst.timelinePosition
                        }

                        Component.onCompleted: {
                            waterViewFirst.setIndx(waterViewFirst.indx);
                        }
                    }

                    Plot2D {
                        id: waterViewSecond

                        enabled: menuBar.numPlots === 2
                        visible: menuBar.numPlots === 2

                        Layout.fillHeight: true
                        Layout.fillWidth: true

                        Layout.rowSpan   : 1
                        Layout.columnSpan: 1
                        focus: true
                        instruments: menuBar.instruments
                        indx: 2

                        onEnabledChanged: {
                            waterViewSecond.setPlotEnabled(enabled)
                        }

                        onVisibleChanged: {
                            if (visible && menuBar.syncPlots) {
                                setCursorFromTo(waterViewFirst.cursorFrom(), waterViewFirst.cursorTo())
                                update()
                            }
                        }

                        onTimelinePositionChanged: {
                            historyScroll.value = timelinePosition
                        }

                        Component.onCompleted: {
                            setIndx(waterViewSecond.indx);
                        }
                    }

                    CSlider {
                        id: historyScroll
                        Layout.margins: 0
                        Layout.fillWidth: true
                        Layout.fillHeight: false
                        Layout.columnSpan: parent.columns
                        implicitHeight: theme.controlHeight
                        height: theme.controlHeight
                        //value: waterViewFirst.timelinePosition
                        stepSize: 0.0001
                        from: 0
                        to: 1
                        barWidth: 50 * theme.resCoeff
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

    Item {
        id: profilesFloatBtn
        z: 9999
        visible: menuBar.profilesBtnVis

        property int  margin: 12
        property real idleOpacity: 0.45
        property real buttonWidth: theme.controlHeight * 4
        property real buttonHeight: theme.controlHeight

        opacity: idleOpacity
        width: profilesContainer.implicitWidth
        height: profilesContainer.implicitHeight

        function clampToWindow() {
            x = Math.max(margin, Math.min(x, mainview.width  - width  - margin))
            y = Math.max(margin, Math.min(y, mainview.height - height - margin))
        }

        Component.onCompleted: {
            x = mainview.width - width - margin
            y = margin
            clampToWindow()
        }

        Connections {
            target: mainview
            function onWidthChanged()  { profilesFloatBtn.clampToWindow() }
            function onHeightChanged() { profilesFloatBtn.clampToWindow() }
        }

        Behavior on opacity { NumberAnimation { duration: 120 } }

        Column {
            id: profilesContainer
            spacing: 6
            anchors.horizontalCenter: parent.horizontalCenter

            CheckButton {
                id: profilesBtn
                width: profilesFloatBtn.buttonWidth
                height: profilesFloatBtn.buttonHeight
                text: qsTr("Profiles...")
                backColor: theme.controlBackColor
                borderColor: "transparent"
                onClicked: profilesDialog.open()
            }

            Repeater {
                id: quickButtonsRepeater
                model: profilesModel

                delegate: CButton {
                    text: (index + 1).toString()
                    enabled: path && path.length > 0
                    width: profilesBtn.width
                    height: profilesBtn.height
                    onClicked: {
                        if (path && path.length > 0) {
                            menuBar.applyProfileToAllDevices(path)
                        }
                    }
                }
            }
        }

        DragHandler {
            id: profilesDrag
            target: profilesFloatBtn
            xAxis.minimum: profilesFloatBtn.margin
            xAxis.maximum: Math.max(profilesFloatBtn.margin, mainview.width - profilesFloatBtn.width - profilesFloatBtn.margin)
            yAxis.minimum: profilesFloatBtn.margin
            yAxis.maximum: Math.max(profilesFloatBtn.margin, mainview.height - profilesFloatBtn.height - profilesFloatBtn.margin)
            onActiveChanged: {
                if (!active) {
                    profilesFloatBtn.clampToWindow()
                }
            }
        }

        HoverHandler {
            id: profilesHover
            target: profilesContainer
            acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
            onHoveredChanged: {
                if (hovered) {
                    profilesFloatBtn.opacity = 1.0
                }
                else {
                    profilesFloatBtn.opacity = profilesFloatBtn.idleOpacity
                }
            }
        }
    }

    Dialog {
        id: profilesDialog
        title: qsTr("Profiles")
        modal: true
        focus: true
        width: Math.min(parent ? parent.width * 0.9 : 700, 700)
        standardButtons: Dialog.Close

        property int browseRow: -1

        Settings {
            id: profilesStorage
            property var savedProfiles: []
        }

        function loadSavedProfiles() {
            profilesModel.clear()
            var stored = profilesStorage.savedProfiles
            if (!stored || stored.length === 0) {
                return
            }
            for (var i = 0; i < stored.length; ++i) {
                profilesModel.append({ path: stored[i] })
            }
        }

        function saveProfiles() {
            var stored = []
            for (var i = 0; i < profilesModel.count; ++i) {
                var data = profilesModel.get(i)
                stored.push(data.path ? data.path : "")
            }
            profilesStorage.savedProfiles = stored
        }

        Component.onCompleted: {
            loadSavedProfiles()
            standardButton(Dialog.Close).text = qsTr("Close")
        }

        function urlToPath(u) {
            if (!u) return ""
            if (u.toLocalFile) return u.toLocalFile()
            var s = u.toString()
            if (s.startsWith("file:///")) s = s.slice(8)
            else if (s.startsWith("file://")) s = s.slice(7)
            return s
        }

        ListModel {
            id: profilesModel
            //profilesModel.append({ path: "" })
        }

        FileDialog {
            id: profilePickDialog
            title: qsTr("Select profile XML")
            fileMode: FileDialog.OpenFile
            nameFilters: Qt.platform.os === "android" ? ["*/*"] : ["XML files (*.xml)"]

            onAccepted: {
                if (profilesDialog.browseRow < 0) return
                const p = profilesDialog.urlToPath(profilePickDialog.selectedFile)
                profilesModel.setProperty(profilesDialog.browseRow, "path", p)
                profilesDialog.browseRow = -1
                profilesDialog.saveProfiles()
            }
        }

        contentItem: ColumnLayout {
            spacing: 10

            RowLayout {
                Layout.fillWidth: true

                Label {
                    text: qsTr("Add profiles and apply them")
                    Layout.fillWidth: true
                    color: "white"
                }

                CButton {
                    text: "+"
                    onClicked: {
                        profilesModel.append({ path: "" })
                        profilesDialog.saveProfiles()
                    }
                }
            }

            ScrollView {
                Layout.fillWidth: true
                Layout.preferredHeight: 320
                clip: true

                ListView {
                    id: profilesList
                    model: profilesModel
                    spacing: 8

                    delegate: Rectangle {
                        width: ListView.view.width
                        height: 52
                        radius: 8
                        color: "#202020"
                        border.color: "#909090"

                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 8
                            spacing: 8

                            CTextField {
                                id: pathField
                                Layout.fillWidth: true
                                placeholderText: qsTr("Path to profile .xml")
                                text: path
                                color: "white"
                                onEditingFinished: {
                                    profilesModel.setProperty(index, "path", text)
                                    profilesDialog.saveProfiles()
                                }
                            }

                            CButton {
                                text: qsTr("Browse")
                                onClicked: {
                                    profilesDialog.browseRow = index
                                    profilePickDialog.open()
                                }
                            }

                            CButton {
                                text: qsTr("Apply")
                                enabled: (pathField.text && pathField.text.length > 0)
                                onClicked: {
                                    menuBar.applyProfileToAllDevices(pathField.text)
                                }
                            }

                            CButton {
                                text: "✕"
                                onClicked: {
                                    profilesModel.remove(index)
                                    profilesDialog.saveProfiles()
                                }
                            }
                        }
                    }
                }
            }
        }

        background: Rectangle {
            color: theme.controlBackColor
            radius: 8
        }
    }

    MenuFrame {
        id: extraInfoPanel
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.margins: 12
        visible: menuBar.extraInfoVis && !showBanner && dataset.isBoatCoordinateValid
        isDraggable: true
        isOpacityControlled: true
        horizontalMargins: 12
        verticalMargins: 10
        spacing: 8

        function lpad(s, w, ch) {
            s = String(s)
            while (s.length < w) s = (ch || ' ') + s
            return s
        }
        function formatFixed(value, fracDigits, intWidth) {
            if (!isFinite(value)) return lpad("-", intWidth + 1 + fracDigits)
            var sign = value < 0 ? "-" : " "
            var abs  = Math.abs(value)
            var s    = abs.toFixed(fracDigits)
            var p    = s.split(".")
            var intP = lpad(p[0], intWidth, " ")
            return sign + intP + (fracDigits > 0 ? "." + p[1] : "")
        }
        function toDMS(value, isLat) {
            var hemi = isLat ? (value >= 0 ? "N" : "S") : (value >= 0 ? "E" : "W");
            var abs  = Math.abs(value)
            var s    = abs.toFixed(4)
            var p    = s.split(".")
            var intP = lpad(p[0], 3, " ")
            return hemi + " " + intP + "." + p[1]
        }

        property string latDms: ""
        property string lonDms: ""
        property string distStr: ""
        property string angStr: ""
        property string depthStr: ""
        property string speedStr: ""

        function updateFields() {
            latDms   = toDMS(dataset.boatLatitude,  true)  + qsTr("°")
            lonDms   = toDMS(dataset.boatLongitude, false) + qsTr("°")
            distStr  = formatFixed(dataset.distToContact, 1, 3) + qsTr(" m")
            angStr   = formatFixed(dataset.angleToContact, 1, 3) + qsTr("°")
            depthStr = formatFixed(dataset.depth, 1, 3) + qsTr(" m")
            speedStr = formatFixed(dataset.speed, 1, 3) + qsTr(" km/h")
        }

        Timer {
            interval: 333
            repeat: true
            running: extraInfoPanel.visible
            triggeredOnStart: true
            onTriggered: extraInfoPanel.updateFields()
        }

        ColumnLayout {
            spacing: 6

            ColumnLayout {

                CText {
                    visible: dataset.isLastDepthValid
                    text: extraInfoPanel.depthStr
                    font.bold: true
                    font.pixelSize: 40 * theme.resCoeff
                    font.family: "monospace"
                    leftPadding: 4
                }

                CText {
                    visible: dataset.isValidSpeed
                    text: extraInfoPanel.speedStr
                    font.bold: true
                    font.pixelSize: 40 * theme.resCoeff
                    font.family: "monospace"
                    leftPadding: 4
                }
            }

            ColumnLayout {
                visible: dataset.isBoatCoordinateValid

                CText {
                    text: qsTr("Boat position")
                    leftPadding: 4
                    rightPadding: 4
                    font.bold: true
                    font.pixelSize: 16 * theme.resCoeff
                }

                RowLayout {
                    spacing: 6
                    CText { text: qsTr("Lat.:"); opacity: 0.7; leftPadding: 4; }
                    Item  { Layout.fillWidth: true }
                    CText { text: extraInfoPanel.latDms; }
                }

                RowLayout {
                    spacing: 6
                    CText { text: qsTr("Lon.:"); opacity: 0.7; leftPadding: 4; }
                    Item  { Layout.fillWidth: true }
                    CText { text: extraInfoPanel.lonDms; }
                }
            }

            ColumnLayout {
                visible: dataset.isActiveContactIndxValid

                CText {
                    text: qsTr("Active point")
                    leftPadding: 4
                    rightPadding: 4
                    font.bold: true
                    font.pixelSize: 16 * theme.resCoeff
                }

                RowLayout {
                    spacing: 6
                    CText { text: qsTr("Dist.:"); opacity: 0.7; leftPadding: 4 }
                    Item  { Layout.fillWidth: true }
                    CText { text: extraInfoPanel.distStr; }
                }

                RowLayout {
                    spacing: 6
                    CText { text: qsTr("Ang.:"); opacity: 0.7; leftPadding: 4 }
                    Item  { Layout.fillWidth: true }
                    CText { text: extraInfoPanel.angStr; }
                }
            }
        }
    }

    // бровь
    MenuFrame {
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        visible: !showBanner && (deviceManagerWrapper.pilotArmState >= 0) && menuBar.autopilotInfofVis
        isDraggable: true
        isOpacityControlled: true
        Keys.forwardTo: [splitLayer]

        ColumnLayout {
            RowLayout {
                Layout.alignment: Qt.AlignHCenter
                CheckButton {
                    // text: checked ? "Armed" : "Disarmed"
                    icon.source: checked ? "qrc:/icons/ui/propeller.svg" : "qrc:/icons/ui/propeller_off.svg"
                    checked: deviceManagerWrapper.pilotArmState === 1
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
                    checked: deviceManagerWrapper.pilotModeState === 0 // "Manual"
                    onCheckedChanged: {
                    }
                    ButtonGroup.group: autopilotModeGroup
                    implicitWidth: theme.controlHeight
                }

                CheckButton {
                    // Layout.fillWidth: true
                    icon.source: "qrc:/icons/ui/route.svg"
                    checked: deviceManagerWrapper.pilotModeState === 10 // "Auto"
                    onCheckedChanged: {
                    }
                    ButtonGroup.group: autopilotModeGroup
                    implicitWidth: theme.controlHeight
                }

                CheckButton {
                    // Layout.fillWidth: true
                    icon.source: "qrc:/icons/ui/anchor.svg"
                    checked: deviceManagerWrapper.pilotModeState === 5 // "Loiter"
                    onCheckedChanged: {
                    }
                    ButtonGroup.group: autopilotModeGroup
                    implicitWidth: theme.controlHeight
                }

                CheckButton {
                    // Layout.fillWidth: true
                    icon.source: "qrc:/icons/ui/map_pin.svg"
                    checked: deviceManagerWrapper.pilotModeState === 15 // "Guided"
                    onCheckedChanged: {
                    }
                    ButtonGroup.group: autopilotModeGroup
                    implicitWidth: theme.controlHeight
                }

                CheckButton {
                    // Layout.fillWidth: true
                    icon.source: "qrc:/icons/ui/home.svg"
                    checked: deviceManagerWrapper.pilotModeState === 11 || deviceManagerWrapper.pilotModeState === 12  // "RTL" || "SmartRTL"
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

    MainMenuBar {
        id:                menuBar
        objectName:        "menuBar"
        Layout.fillHeight: true
        Keys.forwardTo:    [splitLayer, mousearea3D]
        height: visualisationLayout.height
        Component.onCompleted: {
            menuBar.targetPlot = waterViewFirst
        }
        visible: !showBanner
    }

    function handleChildSignal(langStr) {
        mainview.showBanner = true
        selectedLanguageStr = langStr
    }

    function handleSyncPlotEnabled() {
        waterViewSecond.setCursorFromTo(waterViewFirst.cursorFrom(), waterViewFirst.cursorTo())
        waterViewSecond.update()
    }

    function handlePlotCursorChanged(indx, from, to) {
        if (!menuBar.syncPlots) {
            return;
        }

        if (indx === 1 && waterViewSecond.enabled) {
            waterViewSecond.setCursorFromTo(from, to)
            waterViewSecond.update()
        }
        if (indx === 2) {
            waterViewFirst.setCursorFromTo(from, to)
            waterViewFirst.update()
        }
    }

    function handleUpdateOtherPlot(indx) {
        if (indx === 1 && waterViewSecond.enabled) {
            waterViewSecond.update()
        }
        if (indx === 2) {
            waterViewFirst.update()
        }
    }
    function handlePlotPressed(indx, mouseX, mouseY) {
        let r = core.getConvertedMousePos(indx, mouseX, mouseY)

        if (indx === 1 && waterViewSecond.enabled) {
            waterViewSecond.setAim(r.x, r.y)
        }
        if (indx === 2) {
            waterViewFirst.setAim(r.x, r.y)
        }
    }
    function handlePlotReleased(indx) {
        if (indx === 1 && waterViewSecond.enabled) {
            waterViewSecond.resetAim()
        }
        if (indx === 2) {
            waterViewFirst.resetAim()
        }
    }
    function onPlotSettingsClicked() {
        menuBar.closeMenus()
    }
    function onMenuBarSettingsOpened() {
        waterViewFirst.closeSettings()
        waterViewSecond.closeSettings()
    }
    function handleMosaicLOffsetChanged(val) {
        waterViewFirst.mosaicLOffsetChanged(val)
        waterViewSecond.mosaicLOffsetChanged(val)
    }
    function handleMosaicROffsetChanged(val) {
        waterViewFirst.mosaicROffsetChanged(val)
        waterViewSecond.mosaicROffsetChanged(val)
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
