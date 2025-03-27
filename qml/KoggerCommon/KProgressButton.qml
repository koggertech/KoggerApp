import QtQuick 2.12
import QtQuick.Controls 2.12

Button {
    id: control
    property real progress: 0
    text: qsTr("Ok")

    font.family: "Bahnschrift"; font.pointSize: 10;

    contentItem: Text {
        width: control.width
        text: control.text
        font: control.font
        color: "#F07000"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    background: ButtonBackStyle {
        implicitWidth: 35
        implicitHeight: 30

        Rectangle {
           width: (control.width*progress)/100
           height: control.height
           color: "#5080AA"
        }
    }
}
