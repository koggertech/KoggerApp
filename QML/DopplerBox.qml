import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import QtQuick.Dialogs 1.2
import Qt.labs.settings 1.1

DevSettingsBox {
    id: control
    isActive: dev.isDoppler

    MenuBlock {
    }

    ColumnLayout {
        id: columnItem
        spacing: 24
        Layout.margins: 24

        ParamGroup {
            id: modeChanger
            groupName: "Range Modes"

            function changeMode() {
                dev.dvlChangeMode(mode1Check.checked, mode2Check.checked, mode3Check.checked)
            }

            RowLayout {
                CCheck {
                    id: mode1Check
                    text: "Mode1"

                    onCheckedChanged: {
                        modeChanger.changeMode()
                    }
                }
            }

            RowLayout {
                CCheck {
                    id: mode2Check
                    text: "Mode2"

                    onCheckedChanged: {
                        modeChanger.changeMode()
                    }
                }
            }

            RowLayout {
                CCheck {
                    id: mode3Check
                    text: "Mode3"

                    onCheckedChanged: {
                        modeChanger.changeMode()
                    }
                }
            }
        }

    }
}
