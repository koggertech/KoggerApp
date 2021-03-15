import QtQuick 2.0
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.4
import QtQuick.Dialogs 1.2
import Qt.labs.settings 1.1

MenuViewer {
    id: viewerDeviceSettings
    height: 500

    MenuScroll {
        id: scrollBar

        ColumnLayout {
            width: parent.width
            spacing: 10

            ConnectionViewer {
                Layout.fillWidth: true
                Layout.preferredWidth: parent.width
            }

            DatasetBox {
                Layout.fillWidth: true
                Layout.preferredWidth: parent.width
            }

            ChartBox {
                Layout.fillWidth: true
                Layout.preferredWidth: parent.width
            }


            DistBox {
                Layout.fillWidth: true
                Layout.preferredWidth: parent.width
            }

            TransducerBox {
                Layout.fillWidth: true
                Layout.preferredWidth: parent.width
            }



            SoundSpdBox {
                Layout.fillWidth: true
                Layout.preferredWidth: parent.width
            }

            FlashBox {
                Layout.fillWidth: true
                Layout.preferredWidth: parent.width
            }

            UpgradeBox {
                Layout.fillWidth: true
                Layout.preferredWidth: parent.width
            }

//            DevAddrBox {
//                Layout.fillWidth: true
//                Layout.preferredWidth: parent.width
//            }


        }
    }
}
