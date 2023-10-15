import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import QtQuick.Dialogs 1.2
import Qt.labs.settings 1.1

MenuScroll {
    id: scrollBar
    property bool is2DHorizontal: displaySettings.is2DHorizontal
    property int instruments:  displaySettings.instruments
    property var targetPlot: null

    ColumnLayout {
        width: parent.width
        Layout.preferredWidth: parent.width
        implicitWidth:  parent.width
        spacing: 10

        DisplaySettings {
            id: displaySettings
            targetPlot: scrollBar.targetPlot
            Layout.maximumWidth: parent.width
            Layout.preferredWidth: parent.width
        }
    }
}
