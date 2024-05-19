import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import QtQuick.Dialogs 1.2
import Qt.labs.settings 1.1

Rectangle {
    id: connectionLine
    radius: 2
    // height: parent.height
    // width: parent.width
    anchors.fill: parent
    color: theme.menuBackColor
    border.color: theme.controlBorderColor
    border.width: 0
}
