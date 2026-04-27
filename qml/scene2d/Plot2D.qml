import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Dialogs
import QtCore

import WaterFall 1.0
import "../controls"
import "../menus"
import "../settings"

WaterFall {
    id: plot

    property bool is3dVisible: false
    property bool scrollBarsShown: true
    property bool isLiveMode: core.openedFilePath.length === 0
    onIsLiveModeChanged: {
        if (isLiveMode) {
            timelinePosition = 1.0
            updateOtherPlot(indx)
        }
    }

    Connections {
        target: core
        function onChannelListUpdated() {
            if (plot.isLiveMode) {
                plot.timelinePosition = 1.0
                updateOtherPlot(plot.indx)
            }
        }
    }

    Timer {
        id: scrollHideTimer
        interval: 5000
        repeat: false
        onTriggered: plot.scrollBarsShown = false
        Component.onCompleted: start()
    }
    property int indx: 0
    property var instrumentsGradeList: null
    property int instruments: theme ? theme.instrumentsGrade
                              : ((instrumentsGradeList && typeof instrumentsGradeList.currentIndex === "number")
                                 ? instrumentsGradeList.currentIndex : 0)
    property bool settingsOpen: plotCheckButton.checked
    property bool hasTransientUi: menuBlock.visible || contactDialog.visible
    property bool loupeZoomAdjusting: false
    property bool loupeZoomWasVisibleBeforeAdjust: false
    property int loupeZoomSavedAimEpoch: -1
    property real settingsMenuSpacer: Math.max(4, Math.round(theme.controlHeight * 0.2))
    property var inputState: null
    property bool externalInputRouting: false
    property int pointerLastMouseX: -1
    property int pointerLastMouseY: -1
    property bool pointerWasMoved: false
    property point pointerStartMousePos: Qt.point(-1, -1)
    property real pointerMouseThreshold: 15
    property int pointerContactMouseX: -1
    property int pointerContactMouseY: -1
    property int pinchThresholdXAxis: 15
    property int pinchThresholdYAxis: 15
    property double pinchZoomThreshold: 0.1
    property bool pinchMovementX: false
    property bool pinchMovementY: false
    property bool pinchZoomY: false
    property point pinchStartPos: Qt.point(-1, -1)
    property bool pinchActive: false

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

    function markMouseKeyboardInput() {
        if (inputState && typeof inputState.markMouseKeyboardInput === "function")
            inputState.markMouseKeyboardInput()
    }

    function markTouchInput() {
        if (inputState && typeof inputState.markTouchInput === "function")
            inputState.markTouchInput()
    }

    function clearPinchMovementState() {
        pinchMovementX = false
        pinchMovementY = false
        pinchZoomY = false
    }

    function handlePointerClick(mouseButton, buttons, x, y, keyboardKey) {
        pointerLastMouseX = x
        pointerLastMouseY = y
        forceActiveFocus()

        if (mouseButton === Qt.RightButton) {
            pointerContactMouseX = x
            pointerContactMouseY = y
            plot.simplePlotMousePosition(x, y)

            if (theme.instrumentsGrade !== 0) {
                menuBlock.position(x, y)
            }
        }

        pointerWasMoved = false
    }

    function handlePointerPress(mouseButton, buttons, x, y, keyboardKey) {
        markMouseKeyboardInput()
        scrollBarsShown = true
        scrollHideTimer.restart()
        pointerLastMouseX = x
        pointerLastMouseY = y
        forceActiveFocus()

        if (Qt.platform.os === "android") {
            pointerStartMousePos = Qt.point(x, y)
            longPressTimer.start()
        }

        if (mouseButton === Qt.LeftButton) {
            menuBlock.visible = false
            plot.plotMousePosition(x, y)
            plotPressed(indx, x, y)
        }

        if (mouseButton === Qt.RightButton) {
            pointerContactMouseX = x
            pointerContactMouseY = y
            plot.simplePlotMousePosition(x, y)
        }

        pointerWasMoved = false
    }

    function handlePointerMove(buttons, x, y, keyboardKey) {
        markMouseKeyboardInput()
        plot.onCursorMoved(x, y)

        if (Qt.platform.os === "android") {
            if (!pointerWasMoved) {
                var currDelta = Math.sqrt(Math.pow((x - pointerStartMousePos.x), 2) + Math.pow((y - pointerStartMousePos.y), 2))
                if (currDelta > pointerMouseThreshold) {
                    pointerWasMoved = true
                }
            }
        }

        pointerLastMouseX = x
        pointerLastMouseY = y

        if (buttons & Qt.LeftButton) {
            plot.plotMousePosition(x, y)
            plotPressed(indx, x, y)
        }

        if (buttons & Qt.RightButton) {
            pointerContactMouseX = x
            pointerContactMouseY = y
            plot.simplePlotMousePosition(x, y)
        }
    }

    function handlePointerRelease(mouseButton, buttons, x, y, keyboardKey) {
        markMouseKeyboardInput()
        pointerLastMouseX = -1
        pointerLastMouseY = -1

        if (Qt.platform.os === "android") {
            longPressTimer.stop()
        }

        if (mouseButton === Qt.LeftButton) {
            plot.plotMousePosition(-1, -1)
        }

        if (mouseButton === Qt.RightButton) {
            pointerContactMouseX = x
            pointerContactMouseY = y
            plot.simplePlotMousePosition(x, y)
        }

        pointerWasMoved = false
        pointerStartMousePos = Qt.point(-1, -1)
        plotReleased(indx)
    }

    function handlePointerCancel() {
        markMouseKeyboardInput()
        pointerLastMouseX = -1
        pointerLastMouseY = -1

        if (Qt.platform.os === "android") {
            longPressTimer.stop()
        }

        pointerWasMoved = false
        pointerStartMousePos = Qt.point(-1, -1)
        plotReleased(indx)
    }

    function handlePointerWheel(buttons, x, y, angleDelta, modifiers, keyboardKey) {
        markMouseKeyboardInput()

        if (modifiers & Qt.ControlModifier) {
            let val = -angleDelta.y
            plot.verZoomEvent(val)
            plotCursorChanged(indx, cursorFrom(), cursorTo())
        }
        else if (modifiers & Qt.ShiftModifier) {
            let val = -angleDelta.y
            plot.verScrollEvent(val)
            plotCursorChanged(indx, cursorFrom(), cursorTo())
        }
        else {
            let val = angleDelta.y
            plot.horScrollEvent(val)
            updateOtherPlot(indx)
        }
    }

    function handlePinchStarted(centerX, centerY) {
        markTouchInput()
        pinchActive = true
        menuBlock.visible = false
        plot.plotMousePosition(-1, -1)
        clearPinchMovementState()
        pinchStartPos = Qt.point(centerX, centerY)
    }

    function handlePinchUpdated(prevCenterX, prevCenterY, currCenterX, currCenterY, prevScale, scale, prevAngle, angle) {
        markTouchInput()
        if (pinchMovementX) {
            let val = -(prevCenterX - currCenterX)
            plot.horScrollEvent(val)
            updateOtherPlot(indx)
        }
        else if (pinchMovementY) {
            let val = prevCenterY - currCenterY
            plot.verScrollEvent(val)
            plotCursorChanged(indx, cursorFrom(), cursorTo())
        }
        else if (pinchZoomY) {
            let val = (prevScale - scale) * 500.0
            plot.verZoomEvent(val)
            plotCursorChanged(indx, cursorFrom(), cursorTo())
        }
        else {
            if (Math.abs(pinchStartPos.x - currCenterX) > pinchThresholdXAxis) {
                pinchMovementX = true
            }
            else if (Math.abs(pinchStartPos.y - currCenterY) > pinchThresholdYAxis) {
                pinchMovementY = true
            }
            else if (scale > (1.0 + pinchZoomThreshold) || scale < (1.0 - pinchZoomThreshold)) {
                pinchZoomY = true
            }
        }
    }

    function handlePinchFinished() {
        markTouchInput()
        pinchActive = false
        plot.plotMousePosition(-1, -1)
        clearPinchMovementState()
        pinchStartPos = Qt.point(-1, -1)
    }

    onEnabledChanged: {
        setPlotEnabled(enabled)
        if (enabled) {
            update();
        }
    }

    Component.onCompleted: {
        setPlotEnabled(enabled)
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
        enabled: !plot.externalInputRouting

        onPinchStarted: plot.handlePinchStarted(pinch.center.x, pinch.center.y)

        onPinchUpdated: plot.handlePinchUpdated(pinch.previousCenter.x, pinch.previousCenter.y,
                                                 pinch.center.x, pinch.center.y,
                                                 pinch.previousScale, pinch.scale,
                                                 pinch.previousAngle, pinch.angle)

        onPinchFinished: plot.handlePinchFinished()

        MouseArea {
            id: mousearea
            enabled: !plot.externalInputRouting && !plot.pinchActive
            anchors.fill: parent
            acceptedButtons: Qt.LeftButton | Qt.RightButton

            hoverEnabled: true

            Timer {
                id: longPressTimer
                interval: 500
                repeat: false
                onTriggered: {
                    if (Qt.platform.os === "android" && theme.instrumentsGrade !== 0 && !plot.pointerWasMoved) {
                        plot.onCursorMoved(plot.pointerLastMouseX, plot.pointerLastMouseY)
                        plot.pointerContactMouseX = plot.pointerLastMouseX
                        plot.pointerContactMouseY = plot.pointerLastMouseY
                        plot.simplePlotMousePosition(plot.pointerLastMouseX, plot.pointerLastMouseY)

                        menuBlock.position(plot.pointerLastMouseX, plot.pointerLastMouseY)
                    }
                }
            }

            onClicked: function(mouse) {
                plot.handlePointerClick(mouse.button, mouse.buttons, mouse.x, mouse.y, Qt.Key_unknown)
            }

            onPressed: function(mouse) {
                plot.handlePointerPress(mouse.button, mouse.buttons, mouse.x, mouse.y, Qt.Key_unknown)
                if (Qt.platform.os === "android")
                    longPressTimer.start()
            }

            onReleased: function(mouse) {
                if (Qt.platform.os === "android")
                    longPressTimer.stop()
                plot.handlePointerRelease(mouse.button, mouse.buttons, mouse.x, mouse.y, Qt.Key_unknown)
            }

            onCanceled: {
                if (Qt.platform.os === "android")
                    longPressTimer.stop()
                plot.handlePointerCancel()
            }

            onPositionChanged: function(mouse) {
                plot.handlePointerMove(mouse.buttons, mouse.x, mouse.y, Qt.Key_unknown)
            }

            onWheel: function(wheel) {
                plot.handlePointerWheel(wheel.buttons, wheel.x, wheel.y, wheel.angleDelta, wheel.modifiers, Qt.Key_unknown)
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
                                    rowDataset.setChannelNamesToBackend()
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
                                    rowDataset.setChannelNamesToBackend()
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

                    RowLayout {
                        visible: instruments > 1
                        id: dopplerBeamVisibleGroup
                        spacing: 0
                        function updateDopplerBeamVisible() {
                            var beamfilter = dopplerBeam1Visible.checked*1 + dopplerBeam2Visible.checked*2 + dopplerBeam3Visible.checked*4 + dopplerBeam4Visible.checked*8
                            plotDopplerBeamVisible(dopplerBeamVisible.checked,
                                                   beamfilter)
                        }

                        CCheck {
                            id: dopplerBeamVisible
                            Layout.fillWidth: true
                            text: qsTr("Doppler Beams")
                            onCheckedChanged: dopplerBeamVisibleGroup.updateDopplerBeamVisible()
                            Component.onCompleted: dopplerBeamVisibleGroup.updateDopplerBeamVisible()
                        }

                        CCheck {
                            id: dopplerBeam1Visible
                            enabled: true
                            checked: true
                            text: "1"

                            onCheckedChanged: dopplerBeamVisibleGroup.updateDopplerBeamVisible()
                        }

                        CCheck {
                            id: dopplerBeam2Visible
                            leftPadding: 0
                            enabled: true
                            checked: true
                            text: "2"
                            onCheckedChanged: dopplerBeamVisibleGroup.updateDopplerBeamVisible()
                        }

                        CCheck {
                            id: dopplerBeam3Visible
                            leftPadding: 0
                            enabled: true
                            checked: true
                            text: "3"
                            onCheckedChanged: dopplerBeamVisibleGroup.updateDopplerBeamVisible()
                        }

                        CCheck {
                            id: dopplerBeam4Visible
                            leftPadding: 0
                            enabled: true
                            checked: true
                            text: "4"
                            onCheckedChanged: dopplerBeamVisibleGroup.updateDopplerBeamVisible()
                        }

                        CCheck {
                            id: dopplerBeamAmpVisible
                            enabled: true
                            checked: true
                            text: "A"
                            onCheckedChanged: dopplerBeamVisibleGroup.updateDopplerBeamVisible()
                        }

                        CCheck {
                            id: dopplerBeamModeVisible
                            leftPadding: 0
                            enabled: true
                            checked: true
                            text: "M"
                            onCheckedChanged: dopplerBeamVisibleGroup.updateDopplerBeamVisible()
                        }
                    }

                    RowLayout {
                        visible: instruments > 1
                        spacing: 0
                        CCheck {
                            id: dopplerInstrumentVisible
                            Layout.fillWidth: true
                            text: qsTr("Doppler Instrument")
                            onCheckedChanged: plotDopplerInstrumentVisible(checked)
                            Component.onCompleted: plotDopplerInstrumentVisible(checked)
                        }

                        CCheck {
                            id: dopplerInstrumentXVisible
                            enabled: false
                            checked: true
                            text: "X"
                            //                        onCheckedChanged: setDopplerInstrumentVis(checked)
                            //                        Component.onCompleted: setDopplerInstrumentVis(checked)
                        }

                        CCheck {
                            id: dopplerInstrumentYVisible
                            enabled: false
                            checked: true
                            text: "Y"
                            //                        onCheckedChanged: setDopplerInstrumentVis(checked)
                            //                        Component.onCompleted: setDopplerInstrumentVis(checked)
                        }

                        CCheck {
                            id: dopplerInstrumentZVisible
                            enabled: false
                            checked: true
                            text: "Z"
                            //                        onCheckedChanged: setDopplerInstrumentVis(checked)
                            //                        Component.onCompleted: setDopplerInstrumentVis(checked)
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
                        property alias dopplerInstrumentVisible: dopplerInstrumentVisible.checked
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

    // Horizontal mode scroll bookmark (bottom edge)
    Item {
        id: echoScrollBarH

        property bool isScrolling: false
        property real grabOffsetX: 0
        property real prevTimelinePos: -1

        readonly property bool trackVisible: scrollMouseH.pressed || isScrolling
        readonly property real thumbW: (scrollMouseH.pressed || isScrolling) ? 96 : 68
        readonly property real thumbH: (scrollMouseH.pressed || isScrolling) ? 28 : 20

        visible: plot.horizontal && plot.hasData
        opacity: (plot.scrollBarsShown || trackVisible) ? 1.0 : 0.0

        onTrackVisibleChanged: {
            if (trackVisible) {
                plot.scrollBarsShown = true
                scrollHideTimer.restart()
            }
        }

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.leftMargin: 6
        anchors.rightMargin: 6
        anchors.bottomMargin: 6
        height: thumbH
        z: 10

        Behavior on height   { NumberAnimation { duration: 120; easing.type: Easing.OutCubic } }
        Behavior on opacity  { NumberAnimation { duration: 400 } }

        Timer {
            id: scrollFadeH
            interval: 250
            repeat: false
            onTriggered: echoScrollBarH.isScrolling = false
        }

        Connections {
            target: plot
            function onTimelinePositionChanged() {
                if (scrollMouseH.pressed) return
                const pos = plot.timelinePosition
                if (Math.abs(pos - echoScrollBarH.prevTimelinePos) > 0.00005) {
                    echoScrollBarH.isScrolling = true
                    scrollFadeH.restart()
                }
                echoScrollBarH.prevTimelinePos = pos
            }
        }

        Rectangle {
            anchors.fill: parent
            radius: parent.height / 2
            color: Qt.rgba(0, 0, 0, 0.25)
            opacity: echoScrollBarH.trackVisible ? 1.0 : 0.0
            Behavior on opacity { NumberAnimation { duration: 200 } }
        }

        Rectangle {
            id: snapTrailH
            x: scrollThumbH.x
            width: Math.max(0, echoScrollBarH.width - x)
            height: parent.height
            radius: height / 2
            color: theme.controlBorderColor
            opacity: (scrollMouseH.pressed && plot.isLiveMode && scrollThumbH.progress > 0.85) ? 1.0 : 0.0
            Behavior on opacity { NumberAnimation { duration: 150 } }
        }

        Rectangle {
            id: ghostThumbH
            x: echoScrollBarH.width - width
            width: echoScrollBarH.thumbW
            height: parent.height
            radius: height / 2
            color: "transparent"
            border.color: theme.controlBorderColor
            border.width: 2
            opacity: (scrollMouseH.pressed && plot.isLiveMode && scrollThumbH.progress > 0.85) ? 0.65 : 0.0
            Behavior on opacity { NumberAnimation { duration: 150 } }
        }

        Rectangle {
            id: scrollThumbH

            readonly property real scrollRange: 1.0 - plot.viewportRatio
            readonly property real progress: {
                if (scrollRange <= 0.0001) return 1.0
                return Math.max(0, Math.min(1, (plot.timelinePosition - plot.viewportRatio) / scrollRange))
            }

            x: {
                const restingW = 68
                const center = progress * (echoScrollBarH.width - restingW) + restingW / 2
                return Math.max(0, Math.min(echoScrollBarH.width - width, center - width / 2))
            }
            width: echoScrollBarH.thumbW
            height: parent.height
            radius: height / 2
            color: theme.controlBorderColor
            opacity: scrollMouseH.pressed ? 1.0 : 0.60

            Behavior on opacity { NumberAnimation { duration: 150 } }
            Behavior on width   { NumberAnimation { duration: 120; easing.type: Easing.OutCubic } }
        }

        NumberAnimation {
            id: snapToEndH
            target: plot
            property: "timelinePosition"
            to: 1.0
            duration: 380
            easing.type: Easing.OutCubic
            onStopped: updateOtherPlot(indx)
        }

        MouseArea {
            id: scrollMouseH
            anchors.fill: parent
            enabled: plot.scrollBarsShown || echoScrollBarH.trackVisible

            onPressed: function(mouse) {
                snapToEndH.stop()
                echoScrollBarH.grabOffsetX = mouse.x - scrollThumbH.x
                if (echoScrollBarH.grabOffsetX < 0 || echoScrollBarH.grabOffsetX > echoScrollBarH.thumbW)
                    echoScrollBarH.grabOffsetX = echoScrollBarH.thumbW / 2
            }
            onReleased: {
                if (plot.isLiveMode && scrollThumbH.progress > 0.85)
                    snapToEndH.start()
            }
            onCanceled: {
                if (plot.isLiveMode && scrollThumbH.progress > 0.85)
                    snapToEndH.start()
            }
            onPositionChanged: function(mouse) {
                if (!pressed) return
                const trackPx = echoScrollBarH.width - echoScrollBarH.thumbW
                if (trackPx <= 0) return
                const prog = Math.max(0, Math.min(1, (mouse.x - echoScrollBarH.grabOffsetX) / trackPx))
                plot.timelinePosition = prog * (1.0 - plot.viewportRatio) + plot.viewportRatio
                updateOtherPlot(indx)
            }
        }
    }

    // Vertical mode scroll bookmark (right edge)
    Item {
        id: echoScrollBarV

        property bool isScrolling: false
        property real grabOffsetY: 0
        property real prevTimelinePos: -1

        readonly property bool trackVisible: scrollMouseV.pressed || isScrolling
        readonly property real thumbH: (scrollMouseV.pressed || isScrolling) ? 96 : 68
        readonly property real thumbW: (scrollMouseV.pressed || isScrolling) ? 28 : 20

        visible: !plot.horizontal && plot.hasData
        opacity: (plot.scrollBarsShown || trackVisible) ? 1.0 : 0.0

        onTrackVisibleChanged: {
            if (trackVisible) {
                plot.scrollBarsShown = true
                scrollHideTimer.restart()
            }
        }

        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.topMargin: 6
        anchors.bottomMargin: 6
        anchors.rightMargin: 6
        width: thumbW
        z: 10

        Behavior on width    { NumberAnimation { duration: 120; easing.type: Easing.OutCubic } }
        Behavior on opacity  { NumberAnimation { duration: 400 } }

        Timer {
            id: scrollFadeV
            interval: 250
            repeat: false
            onTriggered: echoScrollBarV.isScrolling = false
        }

        Connections {
            target: plot
            function onTimelinePositionChanged() {
                if (scrollMouseV.pressed) return
                const pos = plot.timelinePosition
                if (Math.abs(pos - echoScrollBarV.prevTimelinePos) > 0.00005) {
                    echoScrollBarV.isScrolling = true
                    scrollFadeV.restart()
                }
                echoScrollBarV.prevTimelinePos = pos
            }
        }

        Rectangle {
            anchors.fill: parent
            radius: parent.width / 2
            color: Qt.rgba(0, 0, 0, 0.25)
            opacity: echoScrollBarV.trackVisible ? 1.0 : 0.0
            Behavior on opacity { NumberAnimation { duration: 200 } }
        }

        Rectangle {
            id: snapTrailV
            y: 0
            height: Math.max(0, scrollThumbV.y + scrollThumbV.height)
            width: parent.width
            radius: width / 2
            color: theme.controlBorderColor
            opacity: (scrollMouseV.pressed && plot.isLiveMode && scrollThumbV.progress > 0.85) ? 1.0 : 0.0
            Behavior on opacity { NumberAnimation { duration: 150 } }
        }

        Rectangle {
            id: ghostThumbV
            y: 0
            width: parent.width
            height: echoScrollBarV.thumbH
            radius: width / 2
            color: "transparent"
            border.color: theme.controlBorderColor
            border.width: 2
            opacity: (scrollMouseV.pressed && plot.isLiveMode && scrollThumbV.progress > 0.85) ? 0.65 : 0.0
            Behavior on opacity { NumberAnimation { duration: 150 } }
        }

        Rectangle {
            id: scrollThumbV

            readonly property real scrollRange: 1.0 - plot.viewportRatio
            readonly property real progress: {
                if (scrollRange <= 0.0001) return 1.0
                return Math.max(0, Math.min(1, (plot.timelinePosition - plot.viewportRatio) / scrollRange))
            }

            y: {
                const restingH = 68
                const vp = 1.0 - progress
                const center = vp * (echoScrollBarV.height - restingH) + restingH / 2
                return Math.max(0, Math.min(echoScrollBarV.height - height, center - height / 2))
            }
            width: parent.width
            height: echoScrollBarV.thumbH
            radius: width / 2
            color: theme.controlBorderColor
            opacity: scrollMouseV.pressed ? 1.0 : 0.60

            Behavior on opacity { NumberAnimation { duration: 150 } }
            Behavior on height  { NumberAnimation { duration: 120; easing.type: Easing.OutCubic } }
        }

        NumberAnimation {
            id: snapToEndV
            target: plot
            property: "timelinePosition"
            to: 1.0
            duration: 380
            easing.type: Easing.OutCubic
            onStopped: updateOtherPlot(indx)
        }

        MouseArea {
            id: scrollMouseV
            anchors.fill: parent
            enabled: plot.scrollBarsShown || echoScrollBarV.trackVisible

            onPressed: function(mouse) {
                snapToEndV.stop()
                echoScrollBarV.grabOffsetY = mouse.y - scrollThumbV.y
                if (echoScrollBarV.grabOffsetY < 0 || echoScrollBarV.grabOffsetY > echoScrollBarV.thumbH)
                    echoScrollBarV.grabOffsetY = echoScrollBarV.thumbH / 2
            }
            onReleased: {
                if (plot.isLiveMode && scrollThumbV.progress > 0.85)
                    snapToEndV.start()
            }
            onCanceled: {
                if (plot.isLiveMode && scrollThumbV.progress > 0.85)
                    snapToEndV.start()
            }
            onPositionChanged: function(mouse) {
                if (!pressed) return
                const trackPx = echoScrollBarV.height - echoScrollBarV.thumbH
                if (trackPx <= 0) return
                const visualProg = Math.max(0, Math.min(1, (mouse.y - echoScrollBarV.grabOffsetY) / trackPx))
                const prog = 1.0 - visualProg
                plot.timelinePosition = prog * (1.0 - plot.viewportRatio) + plot.viewportRatio
                updateOtherPlot(indx)
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
                contactDialog.x = plot.pointerContactMouseX
                contactDialog.y = plot.pointerContactMouseY
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
