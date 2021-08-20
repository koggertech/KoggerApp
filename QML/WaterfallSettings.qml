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
                text: "Waterfall theme:"
                color: "white"
                font.pointSize: 12
            }

//            CCombo  {
//                id: waterfallTheme
//                Layout.fillWidth: true
//                model: ["Classic", "Sepia", "WRGBD", "WhiteBlack", "BlackWhite"]
//                currentIndex: 1

//                onCurrentIndexChanged: {
//                    plot.themeId = currentIndex
//                }

//                Component.onCompleted: {
//                    plot.themeId = currentIndex
//                }

//                Settings {
//                    property alias waterfallTheme: waterfallTheme.currentIndex
//                }
//            }
        }
    }

}
