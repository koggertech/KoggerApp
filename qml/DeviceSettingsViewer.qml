import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import QtQuick.Dialogs 1.2
import Qt.labs.settings 1.1

MenuScroll {
    id: scrollBar
    property int menuWidth: 200
    property string filePath: devConnection.filePath

    Column {
        // width: menuWidth
        // Layout.margins: 0
        padding: 0
        spacing: 10

        MenuFrame {
            ConnectionViewer {
                id: devConnection
                width: menuWidth
            }
        }

        MenuFrame {
            visible: false
            FactoryBox {
                dev: devConnection.dev
                visible: core.isFactoryMode
            }
        }

        MenuFrame {
            visible: sonarBox.isActive
            SonarBox {
                id: sonarBox
                dev: devConnection.dev
                width: menuWidth
            }
        }

        MenuFrame {
            visible: dopplerBox.isActive
            DopplerBox {
                id: dopplerBox
                visible: isActive
                dev: devConnection.dev
                width: menuWidth
            }
        }

        MenuFrame {
            visible: usblBox.isActive
            USBLBox {
                id: usblBox
                visible: isActive
                dev: devConnection.dev
                width: menuWidth
            }
        }

        MenuFrame {
            visible: recorderBox.isActive
            RecorderBox {
                id: recorderBox
                dev: devConnection.dev
                width: menuWidth
            }
        }

        MenuFrame {
            visible: upgradeBox.isActive
            UpgradeBox {
                id: upgradeBox
                dev: devConnection.dev
                width: menuWidth
            }
        }

//        DevAddrBox {
//            dev: devConnection.dev
//            Layout.fillWidth: true
//            Layout.preferredWidth: parent.width
//        }
    }
}
