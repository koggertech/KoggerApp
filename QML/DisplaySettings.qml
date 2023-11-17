import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Dialogs 1.2
import Qt.labs.settings 1.1

GridLayout {
    id: control

    property bool is2DHorizontal: horisontalVertical.checked
    property int instruments:  instrumentsGradeList.currentIndex

    property var targetPlot: null

    MenuBlock {
    }

    ColumnLayout {
        id: columnItem
        spacing: 24
        Layout.margins: 24

        ParamGroup {
            groupName: "Plot"

            RowLayout {
                id:rowDataset
                visible: instruments > 1
                CCombo  {
                    id: datasetCombo
                    Layout.fillWidth: true
//                    Layout.preferredWidth: columnItem.width/3
                    visible: true
                    onPressedChanged: {
                    }

                    Component.onCompleted: {
                        model = ["Dataset #1"]
                    }
                }

                CCombo  {
                    id: channel1Combo
//                    Layout.fillWidth: true
                    currentIndex: 1
                    Layout.preferredWidth: rowDataset.width/3
                    visible: true
                    onPressedChanged: {
                        if(pressed) {
                            model = dataset.channelsNameList()
                        }
                    }

                    Component.onCompleted: {
                        model = dataset.channelsNameList()
                        channel1Combo.currentIndex = 1
                    }

                    onCurrentTextChanged: {
                        var ch1 = channel1Combo.currentText != "None" ? channel1Combo.currentText != "First" ? channel1Combo.currentText : 32767 : 32768
                        var ch2 = channel2Combo.currentText != "None" ? channel2Combo.currentText != "First" ? channel2Combo.currentText : 32767 : 32768

                        targetPlot.plotDatasetChannel(ch1, ch2)
                    }
                }

                CCombo  {
                    id: channel2Combo
//                    Layout.fillWidth: true
                    currentIndex: 0
                    Layout.preferredWidth: rowDataset.width/3
                    visible: true
                    onPressedChanged: {
                        if(pressed) {
                            model = dataset.channelsNameList()
                        }
                    }

                    Component.onCompleted: {
                        model = dataset.channelsNameList()
                    }

                    onCurrentTextChanged: {
                        var ch1 = channel1Combo.currentText != "None" ? channel1Combo.currentText != "First" ? channel1Combo.currentText : 32767 : 32768
                        var ch2 = channel2Combo.currentText != "None" ? channel2Combo.currentText != "First" ? channel2Combo.currentText : 32767 : 32768

                        targetPlot.plotDatasetChannel(ch1, ch2)
                    }
                }
            }

            RowLayout {
                CCheck {
                    id: echogramVisible
                    Layout.fillWidth: true
                    //                        Layout.preferredWidth: 150
                    checked: true
                    text: "Echogram"
                    onCheckedChanged: targetPlot.plotEchogramVisible(checked)
                    Component.onCompleted: targetPlot.plotEchogramVisible(checked)
                }

                CCombo  {
                    id: echoTheme
                    //                        Layout.fillWidth: true
                    Layout.preferredWidth: 150
                    model: ["Blue", "Sepia", "WRGBD", "WhiteBlack", "BlackWhite"]
                    currentIndex: 0

                    onCurrentIndexChanged: targetPlot.plotEchogramTheme(currentIndex)
                    Component.onCompleted: targetPlot.plotEchogramTheme(currentIndex)

                    Settings {
                        property alias waterfallThemeId: echoTheme.currentIndex
                    }
                }

                CCombo  {
                    id: echogramTypesList
                    //                        Layout.fillWidth: true
                    Layout.preferredWidth: 150
                    model: ["Raw", "Side-Scan"]
                    currentIndex: 0

                    onCurrentIndexChanged: targetPlot.plotEchogramType(currentIndex)
                    Component.onCompleted: targetPlot.plotEchogramType(currentIndex)

                    Settings {
                        property alias echogramTypesList: echogramTypesList.currentIndex
                    }
                }
            }

            RowLayout {
                visible: instruments > 0
                CCheck {
                    id: bottomTrackVisible
                    Layout.fillWidth: true
                    text: "Bottom-Track"
                    onCheckedChanged: targetPlot.plotBottomTrackVisible(checked)
                    Component.onCompleted: targetPlot.plotBottomTrackVisible(checked)
                }

                CCombo  {
                    id: bottomTrackThemeList
                    //                        Layout.fillWidth: true
                    //                        Layout.preferredWidth: 150
                    model: ["Line1", "Line2", "Dot1", "Dot2", "DotLine"]
                    currentIndex: 1

                    onCurrentIndexChanged: targetPlot.plotBottomTrackTheme(currentIndex)
                    Component.onCompleted: targetPlot.plotBottomTrackTheme(currentIndex)

                    Settings {
                        property alias bottomTrackThemeList: bottomTrackThemeList.currentIndex
                    }
                }
            }

            RowLayout {
                CCheck {
                    id: rangefinderVisible
                    Layout.fillWidth: true
                    text: "Rangefinder"
                    onCheckedChanged: targetPlot.plotRangefinderVisible(checked)
                    Component.onCompleted: targetPlot.plotRangefinderVisible(checked)
                }

                CCombo  {
                    id: rangefinderThemeList
                    model: ["Text", "Line", "Dot"]
                    currentIndex: 1

                    onCurrentIndexChanged: targetPlot.plotRangefinderTheme(currentIndex)
                    Component.onCompleted: targetPlot.plotRangefinderTheme(currentIndex)

                    Settings {
                        property alias rangefinderThemeList: rangefinderThemeList.currentIndex
                    }
                }
            }


            CCheck {
                visible: instruments > 1
                id: ahrsVisible
                text: "Attitude"
                onCheckedChanged: targetPlot.plotAttitudeVisible(checked)
                Component.onCompleted: targetPlot.plotAttitudeVisible(checked)
            }

            RowLayout {
                visible: instruments > 1
                id: dopplerBeamVisibleGroup
                spacing: 0
                function updateDopplerBeamVisible() {
                    var beamfilter = dopplerBeam1Visible.checked*1 + dopplerBeam2Visible.checked*2 + dopplerBeam3Visible.checked*4 + dopplerBeam4Visible.checked*8
                    targetPlot.plotDopplerBeamVisible(dopplerBeamVisible.checked,
                                           beamfilter)
                }

                CCheck {
                    id: dopplerBeamVisible
                    Layout.fillWidth: true
                    text: "Doppler Beams"
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
                    text: "Doppler Instrument"
                    onCheckedChanged: targetPlot.plotDopplerInstrumentVisible(checked)
                    Component.onCompleted: targetPlot.plotDopplerInstrumentVisible(checked)
                }

                CCheck {
                    id: dopplerInstrumentXVisible
                    enabled: false
                    checked: true
                    text: "X"
                    //                        onCheckedChanged: targetPlot.setDopplerInstrumentVis(checked)
                    //                        Component.onCompleted: targetPlot.setDopplerInstrumentVis(checked)
                }

                CCheck {
                    id: dopplerInstrumentYVisible
                    enabled: false
                    checked: true
                    text: "Y"
                    //                        onCheckedChanged: targetPlot.setDopplerInstrumentVis(checked)
                    //                        Component.onCompleted: targetPlot.setDopplerInstrumentVis(checked)
                }

                CCheck {
                    id: dopplerInstrumentZVisible
                    enabled: false
                    checked: true
                    text: "Z"
                    //                        onCheckedChanged: targetPlot.setDopplerInstrumentVis(checked)
                    //                        Component.onCompleted: targetPlot.setDopplerInstrumentVis(checked)
                }
            }

            RowLayout {
                visible: instruments > 1
                CCheck {
                    id: adcpVisible
                    enabled: false
                    Layout.fillWidth: true
                    text: "Doppler Profiler"
                }
            }

            RowLayout {
                visible: instruments > 1
                CCheck {
                    id: gnssVisible
                    checked: true
                    Layout.fillWidth: true
                    text: "GNSS data"

                    onCheckedChanged: targetPlot.plotGNSSVisible(checked, 1)
                    Component.onCompleted: targetPlot.plotGNSSVisible(checked, 1)
                }

//                CCheck {
//                    id: gnssHSpeedVisible
//                    checked: true
//                    text: "Hor. Speed"
//                    //                        onCheckedChanged: targetPlot.setDopplerInstrumentVis(checked)
//                    //                        Component.onCompleted: targetPlot.setDopplerInstrumentVis(checked)
//                }
            }



            RowLayout {
                CCheck {
                    id: gridVisible
                    Layout.fillWidth: true
                    text: "Grid"
                    onCheckedChanged: targetPlot.plotGridVerticalNumber(gridNumber.value*gridVisible.checked)
                }

                SpinBoxCustom {
                    id: gridNumber
                    from: 1
                    to: 20
                    stepSize: 1
                    value: 5

                    onValueChanged: targetPlot.plotGridVerticalNumber(gridNumber.value*gridVisible.checked)
                    Component.onCompleted: targetPlot.plotGridVerticalNumber(gridNumber.value*gridVisible.checked)

                    Settings {
                        property alias gridNumber: gridNumber.value
                    }
                }
            }

            RowLayout {
                visible: instruments > 1
                CCheck {
                    id: velocityVisible
                    Layout.fillWidth: true
                    text: "Velocity range, m/s"
                    onCheckedChanged: targetPlot.plotVelocityVisible(checked)
                    Component.onCompleted: targetPlot.plotVelocityVisible(checked)

                    Settings {
                        property alias velocityVisible: velocityVisible.checked
                    }
                }

                SpinBoxCustom {
                    id: velocityRange
                    from: 500
                    to: 1000*8
                    stepSize: 500
                    value: 5

                    onValueChanged: targetPlot.plotVelocityRange(velocityRange.realValue)
                    Component.onCompleted: targetPlot.plotVelocityRange(velocityRange.realValue)

                    property int decimals: 1
                    property real realValue: value / 1000

                    validator: DoubleValidator {
                        bottom: Math.min(from, to)
                        top:  Math.max(from, to)
                    }

                    textFromValue: function(value, locale) {
                        return Number(value / 1000).toLocaleString(locale, 'f', decimals)
                    }

                    valueFromText: function(text, locale) {
                        return Number.fromLocaleString(locale, text) * 1000
                    }

                    onRealValueChanged: targetPlot.plotVelocityRange(realValue)

                    Settings {
                        property alias velocityRange: velocityRange.value
                    }
                }
            }

            RowLayout {
                id: distanceAutoRangeRow
                function distanceAutorangeMode() {
                    targetPlot.plotDistanceAutoRange(distanceAutoRange.checked ? distanceAutoRangeList.currentIndex : -1)
                }

                CCheck {
                    id: distanceAutoRange
                    checked: true
                    Layout.fillWidth: true
                    text: "Distance auto range"

                    onCheckedChanged: {
                        distanceAutoRangeRow.distanceAutorangeMode()
                    }
                    Component.onCompleted: distanceAutoRangeRow.distanceAutorangeMode()

                    Settings {
                        property alias distanceAutoRange: distanceAutoRange.checked
                    }
                }

                CCombo  {
                    id: distanceAutoRangeList
                    model: ["Last data       ", "Last on screen", "Max on screen"]
                    currentIndex: 0
                    onCurrentIndexChanged: distanceAutoRangeRow.distanceAutorangeMode()
                    Component.onCompleted: distanceAutoRangeRow.distanceAutorangeMode()

                    Settings {
                        property alias distanceAutoRangeList: distanceAutoRangeList.currentIndex
                    }
                }
            }

            CCheck {
                id: horisontalVertical
                checked: true
                text: "Horisontal"
            }

            Settings {
                property alias echogramVisible: echogramVisible.checked
                property alias rangefinderVisible: rangefinderVisible.checked
                property alias postProcVisible: bottomTrackVisible.checked
                property alias ahrsVisible: ahrsVisible.checked

                property alias gridVisible: gridVisible.checked


                property alias dopplerBeamVisible: dopplerBeamVisible.checked
                property alias dopplerInstrumentVisible: dopplerInstrumentVisible.checked

                property alias horisontalVertical: horisontalVertical.checked
            }


        }

        ParamGroup {
            visible: instruments > 0
            id: bottomTrackProcessingGroup
            groupName: "Bottom-Track processing"

            property bool autoApplyChange: false

            function updateProcessing() {
                targetPlot.doDistProcessing(bottomTrackList.currentIndex,
                                            bottomTrackWindow.checked ? bottomTrackWindowValue.value : 1,
                                            bottomTrackVerticalGap.checked ? bottomTrackVerticalGapValue.value*0.01 : 0,
                                            bottomTrackMinRange.checked ? bottomTrackMinRangeValue.realValue : 0,
                                            bottomTrackMaxRange.checked ? bottomTrackMaxRangeValue.realValue : 1000,
                                            bottomTrackGainSlope.checked ? bottomTrackGainSlopeValue.realValue : 1,
                                            bottomTrackThreshold.checked ? bottomTrackThresholdValue.realValue : 0,
                                            bottomTrackSensorOffset.checked ? bottomTrackSensorOffsetValueX.value*0.001 : 0,
                                            bottomTrackSensorOffset.checked ? bottomTrackSensorOffsetValueY.value*0.001 : 0,
                                            bottomTrackSensorOffset.checked ? -bottomTrackSensorOffsetValueZ.value*0.001 : 0
                                            );
            }

            RowLayout {
                ParamSetup {
                    paramName: "Preset:"

                    CCombo  {
                        id: bottomTrackList
                        //                        Layout.fillWidth: true
                        Layout.preferredWidth: 250
                        model: ["Normal 2D", "Narrow 2D", "Echogram Side-Scan"]
                        currentIndex: 0

//                        onCurrentIndexChanged: bottomTrackProcessingGroup.updateProcessing()

                        Settings {
                            property alias bottomTrackList: bottomTrackList.currentIndex
                        }
                    }
                }
            }

            RowLayout {
                CCheck {
                    id: bottomTrackGainSlope
                    Layout.fillWidth: true
                    text: "Gain slope:"

                    Settings {
                        property alias bottomTrackGainSlope: bottomTrackGainSlope.checked
                    }
                }

                SpinBoxCustom {
                    id: bottomTrackGainSlopeValue
                    from: 0
                    to: 300
                    stepSize: 10
                    value: 100

                    property int decimals: 2
                    property real realValue: value / 100

                    validator: DoubleValidator {
                        bottom: Math.min(from, to)
                        top:  Math.max(from, to)
                    }

                    textFromValue: function(value, locale) {
                        return Number(value / 100).toLocaleString(locale, 'f', decimals)
                    }

                    valueFromText: function(text, locale) {
                        return Number.fromLocaleString(locale, text) * 100
                    }

                    Settings {
                        property alias bottomTrackGainSlopeValue: bottomTrackGainSlopeValue.value
                    }
                }
            }

            RowLayout {
                CCheck {
                    id: bottomTrackThreshold
                    Layout.fillWidth: true
                    text: "Threshold:"

                    Settings {
                        property alias bottomTrackThreshold: bottomTrackThreshold.checked
                    }
                }

                SpinBoxCustom {
                    id: bottomTrackThresholdValue
                    from: 0
                    to: 200
                    stepSize: 5
                    value: 0

                    property int decimals: 2
                    property real realValue: value / 100

                    validator: DoubleValidator {
                        bottom: Math.min(from, to)
                        top:  Math.max(from, to)
                    }

                    textFromValue: function(value, locale) {
                        return Number(value / 100).toLocaleString(locale, 'f', decimals)
                    }

                    valueFromText: function(text, locale) {
                        return Number.fromLocaleString(locale, text) * 100
                    }

                    Settings {
                        property alias bottomTrackThresholdValue: bottomTrackThresholdValue.value
                    }
                }
            }

            RowLayout {
                CCheck {
                    id: bottomTrackWindow
                    Layout.fillWidth: true
                    text: "Horizontal window:"

                    Settings {
                        property alias bottomTrackWindow: bottomTrackWindow.checked
                    }
                }

                SpinBoxCustom {
                    id: bottomTrackWindowValue
                    from: 1
                    to: 100
                    stepSize: 2
                    value: 1

                    Settings {
                        property alias bottomTrackWindowValue: bottomTrackWindowValue.value
                    }
                }
            }


            RowLayout {
                CCheck {
                    id: bottomTrackVerticalGap
                    Layout.fillWidth: true
                    text: "Vertical gap, %:"
//                    onCheckedChanged: bottomTrackProcessingGroup.updateProcessing()
                    Settings {
                        property alias bottomTrackVerticalGap: bottomTrackVerticalGap.checked
                    }
                }

                SpinBoxCustom {
                    id: bottomTrackVerticalGapValue
                    from: 0
                    to: 100
                    stepSize: 2
                    value: 10
//                    onValueChanged: bottomTrackProcessingGroup.updateProcessing()
                    Settings {
                        property alias bottomTrackVerticalGapValue: bottomTrackVerticalGapValue.value
                    }
                }
            }

            RowLayout {
                CCheck {
                    id: bottomTrackMinRange
                    Layout.fillWidth: true
                    text: "Min range, m:"
//                    onCheckedChanged: bottomTrackProcessingGroup.updateProcessing()
                    Settings {
                        property alias bottomTrackMinRange: bottomTrackMinRange.checked
                    }
                }


                SpinBoxCustom {
                    id: bottomTrackMinRangeValue
                    from: 0
                    to: 200000
                    stepSize: 10
                    value: 0

                    property int decimals: 2
                    property real realValue: value / 1000

                    validator: DoubleValidator {
                        bottom: Math.min(from, to)
                        top:  Math.max(from, to)
                    }

                    textFromValue: function(value, locale) {
                        return Number(value / 1000).toLocaleString(locale, 'f', decimals)
                    }

                    valueFromText: function(text, locale) {
                        return Number.fromLocaleString(locale, text) * 1000
                    }

//                    onRealValueChanged: bottomTrackProcessingGroup.updateProcessing()
                    Settings {
                        property alias bottomTrackMinRangeValue: bottomTrackMinRangeValue.value
                    }
                }
            }

            RowLayout {
                CCheck {
                    id: bottomTrackMaxRange
                    Layout.fillWidth: true
                    text: "Max range, m:"
//                    onCheckedChanged: bottomTrackProcessingGroup.updateProcessing()
                    Settings {
                        property alias bottomTrackMaxRange: bottomTrackMaxRange.checked
                    }
                }

                SpinBoxCustom {
                    id: bottomTrackMaxRangeValue
                    from: 0
                    to: 200000
                    stepSize: 1000
                    value: 100000

                    property int decimals: 2
                    property real realValue: value / 1000

                    validator: DoubleValidator {
                        bottom: Math.min(from, to)
                        top:  Math.max(from, to)
                    }

                    textFromValue: function(value, locale) {
                        return Number(value / 1000).toLocaleString(locale, 'f', decimals)
                    }

                    valueFromText: function(text, locale) {
                        return Number.fromLocaleString(locale, text) * 1000
                    }

//                    onRealValueChanged: bottomTrackProcessingGroup.updateProcessing()

                    Settings {
                        property alias bottomTrackMaxRangeValue: bottomTrackMaxRangeValue.value
                    }
                }
            }

            RowLayout {
                CCheck {
                    id: bottomTrackSensorOffset
                    Layout.fillWidth: true
                    text: "Sonar offset XYZ, mm:"
//                    onCheckedChanged: bottomTrackProcessingGroup.updateProcessing()
                    Settings {
                        property alias bottomTrackSensorOffset: bottomTrackSensorOffset.checked
                    }
                }

                SpinBoxCustom {
                    id: bottomTrackSensorOffsetValueX
                    spinner: false
                    implicitWidth: 65
                    from: -9999
                    to: 9999
                    stepSize: 50

//                    onRealValueChanged: bottomTrackProcessingGroup.updateProcessing()

                    Settings {
                        property alias bottomTrackSensorOffsetValueX: bottomTrackSensorOffsetValueX.value
                    }
                }

                SpinBoxCustom {
                    id: bottomTrackSensorOffsetValueY
                    spinner: false
                    implicitWidth: 65
                    from: -9999
                    to: 9999
                    stepSize: 50

//                    onRealValueChanged: bottomTrackProcessingGroup.updateProcessing()

                    Settings {
                        property alias bottomTrackSensorOffsetValueY: bottomTrackSensorOffsetValueY.value
                    }
                }

                SpinBoxCustom {
                    id: bottomTrackSensorOffsetValueZ
                    spinner: false
                    implicitWidth: 65
                    from: -9999
                    to: 9999
                    stepSize: 50

//                    onRealValueChanged: bottomTrackProcessingGroup.updateProcessing()

                    Settings {
                        property alias bottomTrackSensorOffsetValueZ: bottomTrackSensorOffsetValueZ.value
                    }
                }
            }

            RowLayout {
                CButton {
                    text: "Processing"
                    Layout.fillWidth: true
                    onClicked: bottomTrackProcessingGroup.updateProcessing()
                }
            }
        }

        ParamGroup {
            visible: instruments > 0
            groupName: "Export"

            ColumnLayout {
                RowLayout {
                    CTextField {
                        id: exportPathText
                        hoverEnabled: true
                        Layout.fillWidth: true

                        text: shortcuts.home

                        placeholderText: qsTr("Enter path")
                        Keys.onPressed: {
                        }
                    }


                    CButton {
                        text: "..."
                        Layout.fillWidth: false
                        onClicked: exportFileDialog.open()
                    }

                    FileDialog {
                        id: exportFileDialog
                        folder: shortcuts.home
                        selectExisting: true
                        selectFolder: true

                        onAccepted: {
                            exportPathText.text = exportFileDialog.folder.toString()
                        }

                        onRejected: { }
                    }

                    Settings {
                        property alias exportFolder: exportFileDialog.folder
                    }

                    Settings {
                        property alias exportFolderText: exportPathText.text
                    }
                }

                RowLayout {
                    CCheck {
                        id: exportDecimation
                        Layout.fillWidth: true
                        text: "Decimation, m:"
    //                    onCheckedChanged: bottomTrackProcessingGroup.updateProcessing()
                        Settings {
                            property alias exportDecimation: exportDecimation.checked
                        }
                    }

                    SpinBoxCustom {
                        id: exportDecimationValue
                        implicitWidth: 100
                        from: 0
                        to: 100
                        stepSize: 1
                        value: 10
    //                    onValueChanged: bottomTrackProcessingGroup.updateProcessing()
                        Settings {
                            property alias exportDecimationValue: exportDecimationValue.value
                        }
                    }

                    CButton {
                        text: "Export to CSV"
                        Layout.fillWidth: true
                        onClicked: core.exportPlotAsCVS(exportPathText.text, targetPlot.plotDatasetChannel(), exportDecimation.checked ? exportDecimationValue.value : 0);
                    }
                }

                RowLayout {
                    CButton {
                        text: "Export to XTF"
                        Layout.fillWidth: true
                        onClicked: core.exportPlotAsXTF(exportPathText.text);
                    }
                }

            }
        }

        ParamGroup {
            groupName: "Preference"

            ParamSetup {
                paramName: "Display theme:"

                CCombo  {
                    id: appTheme
                    Layout.fillWidth: true
                    model: ["Dark", "Super Dark", "Light", "Super Light"]
                    currentIndex: 0

                    onCurrentIndexChanged: theme.themeID = currentIndex
                    Component.onCompleted: theme.themeID = currentIndex

                    Settings {
                        property alias appTheme: appTheme.currentIndex
                    }
                }
            }

            ParamSetup {
                paramName: "Instrumets grade:"

                CCombo  {
                    id: instrumentsGradeList
                    Layout.fillWidth: true
                    model: ["Fish Finders", "Bottom Tracking", "Maximum"]
                    currentIndex: 0

                    Settings {
                        property alias instrumentsGradeList: instrumentsGradeList.currentIndex
                    }
                }
            }
        }

        ParamGroup {
            visible: instruments > 1
            groupName: "Interface"

            CCheck {
                id: consoleVisible
                text: "Console"

                onCheckedChanged: theme.consoleVisible = checked
                Component.onCompleted: theme.consoleVisible = checked

                Settings {
                    property alias consoleVisible: consoleVisible.checked
                }
            }
        }
    }
}
