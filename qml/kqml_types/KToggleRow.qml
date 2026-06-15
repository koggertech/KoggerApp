import QtQuick 2.15
import QtQuick.Layouts 1.15

Rectangle {
    id: ktrRoot

    property string label: ""
    property url iconSource: ""
    property bool checked: false
    signal toggled(bool val)

    Layout.fillWidth: true
    implicitHeight: Math.round(38 * AppPalette.scale)
    radius: Tokens.radiusLg
    color: ktrRootMouse.containsMouse ? AppPalette.bgHover : AppPalette.bg
    border.width: 1
    border.color: ktrRootMouse.containsMouse ? AppPalette.borderHover : AppPalette.border
    Behavior on color        { ColorAnimation { duration: 110 } }
    Behavior on border.color { ColorAnimation { duration: 110 } }

    function _flip() {
        ktrRoot.checked = !ktrRoot.checked
        ktrRoot.toggled(ktrRoot.checked)
    }

    MouseArea {
        id: ktrRootMouse
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        z: -1
        onClicked: ktrRoot._flip()
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Tokens.spaceMd
        anchors.rightMargin: Tokens.spaceMd
        spacing: Tokens.spaceMd

        Image {
            source: ktrRoot.iconSource
            sourceSize.width:  Tokens.iconSm
            sourceSize.height: Tokens.iconSm
            Layout.preferredWidth:  Tokens.iconSm
            Layout.preferredHeight: Tokens.iconSm
            fillMode: Image.PreserveAspectFit
            Layout.alignment: Qt.AlignVCenter
            visible: ktrRoot.iconSource !== ""
        }

        Text {
            text: ktrRoot.label
            color: AppPalette.text
            font.pixelSize: Tokens.fontMd
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter
            elide: Text.ElideRight
        }

        KSwitch {
            checked: ktrRoot.checked
            onToggled: function(v) {
                ktrRoot.checked = v
                ktrRoot.toggled(v)
            }
            Layout.alignment: Qt.AlignVCenter
        }
    }
}
