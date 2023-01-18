import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtQuick.Dialogs 1.2
import Qt.labs.settings 1.1

GridLayout {
    id: control

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

                    CCombo  {
                        id: bottomTrackList
//                        Layout.fillWidth: true
                        Layout.preferredWidth: 150
                        model: ["2D", "Side-Scan"]
                        currentIndex: 0

                        onCurrentIndexChanged: plot.bottomTrackType = currentIndex
                        Component.onCompleted: plot.bottomTrackType = currentIndex

                        Settings {
                            property alias bottomTrackList: bottomTrackList.currentIndex
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
                    CCheck {
                        id: dopplerBeamVisible
                        Layout.fillWidth: true
                        text: "Doppler Beam"
                        onCheckedChanged: plot.setDopplerBeamVis(checked)
                        Component.onCompleted: plot.setDopplerBeamVis(checked)
                    }

                    CCheck {
                        id: dopplerBeam1Visible
                        enabled: false
                        checked: true
                        text: "1"

//                        onCheckedChanged: plot.setDopplerBeamVis(checked)
//                        Component.onCompleted: plot.setDopplerBeamVis(checked)
                    }

                    CCheck {
                        id: dopplerBeam2Visible
                        enabled: false
                        checked: true
                        text: "2"
//                        onCheckedChanged: plot.setDopplerBeamVis(checked)
//                        Component.onCompleted: plot.setDopplerBeamVis(checked)
                    }

                    CCheck {
                        id: dopplerBeam3Visible
                        enabled: false
                        checked: true
                        text: "3"
//                        onCheckedChanged: plot.setDopplerBeamVis(checked)
//                        Component.onCompleted: plot.setDopplerBeamVis(checked)
                    }

                    CCheck {
                        id: dopplerBeam4Visible
                        enabled: false
                        checked: true
                        text: "4"
//                        onCheckedChanged: plot.setDopplerBeamVis(checked)
//                        Component.onCompleted: plot.setDopplerBeamVis(checked)
                    }
                }

                RowLayout {
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
                }

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
