import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../controls"

Button {
    property bool isKlfLogging: false
    property bool active: false
    property int borderWidth: 0
    property real iconScale: 0.80
    property real klfTint: 1.0

    id: control
    Layout.preferredHeight: theme.controlHeight
    padding: 0

    readonly property color activeBackColor: theme.activeControlBackColor
    readonly property color baseBackColor: control.active ? control.activeBackColor : theme.controlBackColor

    readonly property color iconTintColor: !control.enabled
                                           ? theme.disabledTextColor
                                           : (control.active ? theme.contrastTextColor(control.baseBackColor) : theme.textColor)

    icon.color: iconTintColor
    icon.width: theme.controlHeight * iconScale
    icon.height: theme.controlHeight * iconScale

    function mixWithRed(base, t) {
        return Qt.rgba(
                    base.r * (1 - t) + 1.0 * t,
                    base.g * (1 - t) + 0.0 * t,
                    base.b * (1 - t) + 0.0 * t,
                    base.a
                );
    }

    background: Rectangle {
        id: backRect
        radius: 2
        height: parent.height
        width: parent.width

        color: control.isKlfLogging
               ? control.mixWithRed(control.baseBackColor, control.klfTint)
               : control.baseBackColor

        border.color: theme.controlBorderColor
        border.width: borderWidth
    }
}
