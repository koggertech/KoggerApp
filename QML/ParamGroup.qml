import QtQuick 2.12
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.12

ColumnLayout {
    spacing: 5

    property string groupName: "Group"

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
