import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls.Styles 1.4

Button {
    checkable: true
    property bool active: false
    property int borderWidth: 1
    property color color: theme.textColor

    implicitHeight: theme.controlHeight
    implicitWidth: theme.controlHeight

    id: control
    Layout.preferredHeight: theme.controlHeight
    padding: 0
    font: theme.textFont
    palette.buttonText: color

    icon.color: checked ? "black" : theme.textColor

    background: Rectangle {
        id: backRect
        radius: 2
        height: parent.height
        width: parent.width
        color: checked ? "white" : "transparent"
        border.color: checked ? "transparent" : theme.textColor
        border.width: borderWidth
    }
}
