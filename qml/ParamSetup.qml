import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15

RowLayout {
    Layout.fillWidth: true
    spacing: 10

    property string paramName: qsTr("Param")

    CText {
        Layout.fillWidth: true
        text: paramName
    }
}
