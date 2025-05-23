import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import QtQuick.Dialogs 1.2
import Qt.labs.settings 1.1

MenuScroll {
    id: scrollBar

    property int numPlots: displaySettings.numPlots
    property bool syncPlots: displaySettings.syncPlots
    property int instruments: displaySettings.instruments
    property var targetPlot: null
    property int menuWidth: 200

    signal languageChanged(string langStr)
    signal syncPlotEnabled()

    function updateBottomTrack() {
        displaySettings.updateBottomTrack()
    }

    ColumnLayout {
        MenuFrame {
            DisplaySettings {
                id: displaySettings
                targetPlot: scrollBar.targetPlot
                width: menuWidth
            }
        }
    }

    function handleChildSignal(langStr) {
        languageChanged(langStr)
    }

    function handleSyncPlotEnabled() {
        syncPlotEnabled()
    }

    Component.onCompleted: {
        displaySettings.languageChanged.connect(handleChildSignal)
        displaySettings.syncPlotEnabled.connect(handleSyncPlotEnabled)
    }
}
