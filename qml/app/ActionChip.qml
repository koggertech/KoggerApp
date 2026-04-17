import QtQuick 2.15
import kqml_types 1.0

Rectangle {
    id: chip

    property string label: ""
    property url iconSource: label === "+"
                             ? "qrc:/icons/ui/plus.svg"
                             : (label === "-" ? "qrc:/icons/ui/minus.svg" : "")
    property int iconPixelSize: Math.round(Math.min(chipWidth, chipHeight) * 0.6)
    property int chipWidth: 30
    property int chipHeight: 24
    property color fillColor: "#1E293B"
    property color fillHoverColor: "#172133"
    property color fillPressedColor: "#0B1220"
    property color borderColor: "#334155"
    property color borderHoverColor: "#475569"
    property color textColor: "#E2E8F0"
    property real hoverWhiteness: 0.1
    property string toolTipText: ""
    property bool autoToolTip: true
    readonly property string resolvedToolTipText: {
        if (toolTipText !== "")
            return toolTipText
        if (!autoToolTip)
            return ""
        if (label === "+")
            return "Add pane"
        if (label === "-")
            return "Remove pane"
        return ""
    }

    signal clicked()

    width: chipWidth
    height: chipHeight
    radius: 8
    scale: chipMouse.pressed ? 0.97 : (chipMouse.containsMouse ? 1.03 : 1.0)
    color: chipMouse.pressed ? chip.fillPressedColor : (chipMouse.containsMouse ? chip.fillHoverColor : chip.fillColor)
    border.width: 1
    border.color: (chipMouse.containsMouse || chipMouse.pressed) ? chip.borderHoverColor : chip.borderColor

    Behavior on scale {
        NumberAnimation {
            duration: 100
            easing.type: Easing.OutCubic
        }
    }

    Behavior on color {
        ColorAnimation {
            duration: 100
            easing.type: Easing.OutCubic
        }
    }

    Behavior on border.color {
        ColorAnimation {
            duration: 100
            easing.type: Easing.OutCubic
        }
    }

    Rectangle {
        anchors.fill: parent
        radius: chip.radius
        color: "#FFFFFF"
        opacity: chipMouse.pressed ? 0.02 : (chipMouse.containsMouse ? chip.hoverWhiteness : 0.0)

        Behavior on opacity {
            NumberAnimation {
                duration: 100
                easing.type: Easing.OutCubic
            }
        }
    }

    Text {
        anchors.centerIn: parent
        visible: iconSource === ""
        color: chipMouse.containsMouse ? Qt.lighter(chip.textColor, 1.1) : chip.textColor
        font.pixelSize: 12
        text: chip.label

        Behavior on color {
            ColorAnimation {
                duration: 100
                easing.type: Easing.OutCubic
            }
        }
    }

    Image {
        anchors.centerIn: parent
        visible: iconSource !== ""
        source: iconSource
        width: chip.iconPixelSize
        height: chip.iconPixelSize
        sourceSize.width: width
        sourceSize.height: height
        fillMode: Image.PreserveAspectFit
        cache: true
        asynchronous: true
        smooth: true
        mipmap: true
        opacity: chipMouse.containsMouse ? 1.0 : 0.92

        Behavior on opacity {
            NumberAnimation {
                duration: 100
                easing.type: Easing.OutCubic
            }
        }
    }

    MouseArea {
        id: chipMouse

        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: chip.clicked()
    }

    KToolTip {
        text: chip.resolvedToolTipText
        targetItem: chip
        shown: chipMouse.containsMouse
    }
}
