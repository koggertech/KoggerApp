import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls.Styles 1.4

Button {
    property bool isKlfLogging: false
    property bool active: false
    property int borderWidth: 0
    property real klfTint: 1.0

    id: control
    Layout.preferredHeight: theme.controlHeight
    padding: 0

    icon.color: theme.textColor

    property color baseBackColor: (control.active)
                                  ? theme.controlSolidBackColor
                                  : theme.controlBackColor

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

//    contentItem: CText {
//        text: control.text
//        horizontalAlignment: Text.AlignHCenter
//        verticalAlignment: Text.AlignTop
//        font.pointSize: 20
//    }
}
