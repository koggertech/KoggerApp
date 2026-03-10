import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import QtQuick.Dialogs

MenuScroll {
    id: scrollBar
    property int menuWidth: 200
    property string filePath: devConnection.filePath
    function importProfileForAllDevices(path) {
        devConnection.importSettingsToAllDevices(path)
    }

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

        MenuFrame {
            id: recentOpenedFrame
            visible: devConnection.recentOpenedFiles.length > 0

            ColumnLayout {
                width: menuWidth
                spacing: 4

                MenuRow {
                    CText {
                        text: qsTr("Recently opened:")
                    }
                }

                MenuRow {
                    visible: devConnection.recentOpenedFiles.length > 0
                    Item {
                        Layout.fillWidth: true
                        Layout.preferredHeight: theme.controlHeight
                        property string filePath: devConnection.recentOpenedFiles.length > 0 ? devConnection.recentOpenedFiles[0] : ""

                        Rectangle {
                            anchors.fill: parent
                            color: "transparent"
                            border.width: 0
                        }

                        CText {
                            anchors.fill: parent
                            anchors.leftMargin: 6
                            anchors.rightMargin: 6
                            text: parent.filePath
                            horizontalAlignment: Text.AlignRight
                            verticalAlignment: Text.AlignVCenter
                            elide: Text.ElideLeft
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: devConnection.openRecentFile(parent.filePath)
                        }
                    }
                }

                MenuRow {
                    visible: devConnection.recentOpenedFiles.length > 1
                    Item {
                        Layout.fillWidth: true
                        Layout.preferredHeight: theme.controlHeight
                        property string filePath: devConnection.recentOpenedFiles.length > 1 ? devConnection.recentOpenedFiles[1] : ""

                        Rectangle {
                            anchors.fill: parent
                            color: "transparent"
                            border.width: 0
                        }

                        CText {
                            anchors.fill: parent
                            anchors.leftMargin: 6
                            anchors.rightMargin: 6
                            text: parent.filePath
                            horizontalAlignment: Text.AlignRight
                            verticalAlignment: Text.AlignVCenter
                            elide: Text.ElideLeft
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: devConnection.openRecentFile(parent.filePath)
                        }
                    }
                }

                MenuRow {
                    visible: devConnection.recentOpenedFiles.length > 2
                    Item {
                        Layout.fillWidth: true
                        Layout.preferredHeight: theme.controlHeight
                        property string filePath: devConnection.recentOpenedFiles.length > 2 ? devConnection.recentOpenedFiles[2] : ""

                        Rectangle {
                            anchors.fill: parent
                            color: "transparent"
                            border.width: 0
                        }

                        CText {
                            anchors.fill: parent
                            anchors.leftMargin: 6
                            anchors.rightMargin: 6
                            text: parent.filePath
                            horizontalAlignment: Text.AlignRight
                            verticalAlignment: Text.AlignVCenter
                            elide: Text.ElideLeft
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: devConnection.openRecentFile(parent.filePath)
                        }
                    }
                }
            }
        }

//        DevAddrBox {
//            dev: devConnection.dev
//            Layout.fillWidth: true
//            Layout.preferredWidth: parent.width
//        }
    }
}
