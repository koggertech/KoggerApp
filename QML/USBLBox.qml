import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import QtQuick.Dialogs 1.2
import Qt.labs.settings 1.1

DevSettingsBox {
    id: control
    isActive: dev ? (dev.isUSBLBeacon || dev.isUSBL): false

    ColumnLayout {
        id: columnItem
        spacing: 24
        Layout.margins: 24

        // ParamGroup {
        //     id: modeChanger
        //     groupName: "Range Modes"
        //     CheckButton {
        //         text: "One shot"
        //         checkable: false
        //         onClicked: {
        //             dev.askBeaconPosition();
        //         }
        //     }
        // }

        CheckButton {
            visible: dev ? (dev.isUSBL): false
            text: "USBL One shot"
            checkable: false
            onClicked: {
                dev.askBeaconPosition();
            }
        }

        CheckButton {
            visible: dev ? (dev.isUSBLBeacon): false
            text: "Activate Beacon"
            checkable: false
            onClicked: {
                dev.enableBeaconOnce(3);
            }
        }
    }
}
