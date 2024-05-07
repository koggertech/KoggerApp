import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import QtQuick.Dialogs 1.2
import Qt.labs.settings 1.1

MenuScroll {
    id: scrollBar
    property int menuWidth: 200

    ColumnLayout {
        width: menuWidth
        Layout.maximumWidth: menuWidth
        Layout.margins: 0
        spacing: 10

        ConnectionViewer {
            id: devConnection
            width: parent.width
            Layout.maximumWidth: menuWidth
        }

        FactoryBox {
            dev: devConnection.dev
            visible: core.isFactoryMode
            Layout.fillWidth: true
            Layout.preferredWidth: parent.width
        }

        SonarBox {
            dev: devConnection.dev
            Layout.fillWidth: true
            Layout.preferredWidth: parent.width
        }

        DopplerBox {
            dev: devConnection.dev
            Layout.fillWidth: true
            Layout.preferredWidth: parent.width
        }

        RecorderBox {
            dev: devConnection.dev
            Layout.fillWidth: true
            Layout.preferredWidth: parent.width
        }

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
