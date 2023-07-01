import QtQuick 2.12
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.12
import QtQuick.Dialogs 1.2
import Qt.labs.settings 1.1

DevSettingsBox {
    id: control
    isActive: dev.isDoppler
//    isActive: true

    MenuBlock {
    }

    ColumnLayout {
        id: columnItem
        spacing: 24
        Layout.margins: 24

        ParamGroup {
            id: modeChanger
            groupName: "Range Modes"

            function changeMode() {
                dev.dvlChangeMode(mode1Check.checked, mode2Check.checked, mode3Check.checked)
            }

            RowLayout {
                CCheck {
                    id: mode1Check
                    text: "Mode1"

                    onCheckedChanged: {
                        modeChanger.changeMode()
                    }
                }
            }

            RowLayout {
                CCheck {
                    id: mode2Check
                    text: "Mode2"

                    onCheckedChanged: {
                        modeChanger.changeMode()
                    }
                }
            }

            RowLayout {
                CCheck {
                    id: mode3Check
                    text: "Mode3"

                    onCheckedChanged: {
                        modeChanger.changeMode()
                    }
                }
            }
        }

//        ParamGroup {
//            groupName: "Doppler Bottom Tracker"

//            ParamSetup {
//                paramName: "Velocity X, mm/s: " + dev.dopplerVeloX
//            }

//            ParamSetup {
//                paramName: "Velocity Y, mm/s: "  + dev.dopplerVeloY
//            }

//            ParamSetup {
//                paramName: "Velocity Z, mm/s: "  + dev.dopplerVeloZ
//            }

//            ParamSetup {
//                paramName: "Distance, mm: "  + dev.dopplerDist
//            }
//        }

//        ParamGroup {
//            groupName: "Doppler Beam"

//            ParamSetup {
//                paramName: "Mode: " + dev.dopplerBeam1Mode
//            }

//            ParamSetup {
//                paramName: "Distance, m: "  + dev.dopplerBeam1Distance
//            }

//            ParamSetup {
//                paramName: "Amplitude, dB: "  + dev.dopplerBeam1Amplitude
//            }

//            ParamSetup {
//                paramName: "Velocity, mm/s: " + dev.dopplerBeam1Velo
//            }

//            ParamSetup {
//                paramName: "Unc, mm/s: "  + dev.dopplerBeam1Unc
//            }

//        }
    }
}
