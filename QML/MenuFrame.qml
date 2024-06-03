import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15

Item {
    id: control

    property int margins: 5
    property int horizontalMargins: margins
    property int verticalMargins: margins
    property int spacing: 10
    property color backgroundColor: theme.menuBackColor

    implicitWidth: columnItem.width + horizontalMargins*2
    implicitHeight: columnItem.height + verticalMargins*2

    default property alias content: columnItem.data

    Rectangle {
        color: backgroundColor
        anchors.fill: parent
        radius: 2
    }

    Item  {
        id: columnItem
        x: horizontalMargins
        y: verticalMargins
        implicitWidth: childrenRect.width
        implicitHeight: childrenRect.height
    }
}
