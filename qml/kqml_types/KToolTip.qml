import QtQuick 2.15
import QtQuick.Controls 2.15

ToolTip {
    id: root

    property Item targetItem: null
    property bool shown: false

    visible: shown && text !== ""
    delay: 520
    timeout: 3200
    x: parent ? Math.round((parent.width - implicitWidth) / 2) : 0
    y: parent ? -implicitHeight - 8 : 0

    contentItem: Text {
        text: root.text
        color: "#E2E8F0"
        font.pixelSize: 12
        wrapMode: Text.NoWrap
    }

    background: Rectangle {
        radius: 6
        color: "#0B1220"
        border.width: 1
        border.color: "#334155"
    }
}
