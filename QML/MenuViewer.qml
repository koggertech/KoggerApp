import QtQuick 2.0
import QtQuick.Layouts 1.3

Item {
    id: control
    visible: parent.active
    x: -parent.x
    y: parent.height
    width: parent.width
    height: parent.height

    Rectangle {
        opacity: 0.7
        color: "#204060"
        x: 3
        y: 3
        width: parent.width - 6
        height: parent.height - 6
    }

    Rectangle {
        opacity: 1.0
        x: 3
        y: 3
        width: control.width - 6
        height: 1
        color: "#4099F0"
    }

    Rectangle {
        opacity: 1.0
        x: 3
        y: control.height - 3
        width: control.width - 6
        height: 1
        color: "#4099F0"
    }

    Rectangle {
        opacity: 1.0
        x: control.width - 3
        width: 1
        height: control.height - 3
        color: "#4099F0"
    }

    Rectangle {
        opacity: 1.0
        x: 3
        y: 3
        width: 1
        height: control.height - 6
        color: "#4099F0"
    }

    Rectangle {
        opacity: 1.0
        anchors.right: parent.right
        anchors.top: parent.top
        width: control.width/3
        height: 3
        color: "#4099F0"
    }

    Rectangle {
        opacity: 1.0
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        width: control.width/3
        height: 3
        color: "#4099F0"
    }

    Rectangle {
        opacity: 1.0
        anchors.right: parent.right
        anchors.top: parent.top
        width: 3
        height: control.height/3
        color: "#4099F0"
    }

    Rectangle {
        opacity: 1.0
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        width: 3
        height: control.height/3
        color: "#4099F0"
    }

}
