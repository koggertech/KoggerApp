import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import QtQuick.Dialogs
import Qt.labs.settings 1.1

MenuScroll {
    id: scrollBar
    property bool is2DHorizontal: displaySettings.is2DHorizontal
    property int instruments:  displaySettings.instruments
    property var targetPlot: null
    property int menuWidth: 200

    signal languageChanged(string langStr)

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

    Component.onCompleted: {
        displaySettings.languageChanged.connect(handleChildSignal)
    }
}
