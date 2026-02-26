import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import QtQuick.Dialogs
import QtCore


DevSettingsBox {
    id: control
    isActive: !!(dev && dev.isChartSupport)
    property var importFolder: StandardPaths.writableLocation(StandardPaths.HomeLocation)
    property var exportFolder: StandardPaths.writableLocation(StandardPaths.HomeLocation)

    Settings {
        property alias importFolder: control.importFolder
        property alias exportFolder: control.exportFolder
    }

    ColumnLayout {
        id: columnItem
        spacing: 24
        Layout.margins: 24

        DeviceItem {
            dev: control.dev
        }

        ParamGroup {
            groupName: qsTr("Actions")


            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                CButton {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 100
                    text: qsTr("Flash settings")

                    onClicked: {
                        dev.flashSettings()
                    }
                }

                CButton {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 100
                    text: qsTr("Erase settings")

                    onClicked: {
                        dev.resetSettings()
                    }
                }

                CButton {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 60
                    text: qsTr("Reboot")

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
                    text: qsTr("Set baudrate")

                    onClicked: {
                        dev.baudrate = Number(baudrateCombo.currentText)
                    }
                }
            }
        }

        ParamGroup {
            groupName: qsTr("Settings")

            FileDialog {
                id: importFileDialog
                title: qsTr("Open file")
                fileMode: FileDialog.OpenFile
                currentFolder: control.importFolder
                nameFilters: ["XML files (*.xml)"]

                onCurrentFolderChanged: {
                    control.importFolder = currentFolder
                }

                onAccepted: {
                    const url = importFileDialog.selectedFile
                    if (!url) return
                    control.importFolder = importFileDialog.currentFolder

                    let filePath = url.toString()

                    if (filePath.startsWith("file:///"))
                        filePath = filePath.slice(8)

                    dev.importSettingsFromXML(filePath)
                }
            }

            FileDialog {
                id: exportFileDialog
                title: qsTr("Save as file")
                fileMode: FileDialog.SaveFile
                currentFolder: control.exportFolder
                nameFilters: ["XML files (*.xml)"]

                onCurrentFolderChanged: {
                    control.exportFolder = currentFolder
                }

                onAccepted: {
                    const url = exportFileDialog.selectedFile
                    if (!url || url.toString() === "")
                        return
                    control.exportFolder = exportFileDialog.currentFolder

                    let filePath = url.toString()

                    if (filePath.startsWith("file:///"))
                        filePath = filePath.slice(8)

                    dev.exportSettingsToXML(filePath)
                }
            }

            ColumnLayout {
                RowLayout {
                    CButton {
                        text: qsTr("Import")
                        Layout.fillWidth: true
                        onClicked: {
                            importFileDialog.currentFolder = control.importFolder
                            importFileDialog.open()
                        }
                    }
                    CButton {
                        text: qsTr("Export")
                        Layout.fillWidth: true
                        onClicked: {
                            exportFileDialog.currentFolder = control.exportFolder
                            exportFileDialog.open()
                        }
                    }
                }
            }
        }
    }
}
