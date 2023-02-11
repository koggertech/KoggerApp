import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtQuick.Dialogs 1.2
import Qt.labs.settings 1.1

GridLayout {
    id: control

    property bool is2DHorizontal: horisontalVertical.checked

    MenuBlock {
    }

    ColumnLayout {
        id: columnItem
        spacing: 24
        Layout.margins: 24


        ParamGroup {
            groupName: "Plot"

                RowLayout {
                    CCheck {
                        id: echogramVisible
                        Layout.fillWidth: true
//                        Layout.preferredWidth: 150
                        checked: true
                        text: "Echogram"
                        onCheckedChanged: plot.setChartVis(checked)
                        Component.onCompleted: plot.setChartVis(checked)
                    }

                    CCombo  {
                        id: echoTheme
//                        Layout.fillWidth: true
                        Layout.preferredWidth: 150
                        model: ["Blue", "Sepia", "WRGBD", "WhiteBlack", "BlackWhite"]
                        currentIndex: 0

                        onCurrentIndexChanged: plot.themeId = currentIndex
                        Component.onCompleted: plot.themeId = currentIndex

                        Settings {
                            property alias waterfallThemeId: appTheme.currentIndex
                        }
                    }

                    CCombo  {
                        id: echogramTypesList
//                        Layout.fillWidth: true
                        Layout.preferredWidth: 150
                        model: ["Raw", "Side-Scan"]
                        currentIndex: 0

                        onCurrentIndexChanged: plot.imageType = currentIndex
                        Component.onCompleted: plot.imageType = currentIndex

                        Settings {
                            property alias echogramTypesList: echogramTypesList.currentIndex
                        }
                    }
                }

                RowLayout {
                    CCheck {
                        id: bottomTrackVisible
                        Layout.fillWidth: true
                        text: "Bottom-Track"
                        onCheckedChanged: plot.setDistProcVis(checked)
                        Component.onCompleted: plot.setDistProcVis(checked)
                    }

                    CCombo  {
                        id: bottomTrackThemeList
    //                        Layout.fillWidth: true
    //                        Layout.preferredWidth: 150
                        model: ["Line1", "Line2", "Dot1", "Dot2", "DotLine"]
                        currentIndex: 1

                        onCurrentIndexChanged: plot.bottomTrackTheme = currentIndex
                        Component.onCompleted: plot.bottomTrackTheme = currentIndex

                        Settings {
                            property alias bottomTrackThemeList: bottomTrackThemeList.currentIndex
                        }
                    }
                }

                CCheck {
                    id: rangefinderVisible
                    text: "Rangefinder"
                    onCheckedChanged: plot.setDistVis(checked)
                    Component.onCompleted: plot.setDistVis(checked)
                }

                CCheck {
                    id: ahrsVisible
                    text: "AHRS"
                    onCheckedChanged: plot.setAHRSVis(checked)
                    Component.onCompleted: plot.setAHRSVis(checked)
                }

                CCheck {
                    id: encoderVisible
                    text: "Encoders"
                    onCheckedChanged: plot.setEncoderVis(checked)
                    Component.onCompleted: plot.setEncoderVis(checked)
                }

                RowLayout {
                    id: dopplerBeamVisibleGroup
                    spacing: 0
                    function updateDopplerBeamVisible() {
                        var beamfilter = dopplerBeam1Visible.checked*1 + dopplerBeam2Visible.checked*2 + dopplerBeam3Visible.checked*4 + dopplerBeam4Visible.checked*8
                        plot.setDopplerBeamVis(dopplerBeamVisible.checked,
                                               beamfilter,
                                               dopplerBeamModeVisible.checked,
                                               dopplerBeamAmpVisible.checked)
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
                    spacing: 0
                    CCheck {
                        id: dopplerInstrumentVisible
                        Layout.fillWidth: true
                        text: "Doppler Instrument"
                        onCheckedChanged: plot.setDopplerInstrumentVis(checked)
                        Component.onCompleted: plot.setDopplerInstrumentVis(checked)
                    }

                    CCheck {
                        id: dopplerInstrumentXVisible
                        enabled: false
                        checked: true
                        text: "X"
//                        onCheckedChanged: plot.setDopplerInstrumentVis(checked)
//                        Component.onCompleted: plot.setDopplerInstrumentVis(checked)
                    }

                    CCheck {
                        id: dopplerInstrumentYVisible
                        enabled: false
                        checked: true
                        text: "Y"
//                        onCheckedChanged: plot.setDopplerInstrumentVis(checked)
//                        Component.onCompleted: plot.setDopplerInstrumentVis(checked)
                    }

                    CCheck {
                        id: dopplerInstrumentZVisible
                        enabled: false
                        checked: true
                        text: "Z"
//                        onCheckedChanged: plot.setDopplerInstrumentVis(checked)
//                        Component.onCompleted: plot.setDopplerInstrumentVis(checked)
                    }
                }

                RowLayout {
                    CCheck {
                        id: adcpVisible
                        enabled: false
                        Layout.fillWidth: true
                        text: "Doppler Profiler"
                    }
                }


                RowLayout {
                    CCheck {
                        id: gridVisible
                        Layout.fillWidth: true
                        text: "Grid"
                        onCheckedChanged: plot.setGridNumber(gridNumber.value*gridVisible.checked)
                    }


                    CCheck {
                        id: velocityVisible
                        text: "Velocity"
                        onCheckedChanged: plot.setVelocityVis(checked)
                        Component.onCompleted: plot.setVelocityVis(checked)
                    }

                    SpinBoxCustom {
                        id: gridNumber
                        from: 1
                        to: 20
                        stepSize: 1
                        value: 5

                        onValueChanged: plot.setGridNumber(gridNumber.value*gridVisible.checked)
                        Component.onCompleted: plot.setGridNumber(gridNumber.value*gridVisible.checked)
                    }
                }

                CCheck {
                    id: horisontalVertical
                    text: "Horisontal"
                }

                Settings {
                    property alias echogramVisible: echogramVisible.checked
                    property alias rangefinderVisible: rangefinderVisible.checked
                    property alias postProcVisible: bottomTrackVisible.checked
                    property alias ahrsVisible: ahrsVisible.checked
                    property alias encoderVisible: encoderVisible.checked
                    property alias velocityVisible: velocityVisible.checked
                    property alias gridVisible: gridVisible.checked
                    property alias gridNumber: gridNumber.value

                    property alias dopplerBeamVisible: dopplerBeamVisible.checked
                    property alias dopplerInstrumentVisible: dopplerInstrumentVisible.checked

                    property alias horisontalVertical: horisontalVertical.checked
                }


        }

        ParamGroup {
            id: bottomTrackProcessingGroup
            groupName: "Bottom-Track processing"

            function updateProcessing() {
                plot.doDistProcessing(bottomTrackProcessing.checked ? bottomTrackList.currentIndex : -1,
                                      bottomTrackWindow.checked ? bottomTrackWindowValue.value : 1,
                                      bottomTrackVerticalGap.checked ? bottomTrackVerticalGapValue.value*0.01 : 1,
                                      bottomTrackMinRange.checked ? bottomTrackMinRangeValue.realValue : 0,
                                      bottomTrackMaxRange.checked ? bottomTrackMaxRangeValue.realValue : 1000,
                                      );
            }

            RowLayout {
                CCheck {
                    id: bottomTrackProcessing
                    Layout.fillWidth: true
                    text: "Source type:"
                    onCheckedChanged: bottomTrackProcessingGroup.updateProcessing()
                    Component.onCompleted: bottomTrackProcessingGroup.updateProcessing()
                    Settings {
                        property alias bottomTrackProcessing: bottomTrackProcessing.checked
                    }
                }

                CCombo  {
                    id: bottomTrackList
                    //                        Layout.fillWidth: true
                    Layout.preferredWidth: 250
                    model: ["Echogram 2D", "Echogram Side-Scan"]
                    currentIndex: 0

                    onCurrentIndexChanged: bottomTrackProcessingGroup.updateProcessing()

                    Settings {
                        property alias bottomTrackList: bottomTrackList.currentIndex
                    }
                }
            }

            RowLayout {
                CCheck {
                    id: bottomTrackWindow
                    Layout.fillWidth: true
                    text: "Horizontal window:"
                    onCheckedChanged: bottomTrackProcessingGroup.updateProcessing()

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
                    onValueChanged: bottomTrackProcessingGroup.updateProcessing()

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
                    onCheckedChanged: bottomTrackProcessingGroup.updateProcessing()
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
                    onValueChanged: bottomTrackProcessingGroup.updateProcessing()
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
                    onCheckedChanged: bottomTrackProcessingGroup.updateProcessing()
                    Settings {
                        property alias bottomTrackMinRange: bottomTrackMinRange.checked
                    }
                }


                SpinBoxCustom {
                    id: bottomTrackMinRangeValue
                    from: 0
                    to: 100000
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

                    onRealValueChanged: bottomTrackProcessingGroup.updateProcessing()
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
                    onCheckedChanged: bottomTrackProcessingGroup.updateProcessing()
                    Settings {
                        property alias bottomTrackMaxRange: bottomTrackMaxRange.checked
                    }
                }

                SpinBoxCustom {
                    id: bottomTrackMaxRangeValue
                    from: 0
                    to: 100000
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

                    onRealValueChanged: bottomTrackProcessingGroup.updateProcessing()

                    Settings {
                        property alias bottomTrackMaxRangeValue: bottomTrackMaxRangeValue.value
                    }
                }
            }
        }

        ParamGroup {
            groupName: "Export"

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

                CButton {
                    text: "Export all"
                    Layout.fillWidth: false
                    onClicked: core.exportPlotAsCVS(exportPathText.text);
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
        }

        ParamGroup {
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
