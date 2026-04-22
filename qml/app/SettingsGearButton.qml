import QtQuick 2.15
import QtQuick.Layouts 1.15

CircleIconButton {
    property string modeTag: "app"

    iconSource: "qrc:/icons/ui/settings.svg"
    iconPixelSize: modeTag === "app" ? 20 : 19
    toolTipText: modeTag === "3D" ? qsTr("3D settings")
                                 : modeTag === "2D" ? qsTr("2D settings")
                                                    : qsTr("Settings")
    width: 36
    height: 36
    padding: 0
    focusPolicy: Qt.NoFocus
    Layout.preferredWidth: width
    Layout.preferredHeight: height
    Layout.minimumWidth: width
    Layout.minimumHeight: height
    Layout.maximumWidth: width
    Layout.maximumHeight: height
    Layout.alignment: Qt.AlignVCenter
}
