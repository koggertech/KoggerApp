import QtQuick 2.12
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.12

ColumnLayout {
    spacing: 2

    property string groupName: qsTr("Group")

    RowLayout {
        spacing: 16

        Rectangle {
            Layout.fillWidth: true
            height: 2
            color: "#808080"
        }

        KText {
            text: groupName
        }

        Rectangle {
            Layout.fillWidth: true
            height: 2
            color: "#808080"
        }
    }
}
