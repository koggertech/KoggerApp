import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Button {
    id: control
    checkable: true
    property bool active: checked || down

    property int borderWidth: 1
    property color color: theme.textColor
    property color checkedColor: theme.textSolidColor
    property color checkedBackColor: theme.activeControlBackColor
    property color backColor: "transparent"
    property color checkedBorderColor: theme.controlSolidBorderColor
    property color borderColor: theme.controlBorderColor
    property string iconSource: ""
    property real iconScale: 0.80
    readonly property bool hoverActive: control.hovered && control.enabled && !control.active
    readonly property bool checkedColorIsCustom: control.checkedColor != theme.textSolidColor
    readonly property color defaultCheckedForegroundColor: {
        var inverted = theme.invertedColor(control.color)
        return theme.contrastRatio(inverted, control.checkedBackColor) >= 2.4
                ? inverted
                : theme.contrastTextColor(control.checkedBackColor)
    }
    readonly property color foregroundColor: !control.enabled
                                             ? theme.disabledTextColor
                                             : (control.active
                                                ? (control.checkedColorIsCustom ? control.checkedColor : control.defaultCheckedForegroundColor)
                                                : control.color)

    implicitHeight: theme.controlHeight

    icon.source: iconSource
    icon.width: control.height * iconScale
    icon.height: control.height * iconScale

    hoverEnabled: true
    padding: 0
    rightPadding: text === "" ? 2 : 6
    leftPadding: icon.source === "" ? 6 : 2

    font: theme.textFont
    palette.buttonText: foregroundColor
    palette.brightText: foregroundColor

    icon.color: foregroundColor

    background: Rectangle {
        id: backRect
        anchors.fill: parent
        anchors.margins: 0
        radius: 2
        height: parent.height
        width: parent.width
        color: !control.enabled ? theme.disabledBackColor
                                : control.active ? control.checkedBackColor
                                                 : hoverActive ? theme.hoveredBackColor
                                                               : control.backColor
        border.color: control.active ? control.checkedBorderColor
                                     : hoverActive ? theme.controlSolidBorderColor
                                                   : control.borderColor
        border.width: control.borderWidth

        Behavior on color { ColorAnimation { duration: 120 } }
        Behavior on border.color { ColorAnimation { duration: 120 } }
    }

    onCheckableChanged: {
        if (!checkable && checked) {
            checked = false
        }
    }

    scale: pressed ? 0.96 : 1
    Behavior on scale {
        NumberAnimation { duration: 100; easing.type: Easing.OutQuad }
    }
}
