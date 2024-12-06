import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import QtQuick.Dialogs
import Qt.labs.settings 1.1

DevSettingsBox {
    id: control
    isActive: dev ? (dev.isUSBLBeacon || dev.isUSBL): false

    property bool isUSBL: dev ? (dev.isUSBL) ? true : false : false
    property bool isBeacon: dev ? (dev.isUSBLBeacon) ? true : false : false

    ColumnLayout {
        id: columnItem
        // spacing: 10
        Layout.margins: 10
        Layout.topMargin: 0
        Layout.bottomMargin: 0

        RowLayout {
            visible: isUSBL
            CheckButton {
                icon.source: checked ? "./icons/radar.svg" :  "./icons/radar-off.svg"
                onCheckedChanged: {
                    deviceManagerWrapper.isbeaconDirectQueueAsk = checked
                }
            }

            CheckButton {
                Layout.leftMargin: 12
                // enabled: false
                checkable: false
                borderColor: "transparent"
                icon.source: "./icons/refresh.svg"

                onClicked: {
                    dev.askBeaconPosition();
                }
            }

            CText {
                Layout.fillWidth: true
                text: "1"
            }

            CheckButton {
                borderColor: "transparent"
                icon.source: "./icons/settings.svg"
            }
        }

        RowLayout {
            // visible: isBeacon
            CheckButton {
                icon.source: "./icons/submarine.svg"
                // enabled: false
                checkable: false
                borderColor: "transparent"
                text: "1"

                onClicked: {
                    dev.enableBeaconOnce(3);
                }
            }

            CheckButton {
                Layout.leftMargin: 12
                enabled: false
                borderColor: "transparent"
                icon.source: "./icons/arrow-bar-down.svg"
            }

            CText {
                text: "20.0 m"
            }

            CheckButton {
                Layout.leftMargin: 12
                enabled: false
                borderColor: "transparent"
                icon.source: "./icons/ruler-measure.svg"
            }

            CText {
                Layout.fillWidth: true
                text: "100.0 m"
            }

            CheckButton {
                borderColor: "transparent"
                icon.source: "./icons/settings.svg"
            }
        }
    }
}
