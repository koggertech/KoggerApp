import QtQuick 2.0
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.12
import QtGraphicalEffects 1.0

Item {
    id: control
    visible: parent.active
    x: -parent.x
    y: parent.height
    width: parent.width
    height: parent.height

    Rectangle {
        opacity: 0.8
        color: "#252320"
        radius: 0
        x: 0
        y: 0
        width: control.width
        height: control.height
        border.color: "#353530"
        border.width: 0
    }
}
