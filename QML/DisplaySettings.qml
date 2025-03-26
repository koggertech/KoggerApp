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

    signal languageChanged(string langStr)

    ColumnLayout {
        id: columnItem
        spacing: 24
        Layout.margins: 10

        ParamGroup {
            groupName: qsTr("Plot")

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
                        model = [qsTr("Dataset #1")]
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
                        var ch1 = channel1Combo.currentText !== qsTr("None") ? channel1Combo.currentText !== qsTr("First") ? channel1Combo.currentText : 32767 : 32768
                        var ch2 = channel2Combo.currentText !== qsTr("None") ? channel2Combo.currentText !== qsTr("First") ? channel2Combo.currentText : 32767 : 32768

                        targetPlot.plotDatasetChannel(ch1, ch2)
                        core.setSideScanChannels(ch1, ch2);
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
                        var ch1 = channel1Combo.currentText !== qsTr("None") ? channel1Combo.currentText !== qsTr("First") ? channel1Combo.currentText : 32767 : 32768
                        var ch2 = channel2Combo.currentText !== qsTr("None") ? channel2Combo.currentText !== qsTr("First") ? channel2Combo.currentText : 32767 : 32768

                        targetPlot.plotDatasetChannel(ch1, ch2)
                        core.setSideScanChannels(ch1, ch2);
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
                    onCheckedChanged: targetPlot.plotEchogramVisible(checked)
                    Component.onCompleted: targetPlot.plotEchogramVisible(checked)
                }

                CCombo  {
                    id: echoTheme
                    //                        Layout.fillWidth: true
                    Layout.preferredWidth: 150
                    model: [qsTr("Blue"), qsTr("Sepia"), qsTr("WRGBD"), qsTr("WhiteBlack"), qsTr("BlackWhite")]
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
                    model: [qsTr("Raw"), qsTr("Side-Scan")]
                    currentIndex: 0

                    onCurrentIndexChanged: targetPlot.plotEchogramCompensation(currentIndex) // TODO
                    Component.onCompleted: targetPlot.plotEchogramCompensation(currentIndex) // TODO

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
                    text: qsTr("Bottom-Track")
                    onCheckedChanged: targetPlot.plotBottomTrackVisible(checked)
                    Component.onCompleted: targetPlot.plotBottomTrackVisible(checked)
                }

                CCombo  {
                    id: bottomTrackThemeList
                    //                        Layout.fillWidth: true
                    //                        Layout.preferredWidth: 150
                    model: [qsTr("Line1"), qsTr("Line2"), qsTr("Dot1"), qsTr("Dot2"), qsTr("DotLine")]
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
                    text: qsTr("Rangefinder")
                    onCheckedChanged: targetPlot.plotRangefinderVisible(checked)
                    Component.onCompleted: targetPlot.plotRangefinderVisible(checked)
                }

                CCombo  {
                    id: rangefinderThemeList
                    model: [qsTr("Text"), qsTr("Line"), qsTr("Dot")]
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
                text: qsTr("Attitude")
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

                    onCheckedChanged: targetPlot.plotGNSSVisible(checked, 1)
                    Component.onCompleted: targetPlot.plotGNSSVisible(checked, 1)

                    Settings {
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
                        onCheckedChanged: targetPlot.plotGridVerticalNumber(gridNumber.value*gridVisible.checked)
                    }
                    CCheck {
                        id: fillWidthGrid
                        Layout.fillWidth: true
                        text: qsTr("fill")
                        onCheckedChanged: targetPlot.plotGridFillWidth(checked)
                        visible: gridVisible.checked

                        Component.onCompleted: {
                            targetPlot.plotGridFillWidth(checked)
                        }
                        Settings {
                            property alias fillWidthGrid: fillWidthGrid.checked
                        }
                    }
                }

                SpinBoxCustom {
                    id: gridNumber
                    from: 1
                    to: 24
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
                    id: angleVisible
                    Layout.fillWidth: true
                    text: qsTr("Angle range, °")
                    onCheckedChanged: targetPlot.plotAngleVisibility(checked)
                    Component.onCompleted: targetPlot.plotAngleVisibility(checked)

                    Settings {
                        property alias angleVisible: angleVisible.checked
                    }
                }

                SpinBoxCustom {
                    id: angleRange
                    from: 1
                    to: 360
                    stepSize: 1
                    value: 45

                    onValueChanged: targetPlot.plotAngleRange(angleRange.currValue)
                    Component.onCompleted: targetPlot.plotAngleRange(angleRange.currValue)

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

                    onCurrValueChanged: targetPlot.plotAngleRange(currValue)

                    Settings {
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
                        bottom: Math.min(velocityRange.from, velocityRange.to)
                        top:  Math.max(velocityRange.from, velocityRange.to)
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
                    text: qsTr("Distance auto range")

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
                    model: [qsTr("Last data       "), qsTr("Last on screen"), qsTr("Max on screen")]
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
                text: qsTr("Horizontal")
            }

            RowLayout {
                visible: instruments > 1

                CCheck {
                    id: fixBlackStripesCheckButton
                    Layout.fillWidth: true
                    checked: false
                    text: qsTr("Fix black stripes, window")

                    onCheckedChanged: core.fixBlackStripesState = fixBlackStripesCheckButton.checked
                    Component.onCompleted: core.fixBlackStripesState = fixBlackStripesCheckButton.checked

                    Settings {
                        property alias fixBlackStripesCheckButton: fixBlackStripesCheckButton.checked
                    }
                }

                SpinBoxCustom {
                    id: fixBlackStripesSpinBox
                    from: 1
                    to: 100
                    stepSize: 1
                    value: 5

                    onValueChanged: core.fixBlackStripesRange = fixBlackStripesSpinBox.currValue
                    Component.onCompleted: core.fixBlackStripesRange = fixBlackStripesSpinBox.currValue

                    property int currValue: value

                    validator: DoubleValidator {
                        bottom: Math.min(fixBlackStripesSpinBox.from, fixBlackStripesSpinBox.to)
                        top:  Math.max(fixBlackStripesSpinBox.from, fixBlackStripesSpinBox.to)
                    }

                    textFromValue: function(value, locale) {
                        return Number(value).toLocaleString(locale, 'f', 0)
                    }

                    valueFromText: function(text, locale) {
                        return Number.fromLocaleString(locale, text)
                    }

                    onCurrValueChanged: core.fixBlackStripesRange = currValue

                    Settings {
                        property alias fixBlackStripesSpinBox: fixBlackStripesSpinBox.value
                    }
                }
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
            groupName: qsTr("Bottom-Track processing")

            property bool autoApplyChange: false

            Component.onCompleted: {
                targetPlot.refreshDistParams(bottomTrackList.currentIndex,
                                             bottomTrackWindow.checked ? bottomTrackWindowValue.value : 1,
                                             bottomTrackVerticalGap.checked ? bottomTrackVerticalGapValue.value* 0.01 : 0,
                                             bottomTrackMinRange.checked ? bottomTrackMinRangeValue.realValue : 0,
                                             bottomTrackMaxRange.checked ? bottomTrackMaxRangeValue.realValue : 1000,
                                             bottomTrackGainSlope.checked ? bottomTrackGainSlopeValue.realValue : 1,
                                             bottomTrackThreshold.checked ? bottomTrackThresholdValue.realValue : 0,
                                             bottomTrackSensorOffset.checked ? bottomTrackSensorOffsetValueX.value *  0.001 : 0,
                                             bottomTrackSensorOffset.checked ? bottomTrackSensorOffsetValueY.value *  0.001 : 0,
                                             bottomTrackSensorOffset.checked ? bottomTrackSensorOffsetValueZ.value * -0.001 : 0)
            }

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
                    paramName: qsTr("Preset:")

                    CCombo  {
                        id: bottomTrackList
                        //                        Layout.fillWidth: true
                        Layout.preferredWidth: 250
                        model: [qsTr("Normal 2D"), qsTr("Narrow 2D"), qsTr("Echogram Side-Scan")]
                        currentIndex: 0

//                        onCurrentIndexChanged: bottomTrackProcessingGroup.updateProcessing()

                        onCurrentIndexChanged: {
                            targetPlot.setPreset(bottomTrackList.currentIndex)
                        }

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
                    text: qsTr("Gain slope:")

                    onCheckedChanged: {
                        if (checked) {
                            targetPlot.setGainSlope(bottomTrackGainSlopeValue.realValue)
                        }
                    }

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
                        bottom: Math.min(bottomTrackGainSlopeValue.from, bottomTrackGainSlopeValue.to)
                        top:  Math.max(bottomTrackGainSlopeValue.from, bottomTrackGainSlopeValue.to)
                    }

                    textFromValue: function(value, locale) {
                        return Number(value / 100).toLocaleString(locale, 'f', decimals)
                    }

                    valueFromText: function(text, locale) {
                        return Number.fromLocaleString(locale, text) * 100
                    }

                    onRealValueChanged: {
                        if (bottomTrackGainSlope.checked) {
                            targetPlot.setGainSlope(bottomTrackGainSlopeValue.realValue)
                        }
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
                    text: qsTr("Threshold:")

                    onCheckedChanged: {
                        if (checked) {
                            targetPlot.setThreshold(bottomTrackThresholdValue.realValue)
                        }
                    }

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
                        bottom: Math.min(bottomTrackThresholdValue.from, bottomTrackThresholdValue.to)
                        top:  Math.max(bottomTrackThresholdValue.from, bottomTrackThresholdValue.to)
                    }

                    textFromValue: function(value, locale) {
                        return Number(value / 100).toLocaleString(locale, 'f', decimals)
                    }

                    valueFromText: function(text, locale) {
                        return Number.fromLocaleString(locale, text) * 100
                    }

                    onRealValueChanged: {
                        if (bottomTrackThreshold.checked) {
                            targetPlot.setThreshold(bottomTrackThresholdValue.realValue)
                        }
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
                    text: qsTr("Horizontal window:")

                    onCheckedChanged: {
                        if (checked) {
                            targetPlot.setWindowSize(bottomTrackWindowValue.value)
                        }
                    }

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

                    onValueChanged: {
                        if (bottomTrackWindow.checked) {
                            targetPlot.setWindowSize(bottomTrackWindowValue.value)
                        }
                    }

                    Settings {
                        property alias bottomTrackWindowValue: bottomTrackWindowValue.value
                    }
                }
            }


            RowLayout {
                CCheck {
                    id: bottomTrackVerticalGap
                    Layout.fillWidth: true
                    text: qsTr("Vertical gap, %:")
//                    onCheckedChanged: bottomTrackProcessingGroup.updateProcessing()

                    onCheckedChanged: {
                        if (checked) {
                            targetPlot.setVerticalGap(bottomTrackVerticalGapValue.value * 0.01)
                        }
                    }

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

                    onValueChanged: {
                        if (bottomTrackVerticalGap.checked) {
                            targetPlot.setVerticalGap(bottomTrackVerticalGapValue.value * 0.01)
                        }
                    }

                    Settings {
                        property alias bottomTrackVerticalGapValue: bottomTrackVerticalGapValue.value
                    }
                }
            }

            RowLayout {
                CCheck {
                    id: bottomTrackMinRange
                    Layout.fillWidth: true
                    text: qsTr("Min range, m:")
//                    onCheckedChanged: bottomTrackProcessingGroup.updateProcessing()

                    onCheckedChanged: {
                        if (checked) {
                            targetPlot.setRangeMin(bottomTrackMinRangeValue.realValue)
                        }
                    }

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
                        bottom: Math.min(bottomTrackMinRangeValue.from, bottomTrackMinRangeValue.to)
                        top:  Math.max(bottomTrackMinRangeValue.from, bottomTrackMinRangeValue.to)
                    }

                    textFromValue: function(value, locale) {
                        return Number(value / 1000).toLocaleString(locale, 'f', decimals)
                    }

                    valueFromText: function(text, locale) {
                        return Number.fromLocaleString(locale, text) * 1000
                    }

//                    onRealValueChanged: bottomTrackProcessingGroup.updateProcessing()

                    onRealValueChanged: {
                        if (bottomTrackMinRange.checked) {
                            targetPlot.setRangeMin(bottomTrackMinRangeValue.realValue)
                        }
                    }

                    Settings {
                        property alias bottomTrackMinRangeValue: bottomTrackMinRangeValue.value
                    }
                }
            }

            RowLayout {
                CCheck {
                    id: bottomTrackMaxRange
                    Layout.fillWidth: true
                    text: qsTr("Max range, m:")
//                    onCheckedChanged: bottomTrackProcessingGroup.updateProcessing()

                    onCheckedChanged: {
                        if (checked) {
                            targetPlot.setRangeMax(bottomTrackMaxRangeValue.realValue)
                        }
                    }

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
                        bottom: Math.min(bottomTrackMaxRangeValue.from, bottomTrackMaxRangeValue.to)
                        top:  Math.max(bottomTrackMaxRangeValue.from, bottomTrackMaxRangeValue.to)
                    }

                    textFromValue: function(value, locale) {
                        return Number(value / 1000).toLocaleString(locale, 'f', decimals)
                    }

                    valueFromText: function(text, locale) {
                        return Number.fromLocaleString(locale, text) * 1000
                    }

//                    onRealValueChanged: bottomTrackProcessingGroup.updateProcessing()

                    onRealValueChanged: {
                        if (bottomTrackMaxRange.checked) {
                            targetPlot.setRangeMax(bottomTrackMaxRangeValue.realValue)
                        }
                    }

                    Settings {
                        property alias bottomTrackMaxRangeValue: bottomTrackMaxRangeValue.value
                    }
                }
            }

            RowLayout {
                CCheck {
                    id: bottomTrackSensorOffset
                    Layout.fillWidth: true
                    text: qsTr("Sonar offset XYZ, mm:")
//                    onCheckedChanged: bottomTrackProcessingGroup.updateProcessing()

                    onCheckedChanged: {
                        if (checked) {
                            targetPlot.setOffsetX(bottomTrackSensorOffsetValueX.value * 0.001)
                            targetPlot.setOffsetY(bottomTrackSensorOffsetValueY.value * 0.001)
                            targetPlot.setOffsetZ(bottomTrackSensorOffsetValueZ.value * 0.001)
                        }
                    }

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

                    onValueChanged: {
                        if (bottomTrackSensorOffset.checked) {
                            targetPlot.setOffsetX(bottomTrackSensorOffsetValueX.value * 0.001)
                        }
                    }

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

                    onValueChanged: {
                        if (bottomTrackSensorOffset.checked) {
                            targetPlot.setOffsetY(bottomTrackSensorOffsetValueY.value * 0.001)
                        }
                    }

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

                    onValueChanged: {
                        if (bottomTrackSensorOffset.checked) {
                            targetPlot.setOffsetZ(bottomTrackSensorOffsetValueZ.value * 0.001)
                        }
                    }

                    Settings {
                        property alias bottomTrackSensorOffsetValueZ: bottomTrackSensorOffsetValueZ.value
                    }
                }
            }

            RowLayout {
                CButton {
                    text: qsTr("Processing")
                    Layout.fillWidth: true
                    onClicked: bottomTrackProcessingGroup.updateProcessing()
                }
            }
        }

        ParamGroup {
            visible: instruments > 0
            groupName: qsTr("Export")

            ColumnLayout {
                RowLayout {
                    CTextField {
                        id: exportPathText
                        hoverEnabled: true
                        Layout.fillWidth: true

                        // text: shortcuts.home // TODO

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
                        text: qsTr("Decimation, m:")
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
                        Settings {
                            property alias exportDecimationValue: exportDecimationValue.value
                        }
                    }

                    CButton {
                        text: qsTr("Export to CSV")
                        Layout.fillWidth: true
                        onClicked: core.exportPlotAsCVS(exportPathText.text, targetPlot.plotDatasetChannel(), exportDecimation.checked ? exportDecimationValue.value : 0);
                    }
                }

                RowLayout {
                    CButton {
                        text: qsTr("Export to XTF")
                        Layout.fillWidth: true
                        onClicked: core.exportPlotAsXTF(exportPathText.text);
                    }
                }

                RowLayout {
                    CButton {
                        text: qsTr("Complex signal to CSV")
                        Layout.fillWidth: true
                        onClicked: core.exportComplexToCSV(exportPathText.text);
                    }
                }

                RowLayout {
                    CButton {
                        text: qsTr("USBL to CSV")
                        Layout.fillWidth: true
                        onClicked: core.exportUSBLToCSV(exportPathText.text);
                    }
                }

            }
        }

        ParamGroup {
            groupName: qsTr("Preference")

            ParamSetup {
                paramName: qsTr("Language:")

                CCombo  {
                    id: appLanguage
                    Layout.fillWidth: true
                    model: [qsTr("English"), qsTr("Russian"), qsTr("Polish")]
                    currentIndex: 0

                    function getLanguageByIndex(index) {
                            switch (index) {
                                case 0:
                                    return qsTr("English");
                                case 1:
                                    return qsTr("Russian");
                                case 2:
                                    return qsTr("Polish");
                                default:
                                    return qsTr("English");
                            }
                        }

                    onCurrentIndexChanged: {
                        control.languageChanged(getLanguageByIndex(currentIndex))
                    }

                    Settings {
                        property alias appLanguage: appLanguage.currentIndex
                    }
                }
            }

            ParamSetup {
                paramName: qsTr("Display theme:")

                CCombo  {
                    id: appTheme
                    Layout.fillWidth: true
                    model: [qsTr("Dark"), qsTr("Super Dark"), qsTr("Light"), qsTr("Super Light")]
                    currentIndex: 0

                    onCurrentIndexChanged: theme.themeID = currentIndex
                    Component.onCompleted: theme.themeID = currentIndex

                    Settings {
                        property alias appTheme: appTheme.currentIndex
                    }
                }
            }

            ParamSetup {
                paramName: qsTr("Instrumets grade:")

                CCombo  {
                    id: instrumentsGradeList
                    Layout.fillWidth: true
                    model: [qsTr("Fish Finders"), qsTr("Bottom Tracking"), qsTr("Maximum")]
                    currentIndex: 0

                    onCurrentIndexChanged: theme.instrumentsGrade = currentIndex
                    Component.onCompleted: theme.instrumentsGrade = currentIndex

                    Settings {
                        property alias instrumentsGradeList: instrumentsGradeList.currentIndex
                    }
                }
            }
        }

        ParamGroup {
            visible: instruments > 1
            groupName: qsTr("Interface")

            CCheck {
                id: consoleVisible
                text: qsTr("Console")

                onCheckedChanged: theme.consoleVisible = checked
                Component.onCompleted: theme.consoleVisible = checked

                Settings {
                    property alias consoleVisible: consoleVisible.checked
                }
            }
        }
    }
}
