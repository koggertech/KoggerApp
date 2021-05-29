import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtQuick.Dialogs 1.2
import Qt.labs.settings 1.1

Item {
    id: control
    Layout.fillWidth: true
    Layout.preferredHeight: columnItem.height

    ColumnLayout {
        id: columnItem
        width: control.width

        MenuBlock {
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.margins: 15
            width: control.width

            Text {
                id: textTheme
                text: "Export:"
                color: "white"
                font.pointSize: 12
            }

            CCheck {
                id: exportDist
                text: "Distance"
                checked: true
            }
        }


        RowLayout {
            Layout.fillWidth: true
            Layout.margins: 15
            width: control.width

            CButton {
                Layout.fillWidth: true
                text: "Export to CSV"
                onClicked: {
                    core.exportPlotAsCVS();
                }
            }
        }
    }

}
