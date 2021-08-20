import QtQuick 2.0
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.4
import QtQuick.Dialogs 1.2
import Qt.labs.settings 1.1

MenuScroll {
    id: scrollBar

    ColumnLayout {
        width: parent.width
        spacing: 10

        ConnectionViewer {
            id: devConnection
            Layout.fillWidth: true
            Layout.preferredWidth: parent.width
        }

        //            FactoryBox {
        //                Layout.fillWidth: true
        //                Layout.preferredWidth: parent.width
        //            }

        SonarBox {
            dev: devConnection.dev
            Layout.fillWidth: true
            Layout.preferredWidth: parent.width
        }

//        SonarDatasetBox {
//            dev: devConnection.dev
//            Layout.fillWidth: true
//            Layout.preferredWidth: parent.width
//        }

//        ChartBox {
//            dev: devConnection.dev
//            Layout.fillWidth: true
//            Layout.preferredWidth: parent.width
//        }


//        DistBox {
//            dev: devConnection.dev
//            Layout.fillWidth: true
//            Layout.preferredWidth: parent.width
//        }

//        TransducerBox {
//            dev: devConnection.dev
//            Layout.fillWidth: true
//            Layout.preferredWidth: parent.width
//        }

//        DatasetBox {
//            dev: devConnection.dev
//            Layout.fillWidth: true
//            Layout.preferredWidth: parent.width
//        }

//        DSPSettings {
//            dev: devConnection.dev
//            Layout.fillWidth: true
//            Layout.preferredWidth: parent.width
//        }

//        SoundSpdBox {
//            dev: devConnection.dev
//            Layout.fillWidth: true
//            Layout.preferredWidth: parent.width
//        }

//        FlashBox {
//            dev: devConnection.dev
//            Layout.fillWidth: true
//            Layout.preferredWidth: parent.width
//        }

        UpgradeBox {
            dev: devConnection.dev
            Layout.fillWidth: true
            Layout.preferredWidth: parent.width
        }

//        DevAddrBox {
//            dev: devConnection.dev
//            Layout.fillWidth: true
//            Layout.preferredWidth: parent.width
//        }
    }
}
