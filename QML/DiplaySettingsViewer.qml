import QtQuick 2.0
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.4
import QtQuick.Dialogs 1.2
import Qt.labs.settings 1.1

MenuScroll {
    id: scrollBar
    property bool is2DHorizontal: displaySettings.is2DHorizontal
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
