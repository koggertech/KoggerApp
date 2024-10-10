import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import QtQuick.Dialogs 1.2
import Qt.labs.settings 1.1

DevSettingsBox {
    id: control
    isActive: dev !== null ? dev.isChartSupport : false

    ColumnLayout {
        id: columnItem
        spacing: 24
        Layout.margins: 24

        DeviceItem {
            dev: control.dev
        }

        ParamGroup {
            groupName: "Actions"


            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                CButton {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 100
                    text: "Flash settings"

                    onClicked: {
                        dev.flashSettings()
                    }
                }

                CButton {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 100
                    text: "Erase settings"

                    onClicked: {
                        dev.resetSettings()
                    }
                }

                CButton {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 60
                    text: "Reboot"

                    onClicked: {
                        dev.reboot()
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                CCombo  {
                    id: baudrateCombo
                    Layout.fillWidth: true
                    model: [9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600, 1200000, 2000000]
                    currentIndex: 4
                }

                CButton {
                    text: "Set baudrate"

                    onClicked: {
                        dev.baudrate = Number(baudrateCombo.currentText)
                    }
                }
            }
        }

        ParamGroup {
            groupName: "Settings"

            FileDialog {
                id: importFileDialog
                title: "Open file"
                selectExisting: true
                nameFilters: ["XML files (*.xml)"]

                onAccepted: {
                    var selectedFile = importFileDialog.fileUrl
                    if (selectedFile !== "") {
                        var filePath = selectedFile.toString();
                        if (Qt.platform.os === "windows")
                            filePath = filePath.substring(8)
                        dev.importSettingsFromXML(filePath)
                    }
                }
            }

            FileDialog {
                id: exportFileDialog
                title: "Save as file"
                selectExisting: false
                nameFilters: ["XML files (*.xml)"]

                onAccepted: {
                    var selectedFile = exportFileDialog.fileUrl
                    if (selectedFile !== "") {
                        var filePath = selectedFile.toString();
                        if (Qt.platform.os === "windows")
                            filePath = filePath.substring(8)
                        dev.exportSettingsToXML(filePath)
                    }
                }
            }

            ColumnLayout {
                RowLayout {
                    CButton {
                        text: "Import"
                        Layout.fillWidth: true
                        onClicked: {
                            importFileDialog.open()
                        }
                    }
                    CButton {
                        text: "Export"
                        Layout.fillWidth: true
                        onClicked: {
                            exportFileDialog.open()
                        }
                    }
                }
            }
        }
    }
}
