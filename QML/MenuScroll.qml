import QtQuick 2.12
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.12
import QtQuick.Dialogs 1.2
import Qt.labs.settings 1.1

ScrollView {
    id: scrollBar
    y:5
    width: parent.width
    height: parent.height - 20
    ScrollBar.vertical.interactive: true
    clip: true
    padding: 10
}
