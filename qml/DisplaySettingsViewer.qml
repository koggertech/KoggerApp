import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import QtQuick.Dialogs

MenuScroll {
    id: scrollBar

    property int numPlots: displaySettings.numPlots
    property bool syncPlots: displaySettings.syncPlots
    property int instruments: displaySettings.instruments
    property var targetPlot: null
    property int menuWidth: 200
    property bool extraInfoVis: displaySettings.extraInfoVis
    property bool extraInfoDepthVis: displaySettings.extraInfoDepthVis
    property bool extraInfoSpeedVis: displaySettings.extraInfoSpeedVis
    property bool extraInfoCoordinatesVis: displaySettings.extraInfoCoordinatesVis
    property bool extraInfoActivePointVis: displaySettings.extraInfoActivePointVis
    property bool extraInfoSimpleNavV2Vis: displaySettings.extraInfoSimpleNavV2Vis
    property bool extraInfoBoatStatusVis: displaySettings.extraInfoBoatStatusVis
    property bool autopilotInfofVis: displaySettings.autopilotInfofVis
    property bool profilesButtonVis: displaySettings.profilesButtonVis

    signal languageChanged(string langStr)
    signal syncPlotEnabled()

    onVisibleChanged: {
        if (!visible && displaySettings) {
            displaySettings.closeExtraInfoFiltersPopup()
        }
    }

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
