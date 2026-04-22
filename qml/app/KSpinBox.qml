import QtQuick 2.15
import kqml_types 1.0

Item {
    id: root

    property int value: 0
    property int from: 0
    property int to: 100
    property int stepSize: 1
    property real divisor: 1.0
    property int decimals: 0

    signal valueModified(int val)

    implicitWidth: 115
    implicitHeight: 30

    function clamp(v) { return Math.max(root.from, Math.min(root.to, v)) }

    function increment() {
        var next = clamp(root.value + root.stepSize)
        if (next !== root.value) { root.value = next; root.valueModified(next) }
    }

    function decrement() {
        var prev = clamp(root.value - root.stepSize)
        if (prev !== root.value) { root.value = prev; root.valueModified(prev) }
    }

    Rectangle {
        anchors.fill: parent
        radius: 6
        color: AppPalette.bg
        border.width: 1
        border.color: (minusMouse.containsMouse || plusMouse.containsMouse) ? AppPalette.borderHover : AppPalette.border

        Row {
            anchors.fill: parent

            Rectangle {
                width: 26
                height: parent.height
                color: minusMouse.pressed ? AppPalette.bgDeep : (minusMouse.containsMouse ? AppPalette.card : "transparent")
                radius: 6

                Text {
                    anchors.centerIn: parent
                    text: "−"
                    color: AppPalette.textMuted
                    font.pixelSize: 15
                    font.bold: true
                }

                MouseArea {
                    id: minusMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.decrement()
                    onPressAndHold: holdDec.start()
                    onReleased: holdDec.stop()
                    onCanceled: holdDec.stop()
                }

                Timer { id: holdDec; interval: 80; repeat: true; onTriggered: root.decrement() }
            }

            Item {
                width: parent.width - 52
                height: parent.height

                Text {
                    anchors.centerIn: parent
                    text: (root.value / root.divisor).toFixed(root.decimals)
                    color: AppPalette.text
                    font.pixelSize: 12
                    elide: Text.ElideRight
                }
            }

            Rectangle {
                width: 26
                height: parent.height
                color: plusMouse.pressed ? AppPalette.bgDeep : (plusMouse.containsMouse ? AppPalette.card : "transparent")
                radius: 6

                Text {
                    anchors.centerIn: parent
                    text: "+"
                    color: AppPalette.textMuted
                    font.pixelSize: 15
                    font.bold: true
                }

                MouseArea {
                    id: plusMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.increment()
                    onPressAndHold: holdInc.start()
                    onReleased: holdInc.stop()
                    onCanceled: holdInc.stop()
                }

                Timer { id: holdInc; interval: 80; repeat: true; onTriggered: root.increment() }
            }
        }
    }
}
