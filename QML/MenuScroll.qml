import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import Qt.labs.settings 1.1

ScrollView {
    id: scrollBar
    y:0
    ScrollBar.vertical.interactive: true
    clip: true
    padding: 10
}
