import QtQuick 2.12
import QtQuick.Controls 2.12


ProgressBar {
    id: control

    background: Rectangle {
        color: "transparent"
        radius: 2
        border.color: control.visualPosition > 0 ? "#808080" : "transparent"
        border.width: 2
    }

    contentItem: Item {
        Rectangle {
            width: control.visualPosition * (parent.width - 2*parent.width/100)
            x: parent.width/100
            y: parent.height/4
            height: parent.height/2
            radius: 2
            color: "#17a81a"
        }
    }
}
