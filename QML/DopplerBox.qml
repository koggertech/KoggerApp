import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import QtQuick.Dialogs 1.2
import Qt.labs.settings 1.1

DevSettingsBox {
    id: control
    isActive: dev ? dev.isDoppler : false

    ColumnLayout {
        id: columnItem
        spacing: 24
        Layout.margins: 24

        ParamGroup {
            id: modeChanger
            groupName: qsTr("Range Modes")

            function changeMode() {
                dev.dvlChangeMode(mode1Check.checked, mode2Check.checked, mode3Check.checked, mode4Check.checked, mode4Range.value)
            }

            RowLayout {
                CCheck {
                    id: mode1Check
                    text: qsTr("Mode1")
                    checked: true

                    onCheckedChanged: {
                        modeChanger.changeMode()
                    }
                }
            }

            RowLayout {
                CCheck {
                    id: mode2Check
                    text: qsTr("Mode2")
                    checked: true

                    onCheckedChanged: {
                        modeChanger.changeMode()
                    }
                }
            }

            RowLayout {
                CCheck {
                    id: mode3Check
                    text: qsTr("Mode3")
                    checked: true

                    onCheckedChanged: {
                        modeChanger.changeMode()
                    }
                }
            }

            RowLayout {
                CCheck {
                    id: mode4Check
                    text: qsTr("Mode4 range, m")
                    checked: true

                    onCheckedChanged: {
                        modeChanger.changeMode()
                    }
                }

                SpinBoxCustom {
                    id: mode4Range
                    from: 30
                    to: 100
                    stepSize: 5
                    value: 100
                    onValueChanged: {
                        modeChanger.changeMode()
                    }

                }
            }
        }

    }
}
