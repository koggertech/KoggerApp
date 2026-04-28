import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Dialogs
import QtCore

import WaterFall 1.0

WaterFall {
    id: plot

    property bool is3dVisible: false
    property int indx: 0
    property int instruments: instrumentsGradeList.currentIndex
    property bool settingsOpen: plotCheckButton.checked
    property bool hasTransientUi: menuBlock.visible || contactDialog.visible
    property bool loupeZoomAdjusting: false
    property bool loupeZoomWasVisibleBeforeAdjust: false
    property int loupeZoomSavedAimEpoch: -1
    property real settingsMenuSpacer: Math.max(4, Math.round(theme.controlHeight * 0.2))

    horizontal: horisontalVertical.checked

    function setLevels(low, high) {
        echogramLevelsSlider.startValue = low
        echogramLevelsSlider.stopValue = high
        echogramLevelsSlider.startPointY = echogramLevelsSlider.valueToPosition(low);
        echogramLevelsSlider.stopPointY = echogramLevelsSlider.valueToPosition(high);
        echogramLevelsSlider.update()
    }

    function updateBottomTrackPresentation() {
        const showValue = bottomTrackValueVisible.checked
        const showLine = bottomTrackGraphicsVisible.checked

        plotBottomTrackDepthTextVisible(showValue)
        plotBottomTrackTheme(showLine ? (bottomTrackThemeList.currentIndex + 1) : 0)
        plotBottomTrackVisible(showValue || showLine)
    }

    function updateRangefinderPresentation() {
        const showValue = rangefinderValueVisible.checked
        const showLine = rangefinderGraphicsVisible.checked

        plotRangefinderDepthTextVisible(showValue)
        plotRangefinderTheme(showLine ? (rangefinderThemeList.currentIndex + 1) : 0)
        plotRangefinderVisible(showValue || showLine)
    }

    function beginLoupeZoomPreview() {
        if (loupeZoomAdjusting) {
            return
        }

        loupeZoomAdjusting = true
        loupeZoomWasVisibleBeforeAdjust = loupeVisible.checked
        loupeZoomSavedAimEpoch = getAimEpochIndex()

        if (!loupeVisible.checked) {
            loupeVisible.checked = true
        }

        const previewEpoch = getPreferredLoupeEpochIndex(loupeZoomSavedAimEpoch)
        setAimEpochIndex(previewEpoch)
    }

    function updateLoupeZoomPreview() {
        if (!loupeZoomAdjusting) {
            return
        }

        const previewEpoch = getPreferredLoupeEpochIndex(getAimEpochIndex())
        setAimEpochIndex(previewEpoch)
    }

    function endLoupeZoomPreview() {
        if (!loupeZoomAdjusting) {
            return
        }

        loupeZoomAdjusting = false
        if (loupeZoomSavedAimEpoch >= 0) {
            setAimEpochIndex(loupeZoomSavedAimEpoch)
        }
        else {
            setAimEpochIndex(-1)
            resetAim()
        }

        if (!loupeZoomWasVisibleBeforeAdjust) {
            loupeVisible.checked = false
        }

        loupeZoomSavedAimEpoch = -1
    }

    function closeSettings() {
        if (!plotCheckButton.checked) {
            return false
        }
        plotCheckButton.checked = false
        return true
    }

    function toggleEchogramType() {
        if (echogramTypesList.count <= 0) {
            return
        }

        echogramTypesList.currentIndex = (echogramTypesList.currentIndex + 1) % echogramTypesList.count
    }

    function closeTransientUi() {
        let handled = false

        if (menuBlock.visible) {
            menuBlock.visible = false
            handled = true
        }

        if (contactDialog.visible) {
            contactDialog.visible = false
            handled = true
        }

        return handled
    }

    function setAim(mouseX, mouseY) {
        plotMousePosition(mouseX, mouseY, true)
    }
    function resetAim() {
        plotMousePosition(-1, -1)
    }
    function doVerZoomEvent(paramX) {
        verZoomEvent(paramX)
    }
    function doVerScrollEvent(paramX) {
        verScrollEvent(paramX)
    }

    onEnabledChanged: {
        if (enabled) {
            update();
        }
    }

    signal plotCursorChanged(int indx, real from, real to)
    signal updateOtherPlot(int indx)
    signal plotPressed(int indx, int mousex, int mousey)
    signal plotReleased(int indx)
    signal settingsClicked()
    signal echogramThemeChanged(int themeId)

    PinchArea {
        id: pinch2D
        anchors.fill: parent
        enabled: true

        property int thresholdXAxis: 15
        property int thresholdYAxis: 15
        property double zoomThreshold: 0.1

        property bool movementX: false
        property bool movementY: false
        property bool zoomY: false
        property point pinchStartPos: Qt.point(-1, -1)

        function clearPinchMovementState() {
            movementX = false
            movementY = false
            zoomY = false
        }

        onPinchStarted: {
            menuBlock.visible = false

            mousearea.enabled = false
            plot.plotMousePosition(-1, -1)

            clearPinchMovementState()
            pinchStartPos = Qt.point(pinch.center.x, pinch.center.y)
        }

        onPinchUpdated: {
            console.info("onPinchUpdated")

            if (movementX) {
                let val = -(pinch.previousCenter.x - pinch.center.x)
                plot.horScrollEvent(val)
                updateOtherPlot(indx)
            }
            else if (movementY) {
                let val = pinch.previousCenter.y - pinch.center.y
                plot.verScrollEvent(val)
                plotCursorChanged(indx, cursorFrom(), cursorTo())
            }
            else if (zoomY) {
                let val = (pinch.previousScale - pinch.scale) * 500.0
                plot.verZoomEvent(val)
                plotCursorChanged(indx, cursorFrom(), cursorTo())
            }
            else {
                if (Math.abs(pinchStartPos.x - pinch.center.x) > thresholdXAxis) {
                    movementX = true
                }
                else if (Math.abs(pinchStartPos.y - pinch.center.y) > thresholdYAxis) {
                    movementY = true
                }
                else if (pinch.scale > (1.0 + zoomThreshold) || pinch.scale < (1.0 - zoomThreshold)) {
                    zoomY = true
                }
            }
        }       

        onPinchFinished: {
            mousearea.enabled = true
            plot.plotMousePosition(-1, -1)

            clearPinchMovementState()
            pinchStartPos = Qt.point(-1, -1)
        }

        MouseArea {
            id: mousearea
            enabled: true
            anchors.fill: parent
            acceptedButtons: Qt.LeftButton | Qt.RightButton

            property int lastMouseX: -1
            property bool wasMoved: false
            property point startMousePos: Qt.point(-1, -1)
            property real mouseThreshold: 15
            property int contactMouseX: -1
            property int contactMouseY: -1

            hoverEnabled: true

            Timer {
                id: longPressTimer
                interval: 500
                repeat: false
                onTriggered: {
                    if (Qt.platform.os === "android" && theme.instrumentsGrade !== 0 && !mousearea.wasMoved) {
                        plot.onCursorMoved(mousearea.mouseX, mousearea.mouseY)
                        mousearea.contactMouseX = mousearea.mouseX
                        mousearea.contactMouseY = mousearea.mouseY
                        plot.simplePlotMousePosition(mousearea.mouseX, mousearea.mouseY)

                        menuBlock.position(mousearea.mouseX, mousearea.mouseY)
                    }
                }
            }

            onClicked: function(mouse) {
                lastMouseX = mouse.x
                plot.focus = true

                if (mouse.button === Qt.RightButton) {
                    contactMouseX = mouse.x
                    contactMouseY = mouse.y

                    plot.simplePlotMousePosition(mouse.x, mouse.y)

                    if (theme.instrumentsGrade !== 0) {
                        menuBlock.position(mouse.x, mouse.y)
                    }
                }

                wasMoved = false
            }

            onPressed: function(mouse) {
                lastMouseX = mouse.x

                if (Qt.platform.os === "android") {
                    startMousePos = Qt.point(mouse.x, mouse.y)
                    longPressTimer.start()
                }

                if (mouse.button === Qt.LeftButton) {
                    menuBlock.visible = false
                    plot.plotMousePosition(mouse.x, mouse.y)
                    plotPressed(indx, mouse.x, mouse.y)
                }

                if (mouse.button === Qt.RightButton) {
                    contactMouseX = mouse.x
                    contactMouseY = mouse.y

                    plot.simplePlotMousePosition(mouse.x, mouse.y)
                }

                wasMoved = false
            }

            onReleased: function(mouse) {
                lastMouseX = -1

                if (Qt.platform.os === "android") {
                    longPressTimer.stop()
                }

                if (mouse.button === Qt.LeftButton) {
                    plot.plotMousePosition(-1, -1)
                }

                if (mouse.button === Qt.RightButton) {
                    contactMouseX = mouse.x
                    contactMouseY = mouse.y

                    plot.simplePlotMousePosition(mouse.x, mouse.y)
                }

                wasMoved = false
                startMousePos = Qt.point(-1, -1)
                plotReleased(indx)
            }

            onCanceled: {
                lastMouseX = -1

                if (Qt.platform.os === "android") {
                    longPressTimer.stop()
                }

                wasMoved = false
                startMousePos = Qt.point(-1, -1)
                plotReleased(indx)
            }

            onPositionChanged: function(mouse) {
                plot.onCursorMoved(mouse.x, mouse.y)

                if (Qt.platform.os === "android") {
                    if (!wasMoved) {
                        var currDelta = Math.sqrt(Math.pow((mouse.x - startMousePos.x), 2) + Math.pow((mouse.y - startMousePos.y), 2));
                        if (currDelta > mouseThreshold) {
                            wasMoved = true;
                        }
                    }
                }

                var delta = mouse.x - lastMouseX
                lastMouseX = mouse.x

                if (mousearea.pressedButtons & Qt.LeftButton) {
                    plot.plotMousePosition(mouse.x, mouse.y)
                    plotPressed(indx, mouse.x, mouse.y)
                }

                if (mouse.button === Qt.RightButton) {
                    contactMouseX = mouse.x
                    contactMouseY = mouse.y

                    plot.simplePlotMousePosition(mouse.x, mouse.y)
                }
            }

            onWheel: function(wheel) {
                if (wheel.modifiers & Qt.ControlModifier) {
                    let val = -wheel.angleDelta.y
                    plot.verZoomEvent(val)
                    plotCursorChanged(indx, cursorFrom(), cursorTo())
                }
                else if (wheel.modifiers & Qt.ShiftModifier) {
                    let val = -wheel.angleDelta.y
                    plot.verScrollEvent(val)
                    plotCursorChanged(indx, cursorFrom(), cursorTo())
                }
                else {
                    let val = wheel.angleDelta.y
                    plot.horScrollEvent(val)
                    updateOtherPlot(indx)
                }
            }
        }
    }

    onHeightChanged: {
        if(menuBlock.visible) {
            menuBlock.position(menuBlock.x, menuBlock.y)
        }
    }

    onWidthChanged: {
        if(menuBlock.visible) {
            menuBlock.position(menuBlock.x, menuBlock.y)
        }
    }

    RowLayout {
        id: settingsRow
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.bottomMargin: settingsMenuSpacer

        MenuFrame {
            id: leftPanel
            isOpacityControlled: true
            Layout.alignment: Qt.AlignLeft | Qt.AlignBottom
            Layout.leftMargin: (indx === 1 &&
                                !is3dVisible &&
                                height > plot.height - 130 * theme.resCoeff)
                               ? width
                               : 0

            ColumnLayout {
                id: plotControl
                spacing: 4

                CheckButton {
                    id: plotCheckButton
                    backColor: theme.controlBackColor
                    borderColor: theme.controlBackColor
                    checkedBorderColor: theme.controlBorderColor
                    iconSource: "qrc:/icons/ui/settings.svg"
                    implicitWidth: theme.controlHeight*1.2

                    onCheckedChanged: {
                        if (checked) {
                            settingsClicked()
                        }
                        else {
                            plot.endLoupeZoomPreview()
                        }
                    }
                }

                // brightess slider
                CText {
                    Layout.fillWidth: true
                    Layout.topMargin: 0
                    Layout.preferredWidth: theme.controlHeight*1.2
                    // visible: chartEnable.checked // TODO
                    horizontalAlignment: Text.AlignHCenter
                    text: echogramLevelsSlider.stopValue
                    small: true
                }

                ChartLevel {
                    // opacity: 0.8
                    Layout.fillWidth: true
                    Layout.preferredWidth: theme.controlHeight*1.2
                    id: echogramLevelsSlider
                    // visible: chartEnable.checked // TODO
                    Layout.alignment: Qt.AlignHCenter

                    onStartValueChanged: {
                        plot.plotEchogramSetLevels(startValue, stopValue);
                    }

                    onStopValueChanged: {
                        plot.plotEchogramSetLevels(startValue, stopValue);
                    }

                    Component.onCompleted: {
                        plot.plotEchogramSetLevels(startValue, stopValue);
                    }

                    Settings {
                        category: "Plot2D_" + plot.indx

                        property alias echogramLevelsStart: echogramLevelsSlider.startValue
                        property alias echogramLevelsStop: echogramLevelsSlider.stopValue
                    }
                }

                CText {
                    Layout.fillWidth: true
                    Layout.preferredWidth: theme.controlHeight*1.2
                    Layout.bottomMargin: 0
                    // visible: chartEnable.checked // TODO
                    horizontalAlignment: Text.AlignHCenter

                    text: echogramLevelsSlider.startValue
                    small: true
                }
            }
        }

        MenuScroll {
            id: settingsScroll
            visible: plotCheckButton.checked
            Layout.alignment: Qt.AlignBottom
            readonly property real maxHeightByPlot: Math.max(theme.controlHeight * 2, plot.height - settingsMenuSpacer * 2)
            readonly property real contentHeightWithPadding: plotSettings.implicitHeight + topPadding + bottomPadding
            Layout.preferredHeight: Math.min(maxHeightByPlot, contentHeightWithPadding)

            onVisibleChanged: {
                if (!visible) {
                    plot.endLoupeZoomPreview()
                }
            }

            MenuFrame {
                id: plotSettings

                ParamGroup {
                    groupName: qsTr("Plot")

                    RowLayout {
                        id: rowDataset
                        Layout.fillWidth: true
                        visible: instruments > 1
                        //CCombo  {
                        //    id: datasetCombo
                        //    Layout.fillWidth: true
                        //      Layout.preferredWidth: columnItem.width/3
                        //    visible: true
                        //    onPressedChanged: {
                        //    }

                        //    Component.onCompleted: {
                        //        model = [qsTr("Dataset #1")]
                        //    }
                        //}

                        CText {
                            text: qsTr("Channels:")
                        }

                        function setChannelNamesToBackend() {
                            plotDatasetChannelFromStrings(channel1Combo.currentText, channel2Combo.currentText)
                            plotCursorChanged(indx, cursorFrom(), cursorTo())
                        }

                        CCombo  {
                            id: channel1Combo

                            property bool suppressTextSignal: false

                            Layout.fillWidth: true
                            visible: true

                            onCurrentTextChanged: {
                                if (suppressTextSignal) {
                                    return
                                }

                                rowDataset.setChannelNamesToBackend()
                            }

                            Component.onCompleted: {
                                model = dataset.channelsNameList()

                                let index = model.indexOf(core.ch1Name)
                                if (index >= 0) {
                                    channel1Combo.currentIndex = index
                                }
                            }

                            Connections {
                                target: core
                                function onChannelListUpdated() {
                                    let list = dataset.channelsNameList()

                                    channel1Combo.suppressTextSignal = true

                                    channel1Combo.model = []
                                    channel1Combo.model = list

                                    let newIndex = list.indexOf(core.ch1Name)
                                    if (newIndex >= 0) {
                                        channel1Combo.currentIndex = newIndex
                                    }
                                    else {
                                        channel1Combo.currentIndex = 0
                                    }

                                    channel1Combo.suppressTextSignal = false
                                }
                            }
                        }

                        CCombo  {
                            id: channel2Combo

                            property bool suppressTextSignal: false

                            Layout.fillWidth: true
                            visible: true

                            onCurrentTextChanged: {
                                if (suppressTextSignal) {
                                    return
                                }

                                rowDataset.setChannelNamesToBackend()
                            }


                            Component.onCompleted: {
                                model = dataset.channelsNameList()

                                let index = model.indexOf(core.ch2Name)
                                if (index >= 0) {
                                    channel2Combo.currentIndex = index
                                }
                            }

                            Connections {
                                target: core
                                function onChannelListUpdated() {
                                    let list = dataset.channelsNameList()

                                    channel2Combo.suppressTextSignal = true

                                    channel2Combo.model = []
                                    channel2Combo.model = list

                                    let newIndex = list.indexOf(core.ch2Name)

                                    if (newIndex >= 0) {
                                        channel2Combo.currentIndex = newIndex
                                    }
                                    else {
                                        channel2Combo.currentIndex = 0
                                    }

                                    channel2Combo.suppressTextSignal = false
                                }
                            }
                        }
                    }

                    RowLayout {
                        CCheck {
                            id: echogramVisible
                            Layout.fillWidth: true
                            //                        Layout.preferredWidth: 150
                            checked: true
                            text: qsTr("Echogram")
                            onCheckedChanged: plotEchogramVisible(checked)
                            Component.onCompleted: plotEchogramVisible(checked)
                        }

                        CCombo  {
                            id: echoTheme
                            //                        Layout.fillWidth: true
                            Layout.preferredWidth: 150
                            model: [qsTr("Blue"), qsTr("Sepia"), qsTr("Sepia New"), qsTr("WRGBD"), qsTr("WhiteBlack"), qsTr("BlackWhite"), qsTr("DeepBlue"), qsTr("Ice"), qsTr("Green"), qsTr("Midnight")]
                            currentIndex: 0

                            onCurrentIndexChanged: {
                                plotEchogramTheme(currentIndex)
                                echogramThemeChanged(currentIndex)
                            }
                            Component.onCompleted: {
                                plotEchogramTheme(currentIndex)
                                echogramThemeChanged(currentIndex)
                            }

                            Settings {
                                category: "Plot2D_" + plot.indx

                                property alias waterfallThemeId: echoTheme.currentIndex
                            }
                        }

                        CCombo  {
                            id: echogramTypesList
                            //                        Layout.fillWidth: true
                            Layout.preferredWidth: 150
                            model: [qsTr("Raw"), qsTr("Side-Scan")]
                            currentIndex: 0

                            onCurrentIndexChanged: plotEchogramCompensation(currentIndex) // TODO
                            Component.onCompleted: plotEchogramCompensation(currentIndex) // TODO

                            Settings {
                                category: "Plot2D_" + plot.indx

                                property alias echogramTypesList: echogramTypesList.currentIndex
                            }
                        }
                    }

                    RowLayout {
                        visible: instruments > 0
                        CText {
                            Layout.fillWidth: true
                            text: qsTr("Bottom-Track")
                        }

                        CCheck {
                            id: bottomTrackValueVisible
                            text: qsTr("Value")
                            checked: true

                            onCheckedChanged: plot.updateBottomTrackPresentation()
                            Component.onCompleted: plot.updateBottomTrackPresentation()
                        }

                        CCheck {
                            id: bottomTrackGraphicsVisible
                            text: qsTr("Line")
                            checked: true

                            onCheckedChanged: plot.updateBottomTrackPresentation()
                            Component.onCompleted: plot.updateBottomTrackPresentation()
                        }

                        CCombo  {
                            id: bottomTrackThemeList
                            model: [qsTr("Line"), qsTr("Points")]
                            currentIndex: 0

                            onCurrentIndexChanged: plot.updateBottomTrackPresentation()
                            Component.onCompleted: plot.updateBottomTrackPresentation()

                            Settings {
                                category: "Plot2D_" + plot.indx

                                property alias bottomTrackThemeList: bottomTrackThemeList.currentIndex
                            }
                        }
                    }

                    RowLayout {
                        CText {
                            Layout.fillWidth: true
                            text: qsTr("Rangefinder")
                        }

                        CCheck {
                            id: rangefinderValueVisible
                            text: qsTr("Value")
                            checked: true

                            onCheckedChanged: plot.updateRangefinderPresentation()
                            Component.onCompleted: plot.updateRangefinderPresentation()
                        }

                        CCheck {
                            id: rangefinderGraphicsVisible
                            text: qsTr("Line")
                            checked: false

                            onCheckedChanged: plot.updateRangefinderPresentation()
                            Component.onCompleted: plot.updateRangefinderPresentation()
                        }

                        CCombo  {
                            id: rangefinderThemeList
                            model: [qsTr("Line"), qsTr("Points")]
                            currentIndex: 0

                            onCurrentIndexChanged: plot.updateRangefinderPresentation()
                            Component.onCompleted: plot.updateRangefinderPresentation()

                            Settings {
                                category: "Plot2D_" + plot.indx

                                property alias rangefinderThemeList: rangefinderThemeList.currentIndex
                            }
                        }
                    }


                    CCheck {
                        visible: instruments > 1
                        id: ahrsVisible
                        text: qsTr("Attitude")
                        onCheckedChanged: plotAttitudeVisible(checked)
                        Component.onCompleted: plotAttitudeVisible(checked)
                    }

                    CCheck {
                        visible: instruments > 1
                        id: temperatureVisible
                        text: qsTr("Temperature")
                        onCheckedChanged: plotTemperatureVisible(checked)
                        Component.onCompleted: plotTemperatureVisible(checked)
                    }

                    ColumnLayout {
                        visible: instruments > 1
                        id: dopplerBeamVisibleGroup
                        spacing: 0
                        function updateDopplerBeamVisible() {
                            // 3 bits per beam: V=bit(i*3), A=bit(i*3+1), M=bit(i*3+2)
                            var beamfilter =
                                dopplerBeam1V.checked*1    + dopplerBeam1A.checked*2    + dopplerBeam1M.checked*4 +
                                dopplerBeam2V.checked*8    + dopplerBeam2A.checked*16   + dopplerBeam2M.checked*32 +
                                dopplerBeam3V.checked*64   + dopplerBeam3A.checked*128  + dopplerBeam3M.checked*256 +
                                dopplerBeam4V.checked*512  + dopplerBeam4A.checked*1024 + dopplerBeam4M.checked*2048
                            plotDopplerBeamVisible(dopplerBeamVisible.checked, beamfilter)
                        }

                        RowLayout {
                            spacing: 0
                            CCheck {
                                id: dopplerBeamVisible
                                Layout.fillWidth: true
                                text: qsTr("Doppler Beams")
                                onCheckedChanged: dopplerBeamVisibleGroup.updateDopplerBeamVisible()
                                Component.onCompleted: dopplerBeamVisibleGroup.updateDopplerBeamVisible()
                            }
                        }

                        RowLayout {
                            visible: dopplerBeamVisible.checked
                            spacing: 4
                            Item { width: 8 }
                            CCheck { id: dopplerBeam1A; checked: true; text: "1 " + qsTr("Depth");    onCheckedChanged: dopplerBeamVisibleGroup.updateDopplerBeamVisible() }
                            CCheck { id: dopplerBeam1V; checked: true; text: "1 " + qsTr("Velocity"); onCheckedChanged: dopplerBeamVisibleGroup.updateDopplerBeamVisible() }
                            CCheck { id: dopplerBeam1M; checked: true; text: "1 " + qsTr("Mode");     onCheckedChanged: dopplerBeamVisibleGroup.updateDopplerBeamVisible() }
                        }

                        RowLayout {
                            visible: dopplerBeamVisible.checked
                            spacing: 4
                            Item { width: 8 }
                            CCheck { id: dopplerBeam2A; checked: true; text: "2 " + qsTr("Depth");    onCheckedChanged: dopplerBeamVisibleGroup.updateDopplerBeamVisible() }
                            CCheck { id: dopplerBeam2V; checked: true; text: "2 " + qsTr("Velocity"); onCheckedChanged: dopplerBeamVisibleGroup.updateDopplerBeamVisible() }
                            CCheck { id: dopplerBeam2M; checked: true; text: "2 " + qsTr("Mode");     onCheckedChanged: dopplerBeamVisibleGroup.updateDopplerBeamVisible() }
                        }

                        RowLayout {
                            visible: dopplerBeamVisible.checked
                            spacing: 4
                            Item { width: 8 }
                            CCheck { id: dopplerBeam3A; checked: true; text: "3 " + qsTr("Depth");    onCheckedChanged: dopplerBeamVisibleGroup.updateDopplerBeamVisible() }
                            CCheck { id: dopplerBeam3V; checked: true; text: "3 " + qsTr("Velocity"); onCheckedChanged: dopplerBeamVisibleGroup.updateDopplerBeamVisible() }
                            CCheck { id: dopplerBeam3M; checked: true; text: "3 " + qsTr("Mode");     onCheckedChanged: dopplerBeamVisibleGroup.updateDopplerBeamVisible() }
                        }

                        RowLayout {
                            visible: dopplerBeamVisible.checked
                            spacing: 4
                            Item { width: 8 }
                            CCheck { id: dopplerBeam4A; checked: true; text: "4 " + qsTr("Depth");    onCheckedChanged: dopplerBeamVisibleGroup.updateDopplerBeamVisible() }
                            CCheck { id: dopplerBeam4V; checked: true; text: "4 " + qsTr("Velocity"); onCheckedChanged: dopplerBeamVisibleGroup.updateDopplerBeamVisible() }
                            CCheck { id: dopplerBeam4M; checked: true; text: "4 " + qsTr("Mode");     onCheckedChanged: dopplerBeamVisibleGroup.updateDopplerBeamVisible() }
                        }
                    }

                    ColumnLayout {
                        visible: instruments > 1
                        id: dopplerInstrumentVisibleGroup
                        spacing: 0
                        function updateDopplerInstrumentVisible() {
                            var linefilter = dopplerInstrumentXVisible.checked*1 +
                                             dopplerInstrumentYVisible.checked*2 +
                                             dopplerInstrumentZVisible.checked*4 +
                                             dopplerInstrumentAVisible.checked*8 +
                                             dopplerInstrumentDstVisible.checked*16
                            plotDopplerInstrumentVisible(dopplerInstrumentVisible.checked, linefilter)
                        }

                        RowLayout {
                            spacing: 0
                            CCheck {
                                id: dopplerInstrumentVisible
                                Layout.fillWidth: true
                                text: qsTr("Doppler Instrument")
                                onCheckedChanged: dopplerInstrumentVisibleGroup.updateDopplerInstrumentVisible()
                                Component.onCompleted: dopplerInstrumentVisibleGroup.updateDopplerInstrumentVisible()
                            }
                        }

                        RowLayout {
                            visible: dopplerInstrumentVisible.checked
                            spacing: 4
                            Item { width: 8 }
                            CCheck { id: dopplerInstrumentXVisible;   checked: true; text: "X";                   onCheckedChanged: dopplerInstrumentVisibleGroup.updateDopplerInstrumentVisible() }
                            CCheck { id: dopplerInstrumentYVisible;   checked: true; text: "Y";                   onCheckedChanged: dopplerInstrumentVisibleGroup.updateDopplerInstrumentVisible() }
                            CCheck { id: dopplerInstrumentZVisible;   checked: true; text: "Z";                   onCheckedChanged: dopplerInstrumentVisibleGroup.updateDopplerInstrumentVisible() }
                            CCheck { id: dopplerInstrumentAVisible;   checked: true; text: qsTr("Abs. Velocity"); onCheckedChanged: dopplerInstrumentVisibleGroup.updateDopplerInstrumentVisible() }
                            CCheck { id: dopplerInstrumentDstVisible; checked: true; text: qsTr("Depth");         onCheckedChanged: dopplerInstrumentVisibleGroup.updateDopplerInstrumentVisible() }
                        }
                    }

                    RowLayout {
                        visible: instruments > 1
                        spacing: 0

                        CCheck {
                            id: dvlLegendVisible
                            Layout.fillWidth: true
                            text: qsTr("DVL Legend")
                            checked: true
                            onCheckedChanged: plotDVLLegendVisible(checked)
                            Component.onCompleted: plotDVLLegendVisible(checked)
                        }

                        CCombo {
                            visible: dvlLegendVisible.checked
                            id: dvlLegendPosition
                            enabled: dvlLegendVisible.checked
                            model: [qsTr("Top"), qsTr("Center"), qsTr("Bottom")]
                            currentIndex: 0
                            onCurrentIndexChanged: plotDVLLegendPosition(currentIndex)
                            Component.onCompleted: plotDVLLegendPosition(currentIndex)

                            Settings {
                                category: "Plot2D_" + plot.indx
                                property alias dvlLegendPosition: dvlLegendPosition.currentIndex
                            }
                        }
                    }

                    RowLayout {
                        visible: instruments > 1
                        id: acousticAngleGroup
                        spacing: 0

                        CCheck {
                            id: acousticAngleVisible
                            Layout.fillWidth: true
                            text: qsTr("Acoustic angle")
                            onCheckedChanged: plotAcousticAngleVisible(checked);
                            Component.onCompleted: plotAcousticAngleVisible(checked);
                        }
                    }

                    RowLayout {
                        visible: instruments > 1
                        CCheck {
                            id: adcpVisible
                            enabled: false
                            Layout.fillWidth: true
                            text: qsTr("Doppler Profiler")
                        }
                    }

                    RowLayout {
                        visible: instruments > 1
                        CCheck {
                            id: gnssVisible
                            checked: false
                            Layout.fillWidth: true
                            text: qsTr("GNSS data")

                            onCheckedChanged: plotGNSSVisible(checked, 1)
                            Component.onCompleted: plotGNSSVisible(checked, 1)

                            Settings {
                                category: "Plot2D_" + plot.indx

                                property alias gnssVisible: gnssVisible.checked
                            }
                        }
                    }


                    RowLayout {
                        RowLayout {
                            CCheck {
                                id: gridVisible
                                Layout.fillWidth: true
                                text: qsTr("Grid")
                                onCheckedChanged: plotGridVerticalNumber(gridNumber.value*gridVisible.checked)
                            }
                            CCheck {
                                id: fillWidthGrid
                                Layout.fillWidth: true
                                text: qsTr("fill")
                                onCheckedChanged: plotGridFillWidth(checked)
                                visible: gridVisible.checked

                                Component.onCompleted: {
                                    plotGridFillWidth(checked)
                                }
                                Settings {
                                    category: "Plot2D_" + plot.indx

                                    property alias fillWidthGrid: fillWidthGrid.checked
                                }
                            }
                            CCheck {
                                id: invertGrid
                                Layout.fillWidth: true
                                text: qsTr("invert")
                                onCheckedChanged: plotGridInvert(checked)
                                visible: gridVisible.checked

                                Component.onCompleted: {
                                    plotGridInvert(checked)
                                }
                                Settings {
                                    category: "Plot2D_" + plot.indx
                                    property alias invertGrid: invertGrid.checked
                                }
                            }
                        }

                        SpinBoxCustom {
                            id: gridNumber
                            from: 1
                            to: 24
                            stepSize: 1
                            value: 5

                            onValueChanged: plotGridVerticalNumber(gridNumber.value*gridVisible.checked)
                            Component.onCompleted: plotGridVerticalNumber(gridNumber.value*gridVisible.checked)

                            Settings {
                                category: "Plot2D_" + plot.indx

                                property alias gridNumber: gridNumber.value
                            }
                        }
                    }

                    RowLayout {
                        visible: instruments > 1

                        CCheck {
                            id: angleVisible
                            Layout.fillWidth: true
                            text: qsTr("Angle range, °")
                            onCheckedChanged: plotAngleVisibility(checked)
                            Component.onCompleted: plotAngleVisibility(checked)

                            Settings {
                                category: "Plot2D_" + plot.indx

                                property alias angleVisible: angleVisible.checked
                            }
                        }

                        SpinBoxCustom {
                            id: angleRange
                            from: 1
                            to: 360
                            stepSize: 1
                            value: 45

                            onValueChanged: plotAngleRange(angleRange.currValue)
                            Component.onCompleted: plotAngleRange(angleRange.currValue)

                            property int currValue: value

                            validator: DoubleValidator {
                                bottom: Math.min(angleRange.from, angleRange.to)
                                top:  Math.max(angleRange.from, angleRange.to)
                            }

                            textFromValue: function(value, locale) {
                                return Number(value).toLocaleString(locale, 'f', 0)
                            }

                            valueFromText: function(text, locale) {
                                return Number.fromLocaleString(locale, text)
                            }

                            onCurrValueChanged: plotAngleRange(currValue)

                            Settings {
                                category: "Plot2D_" + plot.indx

                                property alias angleRange: angleRange.value
                            }
                        }
                    }


                    RowLayout {
                        visible: instruments > 1
                        CCheck {
                            id: velocityVisible
                            Layout.fillWidth: true
                            text: qsTr("Velocity range, m/s")
                            onCheckedChanged: plotVelocityVisible(checked)
                            Component.onCompleted: plotVelocityVisible(checked)

                            Settings {
                                category: "Plot2D_" + plot.indx

                                property alias velocityVisible: velocityVisible.checked
                            }
                        }

                        SpinBoxCustom {
                            id: velocityRange
                            from: 500
                            to: 1000*8
                            stepSize: 500
                            value: 5

                            onValueChanged: plotVelocityRange(velocityRange.realValue)
                            Component.onCompleted: plotVelocityRange(velocityRange.realValue)

                            property int decimals: 1
                            property real realValue: value / 1000

                            validator: DoubleValidator {
                                bottom: Math.min(velocityRange.from, velocityRange.to)
                                top:  Math.max(velocityRange.from, velocityRange.to)
                            }

                            textFromValue: function(value, locale) {
                                return Number(value / 1000).toLocaleString(locale, 'f', decimals)
                            }

                            valueFromText: function(text, locale) {
                                return Number.fromLocaleString(locale, text) * 1000
                            }

                            onRealValueChanged: plotVelocityRange(realValue)

                            Settings {
                                category: "Plot2D_" + plot.indx

                                property alias velocityRange: velocityRange.value
                            }
                        }
                    }

                    RowLayout {
                        id: distanceAutoRangeRow
                        function distanceAutorangeMode() {
                            plotDistanceAutoRange(distanceAutoRange.checked ? distanceAutoRangeList.currentIndex : -1)
                        }

                        CCheck {
                            id: distanceAutoRange
                            checked: true
                            Layout.fillWidth: true
                            text: qsTr("Distance auto range")

                            onCheckedChanged: {
                                distanceAutoRangeRow.distanceAutorangeMode()
                            }
                            Component.onCompleted: distanceAutoRangeRow.distanceAutorangeMode()

                            Settings {
                                category: "Plot2D_" + plot.indx

                                property alias distanceAutoRange: distanceAutoRange.checked
                            }
                        }

                        CCombo  {
                            id: distanceAutoRangeList
                            model: [qsTr("Last data       "), qsTr("Last on screen"), qsTr("Max on screen")]
                            currentIndex: 0
                            onCurrentIndexChanged: distanceAutoRangeRow.distanceAutorangeMode()
                            Component.onCompleted: distanceAutoRangeRow.distanceAutorangeMode()

                            Settings {
                                category: "Plot2D_" + plot.indx

                                property alias distanceAutoRangeList: distanceAutoRangeList.currentIndex
                            }
                        }
                    }

                    CCheck {
                        id: horisontalVertical
                        checked: true
                        text: qsTr("Horizontal")
                    }

                    RowLayout {
                        CCheck {
                            id: loupeVisible
                            Layout.fillWidth: true
                            checked: false
                            text: qsTr("Loupe")

                            onCheckedChanged: plotLoupeVisible(checked)
                            Component.onCompleted: plotLoupeVisible(checked)
                        }

                        RowLayout {
                            visible: loupeVisible.checked

                            CText {
                                text: qsTr("size")
                            }
                            SpinBoxCustom {
                                id: loupeSize
                                from: 1
                                to: 3
                                stepSize: 1
                                value: 1

                                onValueChanged: plotLoupeSize(value)
                                Component.onCompleted: plotLoupeSize(value)
                            }
                        }
                        RowLayout {
                            visible: loupeVisible.checked
                            spacing: Math.max(6, Math.round(theme.controlHeight * 0.2))

                            CText {
                                text: qsTr("zoom")
                            }

                            ChartLevelSingle {
                                id: loupeZoom
                                Layout.fillWidth: true
                                Layout.preferredWidth: theme.controlHeight * 5
                                from: 0
                                to: 300
                                stepSize: 1
                                value: 100

                                onValueChanged: plotLoupeZoom(Math.round(value))

                                onPressedChanged: {
                                    if (pressed) {
                                        plot.beginLoupeZoomPreview()
                                    }
                                    else {
                                        plot.endLoupeZoomPreview()
                                    }
                                }

                                onMoved: {
                                    plot.updateLoupeZoomPreview()
                                }

                                Component.onCompleted: plotLoupeZoom(Math.round(value))
                            }

                            CText {
                                text: Math.round(loupeZoom.value) + "%"
                                small: true
                                horizontalAlignment: Text.AlignRight
                                Layout.preferredWidth: theme.controlHeight * 1.7
                            }
                        }
                    }

                    Settings {
                        category: "Plot2D_" + plot.indx

                        property alias echogramVisible: echogramVisible.checked
                        property alias rangefinderVisible: rangefinderGraphicsVisible.checked
                        property alias rangefinderValueVisible: rangefinderValueVisible.checked
                        property alias postProcVisible: bottomTrackGraphicsVisible.checked
                        property alias bottomTrackValueVisible: bottomTrackValueVisible.checked
                        property alias rangefinderGraphicsVisible: rangefinderGraphicsVisible.checked
                        property alias bottomTrackGraphicsVisible: bottomTrackGraphicsVisible.checked
                        property alias ahrsVisible: ahrsVisible.checked
                        property alias temperatureVisible: temperatureVisible.checked
                        property alias gridVisible: gridVisible.checked
                        property alias dopplerBeamVisible: dopplerBeamVisible.checked
                        property alias dopplerBeam1A: dopplerBeam1A.checked
                        property alias dopplerBeam1V: dopplerBeam1V.checked
                        property alias dopplerBeam1M: dopplerBeam1M.checked
                        property alias dopplerBeam2A: dopplerBeam2A.checked
                        property alias dopplerBeam2V: dopplerBeam2V.checked
                        property alias dopplerBeam2M: dopplerBeam2M.checked
                        property alias dopplerBeam3A: dopplerBeam3A.checked
                        property alias dopplerBeam3V: dopplerBeam3V.checked
                        property alias dopplerBeam3M: dopplerBeam3M.checked
                        property alias dopplerBeam4A: dopplerBeam4A.checked
                        property alias dopplerBeam4V: dopplerBeam4V.checked
                        property alias dopplerBeam4M: dopplerBeam4M.checked
                        property alias dopplerInstrumentVisible: dopplerInstrumentVisible.checked
                        property alias dopplerInstrumentX: dopplerInstrumentXVisible.checked
                        property alias dopplerInstrumentY: dopplerInstrumentYVisible.checked
                        property alias dopplerInstrumentZ: dopplerInstrumentZVisible.checked
                        property alias dopplerInstrumentA: dopplerInstrumentAVisible.checked
                        property alias dopplerInstrumentDst: dopplerInstrumentDstVisible.checked
                        property alias dvlLegendVisible: dvlLegendVisible.checked
                        property alias horisontalVertical: horisontalVertical.checked
                        property alias loupeVisible: loupeVisible.checked
                        property alias loupeSize: loupeSize.value
                        property alias loupeZoom: loupeZoom.value
                    }
                }
            } // menu frame
        } // menu scrol
    } // row layout

    CContact {
        id: contactDialog

        onVisibleChanged: {
            if (!visible) {
                parent.focus = true

                if (accepted) {
                    plot.setContact(contactDialog.indx, contactDialog.inputFieldText)
                    updateOtherPlot(plot.indx)
                    accepted = false
                }
                contactDialog.info = ""
                contactDialog.inputFieldText = ""
            }
        }

        onDeleteButtonClicked: {
            plot.deleteContact(contactDialog.indx)
            updateOtherPlot(plot.indx)
        }

        onCopyButtonClicked: {
            plot.updateContact()
        }

        onSetActiveButtonClicked: {
            plot.setActiveContact(contactDialog.indx)
        }

        onInputAccepted: {
            contactDialog.visible = false
            plot.updateContact()
        }

        onSetButtonClicked: {
            contactDialog.visible = false
            plot.updateContact()
        }
    }

    onContactVisibleChanged: {
        contactDialog.visible = plot.contactVisible;

        if (contactDialog.visible) {
            contactDialog.info = plot.contactInfo
            contactDialog.inputFieldText =  plot.contactInfo
        }
        else {
            contactDialog.info = ""
            contactDialog.inputFieldText = ""
        }

        contactDialog.x = plot.contactPositionX
        contactDialog.y = plot.contactPositionY
        contactDialog.indx = plot.contactIndx
        contactDialog.lat = plot.contactLat
        contactDialog.lon = plot.contactLon
        contactDialog.depth = plot.contactDepth
    }

    RowLayout {
        id: menuBlock
        Layout.alignment: Qt.AlignHCenter
        spacing: 1
        visible: false
        Layout.margins: 0

        function position(mx, my) {
            var oy = plot.height - (my + implicitHeight)
            if(oy < 0) {
                my = my + oy
            }

            if(my < 0) {
                my = 0
            }

            var ox = plot.width - (mx - implicitWidth)
            if(ox < 0) {
                mx = mx + ox
            }

            x = mx
            y = my
            visible = true
//            backgrn.focus = true
        }

        ButtonGroup { id: pencilbuttonGroup }

        CheckButton {
            icon.source: "qrc:/icons/ui/direction_arrows.svg"
            checked: true
            backColor: theme.controlBackColor
            implicitWidth: theme.controlHeight

            onCheckedChanged: {
                if (checked) {
                    plot.plotMouseTool(1)
                }
            }

            ButtonGroup.group: pencilbuttonGroup
        }

        CheckButton {
            icon.source: "qrc:/icons/ui/arrow_bar_to_down.svg"
            backColor: theme.controlBackColor
            implicitWidth: theme.controlHeight

            onCheckedChanged: {
                if (checked) {
                    plot.plotMouseTool(2)
                }
            }

            ButtonGroup.group: pencilbuttonGroup
        }

        CheckButton {
            icon.source: "qrc:/icons/ui/pencil.svg"
            backColor: theme.controlBackColor
            implicitWidth: theme.controlHeight

            onCheckedChanged: {
                if (checked) {
                    plot.plotMouseTool(3)
                }
            }

            ButtonGroup.group: pencilbuttonGroup
        }

        CheckButton {
            icon.source: "qrc:/icons/ui/arrow_bar_to_up.svg"
            backColor: theme.controlBackColor
            implicitWidth: theme.controlHeight

            onCheckedChanged: {
                if (checked) {
                    plot.plotMouseTool(4)
                }
            }

            ButtonGroup.group: pencilbuttonGroup
        }

        CheckButton {
            icon.source: "qrc:/icons/ui/eraser.svg"
            backColor: theme.controlBackColor
            implicitWidth: theme.controlHeight

            onCheckedChanged: {
                if (checked) {
                    plot.plotMouseTool(5)
                }
            }

            ButtonGroup.group: pencilbuttonGroup
        }

        CheckButton {
            icon.source: "qrc:/icons/ui/anchor.svg"
            backColor: theme.controlBackColor
            implicitWidth: theme.controlHeight
            checkable: false

            onClicked: {
                contactDialog.x = mousearea.contactMouseX
                contactDialog.y = mousearea.contactMouseY
                contactDialog.visible = true;

                contactDialog.indx = -1

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
                menuBlock.visible = false
            }

            ButtonGroup.group: pencilbuttonGroup
        }
    }
}
