import QtQuick 2.15
import kqml_types 1.0

Rectangle {
    id: root

    readonly property var _core: (typeof core !== "undefined") ? core : null
    readonly property bool _touch: (typeof inputDeviceTracker !== "undefined" && inputDeviceTracker)
                                   ? inputDeviceTracker.touchMode : false
    readonly property bool _contact: !!_core && _core.contactPlacementArmed
    readonly property bool _goTo: !!_core && _core.goToPlacementArmed
    readonly property string text: {
        if (_goTo)
            return _touch ? qsTr("Tap a point in the 3D view to send the boat there")
                          : qsTr("Click a point in the 3D view (left button) to send the boat there · Esc cancels")
        if (_contact)
            return _touch ? qsTr("Tap the echogram or the 3D view to place a contact")
                          : qsTr("Click the echogram or the 3D view (left button) to place a contact · Esc cancels")
        return ""
    }

    property real maxWidth: hintText.implicitWidth + Tokens.spaceXl * 2
    width: Math.min(maxWidth, hintText.implicitWidth + Tokens.spaceXl * 2)
    height: hintText.implicitHeight + Tokens.spaceMd * 2
    radius: Tokens.radiusLg
    color: AppPalette.card
    border.color: AppPalette.border
    border.width: 1
    visible: text.length > 0

    Text {
        id: hintText
        anchors.centerIn: parent
        width: parent.width - Tokens.spaceXl * 2
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.WordWrap
        color: AppPalette.textStrong
        font.pixelSize: Tokens.fontLg
        text: root.text
    }
}
