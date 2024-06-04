import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls.Styles 1.4

Button {
    id: control
    checkable: true
    property bool active: false
    property int borderWidth: 1
    property color color: theme.textColor
    property color checkedColor: "black"
    property color checkedBackColor: "white"
    property color backColor: "transparent"
    property color checkedBorderColor: "transparent"
    property color borderColor: theme.textColor
    rightPadding: text === "" ? 2 : 6
    leftPadding: text === "" ? 2 : 6


    height: theme.controlHeight
    // width: text === "" ? theme.controlHeight : undefined

    padding: 0
    font: theme.textFont
    palette.buttonText: checked ? checkedColor : color
    palette.brightText: checked ? checkedColor : color

    icon.color: checked ? checkedColor : color

    background: Rectangle {
        id: backRect
        radius: 2
        height: parent.height
        width: parent.width
        color: control.checked ? control.checkedBackColor : control.backColor
        border.color: control.checked ? control.checkedBorderColor : control.borderColor
        border.width: control.borderWidth
    }
}
