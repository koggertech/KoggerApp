import QtQuick 2.12
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.12
import QtQuick.Dialogs 1.2
import Qt.labs.settings 1.1


Item {
    id: control
    Layout.preferredHeight: columnItem.height

    MenuBlock {
    }

    ColumnLayout {
        id: columnItem
        width: control.width

        RowLayout {
            Layout.fillWidth: true
            Layout.margins: 15
            spacing: 10

            Text {
//                Layout.leftMargin: 20
                text: "Speed of Sound, m/s:"
                color: "#808080"
                font.pixelSize: 16
            }

            SpinBoxCustom {
                width: 120
                from: 300
                to: 6000
                stepSize: 5
                value: sonarDriver.soundSpeed/1000
                onValueChanged: {
                    sonarDriver.soundSpeed = value*1000
                }
            }
        }

    }
}
