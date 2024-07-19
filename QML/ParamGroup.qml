import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15

ColumnLayout {
    spacing: 5

    property string groupName: qsTr("Group")

    RowLayout {
        spacing: 16

        Rectangle {
            Layout.fillWidth: true
            height: 2
            color: "#808080"
        }

        CText {
            text: groupName
        }

        Rectangle {
            Layout.fillWidth: true
            height: 2
            color: "#808080"
        }
    }
}
