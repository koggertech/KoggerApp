import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtQuick.Dialogs 1.2
import Qt.labs.settings 1.1

GridLayout {
    id: control

    MenuBlock {
    }

    ColumnLayout {
        id: columnItem
        spacing: 24
        Layout.margins: 24

        ParamGroup {
            groupName: "Preference"

            ParamSetup {
                paramName: "Display theme:"

                CCombo  {
                    id: appTheme
                    Layout.fillWidth: true
                    model: ["Dark", "Super Dark", "Light", "Super Light"]
                    currentIndex: 0

                    onCurrentIndexChanged: theme.themeID = currentIndex
                    Component.onCompleted: theme.themeID = currentIndex

                    Settings {
                        property alias appTheme: appTheme.currentIndex
                    }
                }
            }
        }

        ParamGroup {
            groupName: "Interface"

            CCheck {
                id: consoleVisible
                text: "Console"

                onCheckedChanged: theme.consoleVisible = checked
                Component.onCompleted: theme.consoleVisible = checked

                Settings {
                    property alias consoleVisible: consoleVisible.checked
                }
            }
        }


    }

}
