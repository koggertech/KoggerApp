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
    property color borderColor: theme.controlSolidBorderColor
    property string iconSource: ""

    implicitHeight: theme.controlHeight
    //implicitWidth: icon.width + textWidth + leftPadding + rightPadding

    icon.source: iconSource

    padding: 0
    rightPadding: text === "" ? 2 : 6
    leftPadding: icon.source == "" ? 6 : 2

    //height: theme.controlHeight
    //width: text === "" ? theme.controlHeight : undefined


    font: theme.textFont
    palette.buttonText: checked ? checkedColor : color
    palette.brightText: checked ? checkedColor : color

    icon.color: checked ? checkedColor : color

    background: Rectangle {
        id: backRect
        anchors.fill: parent
        anchors.margins: 0
        radius: 2
        height: parent.height
        width: parent.width
        color: control.checked ? control.checkedBackColor : control.backColor
        border.color: control.checked ? control.checkedBorderColor : control.borderColor
        border.width: control.borderWidth
    }
}
