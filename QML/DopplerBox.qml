import QtQuick 2.12
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.12
import QtQuick.Dialogs 1.2
import Qt.labs.settings 1.1

DevSettingsBox {
    id: control
    isActive: dev.isDoppler

    MenuBlock {
    }

    ColumnLayout {
        id: columnItem
        spacing: 24
        Layout.margins: 24

        ParamGroup {
            groupName: "Doppler Bottom Tracker"

            ParamSetup {
                paramName: "Velocity X, mm/s: " + dev.dopplerVeloX
            }

            ParamSetup {
                paramName: "Velocity Y, mm/s: "  + dev.dopplerVeloY
            }

            ParamSetup {
                paramName: "Velocity Z, mm/s: "  + dev.dopplerVeloZ
            }

            ParamSetup {
                paramName: "Distance, mm: "  + dev.dopplerDist
            }
        }

        ParamGroup {
            groupName: "Doppler Beam"

            ParamSetup {
                paramName: "Mode: " + dev.dopplerBeam1Mode
            }

            ParamSetup {
                paramName: "Distance, m: "  + dev.dopplerBeam1Distance
            }

            ParamSetup {
                paramName: "Amplitude, dB: "  + dev.dopplerBeam1Amplitude
            }

            ParamSetup {
                paramName: "Velocity, mm/s: " + dev.dopplerBeam1Velo
            }

            ParamSetup {
                paramName: "Unc, mm/s: "  + dev.dopplerBeam1Unc
            }

        }
    }
}
